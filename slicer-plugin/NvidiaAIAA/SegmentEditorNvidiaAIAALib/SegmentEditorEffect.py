import json
import logging
import os
import shutil
import tempfile
import time
from collections import OrderedDict

import SimpleITK as sitk
import qt
import sitkUtils
import slicer
import vtk
from SegmentEditorEffects import *

from NvidiaAIAAClientAPI.client_api import AIAAClient


class SegmentEditorEffect(AbstractScriptedSegmentEditorEffect):
    """This effect uses NVIDIA AIAA algorithm for segmentation the input volume"""

    def __init__(self, scriptedEffect):
        scriptedEffect.name = 'Nvidia AIAA'
        # this effect operates on all segments at once (not on a single selected segment)
        scriptedEffect.perSegment = False
        # this effect can create its own segments, so we do not require any pre-existing segment
        if (slicer.app.majorVersion >= 5) or (slicer.app.majorVersion >= 4 and slicer.app.minorVersion >= 11):
            scriptedEffect.requireSegments = False
                
        AbstractScriptedSegmentEditorEffect.__init__(self, scriptedEffect)

        self.models = OrderedDict()

        # Effect-specific members
        self.segmentMarkupNode = None
        self.segmentMarkupNodeObservers = []

        self.connectedToEditorForSegmentChange = False
        self.isActivated = False

        self.progressBar = None
        self.logic = None

        self.observedParameterSetNode = None
        self.parameterSetNodeObserverTags = []
        self.observedSegmentation = None
        self.segmentationNodeObserverTags = []

    def __del__(self):
        AbstractScriptedSegmentEditorEffect.__del__(self)
        if self.progressBar:
            self.progressBar.close()

    def clone(self):
        # It should not be necessary to modify this method
        import qSlicerSegmentationsEditorEffectsPythonQt as effects
        clonedEffect = effects.qSlicerSegmentEditorScriptedEffect(None)
        clonedEffect.setPythonSource(__file__.replace('\\', '/'))
        return clonedEffect

    def icon(self, name='SegmentEditorEffect.png'):
        # It should not be necessary to modify this method
        iconPath = os.path.join(os.path.dirname(__file__), name)
        if os.path.exists(iconPath):
            return qt.QIcon(iconPath)
        return qt.QIcon()

    def helpText(self):
        return """NVIDIA AI-Assisted Annotation for automatic and boundary points based segmentation
    . The module requires access to an NVidia Clara AIAA server. See <a href="https://github.com/NVIDIA/ai-assisted-annotation-client/tree/master/slicer-plugin">module documentation</a> for more information."""

    def serverUrl(self):
        serverUrl = self.ui.serverComboBox.currentText
        if not serverUrl:
            # Default Slicer AIAA server
            serverUrl = "http://skull.cs.queensu.ca:8123"
        return serverUrl

    def setupOptionsFrame(self):

        if (slicer.app.majorVersion == 4 and slicer.app.minorVersion <= 10):
            self.scriptedEffect.addOptionsWidget(qt.QLabel("This effect only works in recent 3D Slicer Preview Release (Slicer-4.11.x)."))
            return

        # Load widget from .ui file. This .ui file can be edited using Qt Designer
        # (Edit / Application Settings / Developer / Qt Designer -> launch).
        uiWidget = slicer.util.loadUI(os.path.join(os.path.dirname(__file__), "SegmentEditorNvidiaAIAA.ui"))
        self.scriptedEffect.addOptionsWidget(uiWidget)
        self.ui = slicer.util.childWidgetVariables(uiWidget)

        # Set icons and tune widget properties

        self.ui.segmentationButton.setIcon(self.icon('nvidia-icon.png'))
        self.ui.annotationModelFilterPushButton.setIcon(self.icon('filter-icon.png'))
        self.ui.fetchModelsButton.setIcon(slicer.util.mainWindow().style().standardIcon(qt.QStyle.SP_BrowserReload))
        self.ui.fiducialEditButton.setIcon(self.icon('edit-icon.png'))
        self.ui.annotationButton.setIcon(self.icon('nvidia-icon.png'))

        self.ui.fiducialPlacementWidget.setMRMLScene(slicer.mrmlScene)
        self.ui.fiducialPlacementWidget.buttonsVisible = False
        self.ui.fiducialPlacementWidget.show()
        self.ui.fiducialPlacementWidget.placeButton().show()
        self.ui.fiducialPlacementWidget.deleteButton().show()

        # Connections
        self.ui.fetchModelsButton.connect('clicked(bool)', self.onClickFetchModels)
        self.ui.segmentationModelSelector.connect("currentIndexChanged(int)", self.updateMRMLFromGUI)
        self.ui.segmentationButton.connect('clicked(bool)', self.onClickSegmentation)
        self.ui.annotationModelSelector.connect("currentIndexChanged(int)", self.updateMRMLFromGUI)
        self.ui.fiducialEditButton.connect('clicked(bool)', self.onClickEditPoints)
        self.ui.annotationModelFilterPushButton.connect('toggled(bool)', self.updateMRMLFromGUI)
        self.ui.fiducialPlacementWidget.placeButton().clicked.connect(self.onfiducialPlacementWidgetChanged)
        self.ui.annotationButton.connect('clicked(bool)', self.onClickAnnotation)

    def onCurrentSegmentIDChanged(self, segmentID):
        logging.debug('onCurrentSegmentIDChanged: {}'.format(segmentID))
        if self.isActivated:
            self.updateGUIFromMRML()

    def currentSegment(self):
        pnode = self.scriptedEffect.parameterSetNode()
        segmentationNode = pnode.GetSegmentationNode()
        segmentation = segmentationNode.GetSegmentation() if segmentationNode else None
        if not pnode or not segmentation or not pnode.GetSelectedSegmentID():
            return None
        return segmentation.GetSegment(pnode.GetSelectedSegmentID())

    def currentSegmentID(self):
        pnode = self.scriptedEffect.parameterSetNode()
        return pnode.GetSelectedSegmentID() if pnode else None

    def updateServerSettings(self):
        self.logic.setServer(self.serverUrl())
        self.logic.setUseCompression(slicer.util.settingsValue("NVIDIA-AIAA/compressData", True, converter=slicer.util.toBool))

    def onClickFetchModels(self):

        self.updateMRMLFromGUI()

        # Save selected server URL
        settings = qt.QSettings()
        serverUrl = self.ui.serverComboBox.currentText
        settings.setValue("NVIDIA-AIAA/serverUrl", serverUrl)

        # Save current server URL to the top of history
        serverUrlHistory = settings.value("NVIDIA-AIAA/serverUrlHistory")
        if serverUrlHistory:
            serverUrlHistory = serverUrlHistory.split(";")
        else:
            serverUrlHistory = []
        try:
            serverUrlHistory.remove(serverUrl)
        except ValueError:
            pass
        serverUrlHistory.insert(0, serverUrl)
        serverUrlHistory = serverUrlHistory[:10]  # keep up to first 10 elements
        settings.setValue("NVIDIA-AIAA/serverUrlHistory", ";".join(serverUrlHistory))

        self.updateServerUrlGUIFromSettings()

        start = time.time()
        try:
            self.updateServerSettings()
            models = self.logic.list_models(self.ui.modelFilterLabelLineEdit.text)
        except:
            slicer.util.errorDisplay(
                "Failed to fetch models from remote server. Make sure server address is correct and retry.",
                detailedText=traceback.format_exc())
            self.ui.modelsCollapsibleButton.collapsed = False
            return

        self.ui.modelsCollapsibleButton.collapsed = True

        self.models.clear()
        for model in models:
            model_name = model['name']
            model_type = model['type']
            logging.debug('{} = {}'.format(model_name, model_type))
            self.models[model_name] = model

        self.updateGUIFromMRML()

        msg = ''
        msg += '-----------------------------------------------------\t\n'
        label = self.ui.modelFilterLabelLineEdit.text
        if label:
            msg += 'Using Label: \t\t' + label + '\t\n'
        msg += 'Total Models Loaded: \t' + str(len(models)) + '\t\n'
        msg += '-----------------------------------------------------\t\n'
        msg += 'Segmentation Models: \t' + str(self.ui.segmentationModelSelector.count) + '\t\n'
        msg += 'Annotation Models: \t' + str(self.ui.annotationModelSelector.count) + '\t\n'
        msg += '-----------------------------------------------------\t\n'
        # qt.QMessageBox.information(slicer.util.mainWindow(), 'NVIDIA AIAA', msg)
        logging.debug(msg)
        logging.info("Time consumed by onClickFetchModels: {0:3.1f}".format(time.time() - start))

    def updateSegmentationMask(self, extreme_points, in_file, modelInfo, overwriteCurrentSegment=False):
        start = time.time()
        logging.debug('Update Segmentation Mask from: {}'.format(in_file))
        if in_file is None or os.path.exists(in_file) is False:
            return False

        segmentationNode = self.scriptedEffect.parameterSetNode().GetSegmentationNode()
        segmentation = segmentationNode.GetSegmentation()
        currentSegment = self.currentSegment()

        labelImage = sitk.ReadImage(in_file)
        labelmapVolumeNode = sitkUtils.PushVolumeToSlicer(labelImage, None, className='vtkMRMLLabelMapVolumeNode')

        numberOfExistingSegments = segmentation.GetNumberOfSegments()
        slicer.modules.segmentations.logic().ImportLabelmapToSegmentationNode(labelmapVolumeNode, segmentationNode)
        slicer.mrmlScene.RemoveNode(labelmapVolumeNode)

        modelLabels = modelInfo['labels']
        numberOfAddedSegments = segmentation.GetNumberOfSegments() - numberOfExistingSegments
        logging.debug('Adding {} segments'.format(numberOfAddedSegments))
        addedSegmentIds = [segmentation.GetNthSegmentID(numberOfExistingSegments + i) for i in range(numberOfAddedSegments)]
        for i, segmentId in enumerate(addedSegmentIds):
            segment = segmentation.GetSegment(segmentId)
            if i==0 and overwriteCurrentSegment and currentSegment:
                logging.debug('Update current segment with id: {} => {}'.format(segmentId, segment.GetName()))
                # Copy labelmap representation to the current segment then remove the imported segment
                labelmap = slicer.vtkOrientedImageData()
                segmentationNode.GetBinaryLabelmapRepresentation(segmentId, labelmap)
                self.scriptedEffect.modifySelectedSegmentByLabelmap(labelmap, slicer.qSlicerSegmentEditorAbstractEffect.ModificationModeSet)
                segmentationNode.RemoveSegment(segmentId)
            else:
                logging.debug('Setting new segmentation with id: {} => {}'.format(segmentId, segment.GetName()))
                if i<len(modelLabels):
                    segment.SetName(modelLabels[i])
                else:
                    # we did not get enough labels (for exampe annotation_mri_prostate_cg_and_pz model returns a labelmap with
                    # 2 labels but in the model infor only 1 label is provided)
                    segment.SetName("unknown {}".format(i))

        # Save extreme points into first segment
        if extreme_points:
            logging.debug('Extreme Points: {}'.format(extreme_points))
            if overwriteCurrentSegment and currentSegment:
                segment = currentSegment
            else:
                segment = segmentation.GetNthSegment(numberOfExistingSegments)
            if segment:
                segment.SetTag("AIAA.DExtr3DExtremePoints", json.dumps(extreme_points))

        os.unlink(in_file)
        logging.info("Time consumed by updateSegmentationMask: {0:3.1f}".format(time.time() - start))
        return True

    def setProgressBarLabelText(self, label):
        if not self.progressBar:
            self.progressBar = slicer.util.createProgressDialog(windowTitle="Wait...", maximum=100)
        self.progressBar.labelText = label

    def reportProgress(self, progressPercentage):
        if not self.progressBar:
            self.progressBar = slicer.util.createProgressDialog(windowTitle="Wait...", maximum=100)
        self.progressBar.show()
        self.progressBar.activateWindow()
        self.progressBar.setValue(progressPercentage)
        slicer.app.processEvents()

    def getPermissionForImageDataUpload(self):
        return slicer.util.confirmOkCancelDisplay("Master volume - without any additional patient information -"
            " will be sent to remote data processing server: {0}.\n\n"
            "Click 'OK' to proceed with the segmentation.\n"
            "Click 'Cancel' to not upload any data and cancel segmentation.\n".format(self.serverUrl()),
            dontShowAgainSettingsKey = "NVIDIA-AIAA/showImageDataSendWarning")

    def onClickSegmentation(self):
        if not self.getPermissionForImageDataUpload():
            return

        start = time.time()
        model = self.ui.segmentationModelSelector.currentText
        modelInfo = self.models[model]

        operationDescription = 'Run Segmentation for model: {}'.format(model)
        logging.debug(operationDescription)
        self.setProgressBarLabelText(operationDescription)
        slicer.app.processEvents()
        result = 'FAILED'
        try:
            qt.QApplication.setOverrideCursor(qt.Qt.WaitCursor)
            inputVolume = self.scriptedEffect.parameterSetNode().GetMasterVolumeNode()
            self.updateServerSettings()
            extreme_points, result_file = self.logic.segmentation(model, inputVolume)
            if self.updateSegmentationMask(extreme_points, result_file, modelInfo):
                result = 'SUCCESS'
                self.segmentMarkupNode.RemoveAllMarkups()
                self.updateGUIFromMRML()
        except:
            qt.QApplication.restoreOverrideCursor()
            self.progressBar.hide()
            slicer.util.errorDisplay(operationDescription + " - unexpected error.", detailedText=traceback.format_exc())
            return

        qt.QApplication.restoreOverrideCursor()
        self.progressBar.hide()

        msg = 'Run segmentation for ({0}): {1}\t\nTime Consumed: {2:3.1f} (sec)'.format(model, result, (time.time() - start))
        logging.info(msg)

        self.onClickEditPoints()

    def getFiducialPointsXYZ(self):
        v = self.scriptedEffect.parameterSetNode().GetMasterVolumeNode()
        RasToIjkMatrix = vtk.vtkMatrix4x4()
        v.GetRASToIJKMatrix(RasToIjkMatrix)

        point_set = []
        n = self.segmentMarkupNode.GetNumberOfFiducials()
        for i in range(n):
            coord = [0.0, 0.0, 0.0]
            self.segmentMarkupNode.GetNthFiducialPosition(i, coord)

            p_Ras = [coord[0], coord[1], coord[2], 1.0]
            p_Ijk = RasToIjkMatrix.MultiplyDoublePoint(p_Ras)

            logging.debug('From Fiducial: {} => {}'.format(coord, p_Ijk))
            point_set.append(p_Ijk[0:3])

        logging.info('Current Fiducials-Points: {}'.format(point_set))
        return point_set

    def onClickAnnotation(self):
        if not self.getPermissionForImageDataUpload():
            return

        start = time.time()
        self.ui.fiducialPlacementWidget.placeModeEnabled = False
        model = self.ui.annotationModelSelector.currentText
        label = self.currentSegment().GetName()
        operationDescription = 'Run Annotation for model: {} for segment: {}'.format(model, label)
        logging.debug(operationDescription)
        self.setProgressBarLabelText(operationDescription)
        slicer.app.processEvents()
        try:
            qt.QApplication.setOverrideCursor(qt.Qt.WaitCursor)
            inputVolume = self.scriptedEffect.parameterSetNode().GetMasterVolumeNode()
            modelInfo = self.models[model]
            pointSet = self.getFiducialPointsXYZ()
            self.updateServerSettings()
            result_file = self.logic.dextr3d(model, pointSet, inputVolume, modelInfo)
            result = 'FAILED'
            if self.updateSegmentationMask(pointSet, result_file, modelInfo, overwriteCurrentSegment=True):
                result = 'SUCCESS'
                self.segmentMarkupNode.RemoveAllMarkups()
                self.updateGUIFromMRML()
        except:
            qt.QApplication.restoreOverrideCursor()
            self.progressBar.hide()
            slicer.util.errorDisplay(operationDescription + " - unexpected error.", detailedText=traceback.format_exc())
            return

        qt.QApplication.restoreOverrideCursor()
        self.progressBar.hide()

        msg = 'Run annotation for ({0}): {1}\t\nTime Consumed: {2:3.1f} (sec)'.format(model, result, (time.time() - start))
        logging.info(msg)

    def onClickEditPoints(self):
        segment = self.currentSegment()
        segmentId = self.currentSegmentID()

        self.segmentMarkupNode.RemoveAllMarkups()

        if segmentId:
            v = self.scriptedEffect.parameterSetNode().GetMasterVolumeNode()
            IjkToRasMatrix = vtk.vtkMatrix4x4()
            v.GetIJKToRASMatrix(IjkToRasMatrix)

            fPosStr = vtk.mutable("")
            segment.GetTag("AIAA.DExtr3DExtremePoints", fPosStr)
            pointset = str(fPosStr)
            logging.debug('{} => {} Extreme points are: {}'.format(segmentId, segment.GetName(), pointset))

            if fPosStr is not None and len(pointset) > 0:
                points = json.loads(pointset)
                for p in points:
                    p_Ijk = [p[0], p[1], p[2], 1.0]
                    p_Ras = IjkToRasMatrix.MultiplyDoublePoint(p_Ijk)
                    logging.debug('Add Fiducial: {} => {}'.format(p_Ijk, p_Ras))
                    self.segmentMarkupNode.AddFiducialFromArray(p_Ras[0:3])

        self.updateGUIFromMRML()

    def reset(self):
        if self.ui.fiducialPlacementWidget.placeModeEnabled:
            self.ui.fiducialPlacementWidget.setPlaceModeEnabled(False)

        if self.segmentMarkupNode:
            slicer.mrmlScene.RemoveNode(self.segmentMarkupNode)
            self.setAndObserveSegmentMarkupNode(None)

    def activate(self):
        logging.debug('NVidia AIAA effect activated')

        if not self.logic:
            self.logic = AIAALogic()
            self.logic.scriptedEffect = self.scriptedEffect
            self.logic.setProgressCallback(progress_callback=lambda progressPercentage: self.reportProgress(progressPercentage))

        self.isActivated = True
        self.scriptedEffect.showEffectCursorInSliceView = False

        self.updateServerUrlGUIFromSettings()

        # Create empty markup fiducial node
        if not self.segmentMarkupNode:
            self.createNewMarkupNode()
            self.ui.fiducialPlacementWidget.setCurrentNode(self.segmentMarkupNode)
            self.setAndObserveSegmentMarkupNode(self.segmentMarkupNode)
            self.ui.fiducialPlacementWidget.setPlaceModeEnabled(False)

        self.updateGUIFromMRML()

        self.onClickFetchModels()

        self.observeParameterNode(True)
        self.observeSegmentation(True)

    def deactivate(self):
        logging.debug('NVidia AIAA effect deactivated')
        self.isActivated = False
        self.observeSegmentation(False)
        self.observeParameterNode(False)
        self.reset()

    def createCursor(self, widget):
        # Turn off effect-specific cursor for this effect
        return slicer.util.mainWindow().cursor

    def setMRMLDefaults(self):
        self.scriptedEffect.setParameterDefault("ModelFilterLabel", "")
        self.scriptedEffect.setParameterDefault("SegmentationModel", "")
        self.scriptedEffect.setParameterDefault("AnnotationModel", "")
        self.scriptedEffect.setParameterDefault("AnnotationModelFiltered", 0)

    def observeParameterNode(self, observationEnabled):
        parameterSetNode = self.scriptedEffect.parameterSetNode()
        if observationEnabled and self.observedParameterSetNode == parameterSetNode:
          return
        if not observationEnabled and not self.observedSegmentation:
          return
        # Need to update the observer
        # Remove old observer
        if self.observedParameterSetNode:
          for tag in self.parameterSetNodeObserverTags:
            self.observedParameterSetNode.RemoveObserver(tag)
          self.parameterSetNodeObserverTags = []
          self.observedParameterSetNode = None
        # Add new observer
        if observationEnabled and parameterSetNode is not None:
          self.observedParameterSetNode = parameterSetNode
          observedEvents = [
            vtk.vtkCommand.ModifiedEvent
            ]
          for eventId in observedEvents:
            self.parameterSetNodeObserverTags.append(self.observedParameterSetNode.AddObserver(eventId, self.onParameterSetNodeModified))

    def observeSegmentation(self, observationEnabled):
        segmentationNode = self.scriptedEffect.parameterSetNode().GetSegmentationNode()
        segmentation = segmentationNode.GetSegmentation() if segmentationNode else None
        if observationEnabled and self.observedSegmentation == segmentation:
          return
        if not observationEnabled and not self.observedSegmentation:
          return
        # Need to update the observer
        # Remove old observer
        if self.observedSegmentation:
          for tag in self.segmentationNodeObserverTags:
            self.observedSegmentation.RemoveObserver(tag)
          self.segmentationNodeObserverTags = []
          self.observedSegmentation = None
        # Add new observer
        if observationEnabled and segmentation is not None:
          self.observedSegmentation = segmentation
          observedEvents = [
            slicer.vtkSegmentation.SegmentModified
            ]
          for eventId in observedEvents:
            self.segmentationNodeObserverTags.append(self.observedSegmentation.AddObserver(eventId, self.onSegmentationModified))

    def onParameterSetNodeModified(self, caller, event):
        self.updateGUIFromMRML()

    def onSegmentationModified(self, caller, event):
        self.updateGUIFromMRML()

    def updateServerUrlGUIFromSettings(self):
        # Save current server URL to the top of history
        settings = qt.QSettings()
        serverUrlHistory = settings.value("NVIDIA-AIAA/serverUrlHistory")
        wasBlocked = self.ui.serverComboBox.blockSignals(True)
        self.ui.serverComboBox.clear()
        if serverUrlHistory:
            self.ui.serverComboBox.addItems(serverUrlHistory.split(";"))
        self.ui.serverComboBox.setCurrentText(settings.value("NVIDIA-AIAA/serverUrl"))
        self.ui.serverComboBox.blockSignals(wasBlocked)

    def updateGUIFromMRML(self):
        modelFilterLabel = self.scriptedEffect.parameter("ModelFilterLabel") if self.scriptedEffect.parameterDefined("ModelFilterLabel") else ""
        wasBlocked = self.ui.modelFilterLabelLineEdit.blockSignals(True)
        self.ui.modelFilterLabelLineEdit.setText(modelFilterLabel)
        self.ui.modelFilterLabelLineEdit.blockSignals(wasBlocked)

        annotationModelFiltered = self.scriptedEffect.integerParameter("AnnotationModelFiltered") != 0
        wasBlocked = self.ui.annotationModelFilterPushButton.blockSignals(True)
        self.ui.annotationModelFilterPushButton.checked = annotationModelFiltered
        self.ui.annotationModelFilterPushButton.blockSignals(wasBlocked)

        wasSegmentationModelSelectorBlocked = self.ui.segmentationModelSelector.blockSignals(True)
        wasAnnotationModelSelectorBlocked = self.ui.annotationModelSelector.blockSignals(True)
        self.ui.segmentationModelSelector.clear()
        self.ui.annotationModelSelector.clear()
        currentSegment = self.currentSegment()
        currentSegmentName = currentSegment.GetName().lower() if currentSegment else ""
        for model_name, model in self.models.items():
            if model['type'] == 'segmentation':
                modelWidget = self.ui.segmentationModelSelector
            else:
                if annotationModelFiltered and not (currentSegmentName in model_name.lower()):
                    continue
                modelWidget = self.ui.annotationModelSelector
            modelWidget.addItem(model_name)
            modelWidget.setItemData(modelWidget.count-1, model['description'], qt.Qt.ToolTipRole)

        segmentationModel = self.scriptedEffect.parameter("SegmentationModel") if self.scriptedEffect.parameterDefined("SegmentationModel") else ""
        segmentationModelIndex = self.ui.segmentationModelSelector.findText(segmentationModel)
        self.ui.segmentationModelSelector.setCurrentIndex(segmentationModelIndex)
        try:
            modelInfo = self.models[segmentationModel]
            self.ui.segmentationModelSelector.setToolTip(modelInfo["description"])
        except:
            self.ui.segmentationModelSelector.setToolTip("")

        annotationModel = vtk.mutable("")
        currentSegment = self.currentSegment()
        if currentSegment:
            currentSegment.GetTag("AIAA.AnnotationModel", annotationModel)
        annotationModelIndex = self.ui.annotationModelSelector.findText(annotationModel)
        self.ui.annotationModelSelector.setCurrentIndex(annotationModelIndex)
        try:
            modelInfo = self.models[annotationModel]
            self.ui.annotationModelSelector.setToolTip(modelInfo["description"])
        except:
            self.ui.annotationModelSelector.setToolTip("")

        self.ui.segmentationModelSelector.blockSignals(wasSegmentationModelSelectorBlocked)
        self.ui.annotationModelSelector.blockSignals(wasAnnotationModelSelectorBlocked)

        self.ui.segmentationButton.setEnabled(self.ui.segmentationModelSelector.currentText)

        if self.currentSegment() and self.segmentMarkupNode and self.ui.annotationModelSelector.currentText:
            numberOfDefinedPoints = self.segmentMarkupNode.GetNumberOfDefinedControlPoints()
            if numberOfDefinedPoints >= 6:
                self.ui.annotationButton.setEnabled(True)
                self.ui.annotationButton.setToolTip("Segment the object based on specified boundary points")
            else:
                self.ui.annotationButton.setEnabled(False)
                self.ui.annotationButton.setToolTip("Not enough points. Place at least 6 points near the boundaries of the object (one or more on each side).")
        else:
            self.ui.annotationButton.setEnabled(False)
            self.ui.annotationButton.setToolTip("Select a segment from the segment list and place boundary points.")

        segment = self.currentSegment()
        self.ui.fiducialEditButton.setEnabled(segment and segment.HasTag("AIAA.DExtr3DExtremePoints"))

    def updateMRMLFromGUI(self):

        wasModified = self.scriptedEffect.parameterSetNode().StartModify()
        self.scriptedEffect.setParameter("ModelFilterLabel", self.ui.modelFilterLabelLineEdit.text)

        segmentationModelIndex = self.ui.segmentationModelSelector.currentIndex
        if segmentationModelIndex >= 0:
            # Only overwrite segmentation model in MRML node if there is a valid selection
            # (to not clear the model if it is temporarily not available)
            segmentationModel = self.ui.segmentationModelSelector.itemText(segmentationModelIndex)
            self.scriptedEffect.setParameter("SegmentationModel", segmentationModel)

        annotationModelIndex = self.ui.annotationModelSelector.currentIndex
        if annotationModelIndex >= 0:
            # Only overwrite annotation model in MRML node if there is a valid selection
            # (to not clear the model if it is temporarily not available)
            annotationModel = self.ui.annotationModelSelector.itemText(annotationModelIndex)
            currentSegment = self.currentSegment()
            if currentSegment:
                currentSegment.SetTag("AIAA.AnnotationModel", annotationModel)

        self.scriptedEffect.setParameter("AnnotationModelFiltered", 1 if self.ui.annotationModelFilterPushButton.checked else 0)
        self.scriptedEffect.parameterSetNode().EndModify(wasModified)

    def onfiducialPlacementWidgetChanged(self):
        if self.segmentMarkupNode or not self.ui.fiducialPlacementWidget.placeButton().isChecked():
            return
        # Create empty markup fiducial node if it does not exist already
        self.createNewMarkupNode()
        self.ui.fiducialPlacementWidget.setCurrentNode(self.segmentMarkupNode)

    def createNewMarkupNode(self):
        # Create empty markup fiducial node
        if self.segmentMarkupNode:
            return
        displayNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLMarkupsDisplayNode")
        displayNode.SetTextScale(0)
        self.segmentMarkupNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLMarkupsFiducialNode")
        self.segmentMarkupNode.SetName('A')
        self.segmentMarkupNode.SetAndObserveDisplayNodeID(displayNode.GetID())
        self.setAndObserveSegmentMarkupNode(self.segmentMarkupNode)
        self.updateGUIFromMRML()

    def setAndObserveSegmentMarkupNode(self, segmentMarkupNode):
        if segmentMarkupNode == self.segmentMarkupNode and self.segmentMarkupNodeObservers:
            # no change and node is already observed
            return

        # Remove observer to old parameter node
        if self.segmentMarkupNode and self.segmentMarkupNodeObservers:
            for observer in self.segmentMarkupNodeObservers:
                self.segmentMarkupNode.RemoveObserver(observer)
            self.segmentMarkupNodeObservers = []

        # Set and observe new parameter node
        self.segmentMarkupNode = segmentMarkupNode
        if self.segmentMarkupNode:
            eventIds = [vtk.vtkCommand.ModifiedEvent,
                        slicer.vtkMRMLMarkupsNode.PointModifiedEvent,
                        slicer.vtkMRMLMarkupsNode.PointAddedEvent,
                        slicer.vtkMRMLMarkupsNode.PointRemovedEvent]
            for eventId in eventIds:
                self.segmentMarkupNodeObservers.append(
                    self.segmentMarkupNode.AddObserver(eventId, self.onSegmentMarkupNodeModified))

        # Update GUI
        self.updateModelFromSegmentMarkupNode()

    def onSegmentMarkupNodeModified(self, observer, eventid):
        self.updateModelFromSegmentMarkupNode()
        self.updateGUIFromMRML()

    def updateModelFromSegmentMarkupNode(self):
        self.updateGUIFromMRML()

    def interactionNodeModified(self, interactionNode):
        self.updateGUIFromMRML()

class AIAALogic():
    def __init__(self, server_url=None, server_version=None, progress_callback=None):

        self.aiaa_tmpdir = slicer.util.tempDirectory('slicer-aiaa')
        self.volumeToImageFiles = dict()
        self.progress_callback = progress_callback
        self.useCompression = True

        # Create Single AIAA Client Instance
        self.aiaaClient = AIAAClient()
        self.setServer(server_url, server_version)

    def __del__(self):
        shutil.rmtree(self.aiaa_tmpdir, ignore_errors=True)

    def inputFileExtension(self):
        return ".nii.gz" if self.useCompression else ".nii"

    def outputFileExtension(self):
        # output is currently always generated as .nii.gz
        return ".nii.gz"

    def setServer(self, server_url=None, server_version=None):
        if not server_url:
            server_url='http://0.0.0.0:5000'
        if not server_version:
            server_version='v1'
        logging.debug('Using AIAA server {}: {}'.format(server_version, server_url))
        self.aiaaClient.server_url = server_url
        self.aiaaClient.api_version = server_version

    def setUseCompression(self, useCompression):
        self.useCompression = useCompression

    def setProgressCallback(self, progress_callback=None):
        self.progress_callback = progress_callback

    def reportProgress(self, progress):
        if self.progress_callback:
            self.progress_callback(progress)

    def list_models(self, label=None):
        #self.reportProgress(0)
        logging.debug('Fetching List of Models for label: {}'.format(label))
        result = self.aiaaClient.model_list(label)
        #self.reportProgress(100)
        return result

    def nodeCacheKey(self, mrmlNode):
        return mrmlNode.GetID()+"*"+str(mrmlNode.GetMTime())

    def segmentation(self, model, inputVolume):
        logging.debug('Preparing input data for segmentation')
        self.reportProgress(0)
        in_file = self.volumeToImageFiles.get(self.nodeCacheKey(inputVolume))
        if in_file is None:
            # No cached file
            in_file = tempfile.NamedTemporaryFile(suffix=self.inputFileExtension(), dir=self.aiaa_tmpdir).name
            self.reportProgress(5)
            start = time.time()
            slicer.util.saveNode(inputVolume, in_file)
            logging.info('Saved Input Node into {0} in {1:3.1f}s'.format(in_file, time.time() - start))
            self.volumeToImageFiles[self.nodeCacheKey(inputVolume)] = in_file
        else:
            logging.debug('Using cached image file: {}'.format(in_file))

        self.reportProgress(30)

        result_file = tempfile.NamedTemporaryFile(suffix=self.outputFileExtension(), dir=self.aiaa_tmpdir).name
        params = self.aiaaClient.segmentation(model, in_file, result_file, save_doc=True)

        extreme_points = params.get('points', params.get('extreme_points'))
        logging.debug('Extreme Points: {}'.format(extreme_points))

        self.reportProgress(100)
        return extreme_points, result_file

    def dextr3d(self, model, pointset, inputVolume, modelInfo):
        self.reportProgress(0)

        logging.debug('Preparing for Annotation/Dextr3D Action')

        node_id = inputVolume.GetID()
        in_file = self.volumeToImageFiles.get(self.nodeCacheKey(inputVolume))
        logging.debug('Node Id: {} => {}'.format(node_id, in_file))

        if in_file is None:
            in_file = tempfile.NamedTemporaryFile(suffix=self.inputFileExtension(), dir=self.aiaa_tmpdir).name

            self.reportProgress(5)
            start = time.time()
            slicer.util.saveNode(inputVolume, in_file)
            logging.info('Saved Input Node into {0} in {1:3.1f}s'.format(in_file, time.time() - start))

            self.volumeToImageFiles[self.nodeCacheKey(inputVolume)] = in_file
        else:
            logging.debug('Using Saved Node from: {}'.format(in_file))

        self.reportProgress(30)

        # Pre Process
        pad = 20
        roi_size = '128x128x128'
        if (modelInfo is not None) and ('padding' in modelInfo):
            pad = modelInfo['padding']
            roi_size = 'x'.join(map(str, modelInfo['roi']))

        result_file = tempfile.NamedTemporaryFile(suffix=self.outputFileExtension(), dir=self.aiaa_tmpdir).name
        self.aiaaClient.dextr3d(model, pointset, in_file, result_file, pad, roi_size)

        self.reportProgress(100)
        return result_file
