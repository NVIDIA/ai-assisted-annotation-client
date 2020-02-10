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
        self.annotationFiducialNode = None
        self.annotationFiducialNodeObservers = []

        self.dgPositiveFiducialNode = None
        self.dgPositiveFiducialNodeObservers = []
        self.dgNegativeFiducialNode = None
        self.dgNegativeFiducialNodeObservers = []
        self.ignoreFiducialNodeAddEvent = False

        self.seedFiducialsNodeSelector = None

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
        self.ui.annotationButton.setIcon(self.icon('nvidia-icon.png'))

        self.ui.annotationFiducialEditButton.setIcon(self.icon('edit-icon.png'))
        self.ui.annotationFiducialPlacementWidget.setMRMLScene(slicer.mrmlScene)
        #self.ui.annotationFiducialPlacementWidget.placeButton().show()
        #self.ui.annotationFiducialPlacementWidget.deleteButton().show()

        self.ui.dgPositiveFiducialPlacementWidget.setMRMLScene(slicer.mrmlScene)
        self.ui.dgPositiveFiducialPlacementWidget.placeButton().toolTip = "Select +ve points"

        self.ui.dgNegativeFiducialPlacementWidget.setMRMLScene(slicer.mrmlScene)
        self.ui.dgNegativeFiducialPlacementWidget.placeButton().toolTip = "Select -ve points"

        # Connections
        self.ui.segmentationModelSelector.connect("currentIndexChanged(int)", self.updateMRMLFromGUI)
        self.ui.segmentationButton.connect('clicked(bool)', self.onClickSegmentation)
        self.ui.annotationModelSelector.connect("currentIndexChanged(int)", self.updateMRMLFromGUI)
        self.ui.annotationModelFilterPushButton.connect('toggled(bool)', self.updateMRMLFromGUI)
        self.ui.annotationFiducialEditButton.connect('clicked(bool)', self.onClickEditAnnotationFiducialPoints)
        self.ui.annotationButton.connect('clicked(bool)', self.onClickAnnotation)

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
        self.logic.setDeepGrowModel(slicer.util.settingsValue("NVIDIA-AIAA/deepgrowModel", "clara_deepgrow"))

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
            models = self.logic.list_models()
        except:
            slicer.util.errorDisplay(
                "Failed to fetch models from remote server. Make sure server address is correct and {}/v1/models is accessible in browser".format(self.serverUrl()),
                detailedText=traceback.format_exc())
            return

        self.models.clear()
        for model in models:
            model_name = model['name']
            model_type = model['type']
            logging.debug('{} = {}'.format(model_name, model_type))
            self.models[model_name] = model

        self.updateGUIFromMRML()

        msg = ''
        msg += '-----------------------------------------------------\t\n'
        msg += 'Total Models Loaded: \t' + str(len(models)) + '\t\n'
        msg += '-----------------------------------------------------\t\n'
        msg += 'Segmentation Models: \t' + str(self.ui.segmentationModelSelector.count) + '\t\n'
        msg += 'Annotation Models: \t' + str(self.ui.annotationModelSelector.count) + '\t\n'
        msg += '-----------------------------------------------------\t\n'

        # qt.QMessageBox.information(slicer.util.mainWindow(), 'NVIDIA AIAA', msg)
        logging.debug(msg)
        logging.info("Time consumed by onClickFetchModels: {0:3.1f}".format(time.time() - start))

    def updateSegmentationMask(self, extreme_points, in_file, modelInfo, overwriteCurrentSegment=False, unionLabel=False):
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

        modelLabels = [] if modelInfo is None else modelInfo['labels']
        numberOfAddedSegments = segmentation.GetNumberOfSegments() - numberOfExistingSegments
        logging.debug('Adding {} segments'.format(numberOfAddedSegments))

        addedSegmentIds = [segmentation.GetNthSegmentID(numberOfExistingSegments + i) for i in range(numberOfAddedSegments)]
        for i, segmentId in enumerate(addedSegmentIds):
            segment = segmentation.GetSegment(segmentId)
            if i == 0 and overwriteCurrentSegment and currentSegment:
                logging.debug('Update current segment with id: {} => {}'.format(segmentId, segment.GetName()))
                # Copy labelmap representation to the current segment then remove the imported segment
                labelmap = slicer.vtkOrientedImageData()
                segmentationNode.GetBinaryLabelmapRepresentation(segmentId, labelmap)

                # TODO:: Reset current slice and then union/add
                labelOp = slicer.qSlicerSegmentEditorAbstractEffect.ModificationModeAdd if unionLabel else slicer.qSlicerSegmentEditorAbstractEffect.ModificationModeSet
                self.scriptedEffect.modifySelectedSegmentByLabelmap(labelmap, labelOp)
                segmentationNode.RemoveSegment(segmentId)
            else:
                logging.debug('Setting new segmentation with id: {} => {}'.format(segmentId, segment.GetName()))
                if i<len(modelLabels):
                    segment.SetName(modelLabels[i])
                else:
                    # we did not get enough labels (for example annotation_mri_prostate_cg_and_pz model returns a labelmap with
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
        self.onClickEditAnnotationFiducialPoints()

    def getFiducialPointsXYZ(self, fiducialNode):
        v = self.scriptedEffect.parameterSetNode().GetMasterVolumeNode()
        RasToIjkMatrix = vtk.vtkMatrix4x4()
        v.GetRASToIJKMatrix(RasToIjkMatrix)

        point_set = []
        n = fiducialNode.GetNumberOfFiducials()
        for i in range(n):
            coord = [0.0, 0.0, 0.0]
            fiducialNode.GetNthFiducialPosition(i, coord)

            p_Ras = [coord[0], coord[1], coord[2], 1.0]
            p_Ijk = RasToIjkMatrix.MultiplyDoublePoint(p_Ras)

            logging.debug('From Fiducial: {} => {}'.format(coord, p_Ijk))
            point_set.append(p_Ijk[0:3])

        logging.info('Current Fiducials-Points: {}'.format(point_set))
        return point_set

    def getFiducialPointXYZ(self, fiducialNode, index):
        v = self.scriptedEffect.parameterSetNode().GetMasterVolumeNode()
        RasToIjkMatrix = vtk.vtkMatrix4x4()
        v.GetRASToIJKMatrix(RasToIjkMatrix)

        coord = [0.0, 0.0, 0.0]
        fiducialNode.GetNthFiducialPosition(index, coord)

        p_Ras = [coord[0], coord[1], coord[2], 1.0]
        p_Ijk = RasToIjkMatrix.MultiplyDoublePoint(p_Ras)

        logging.debug('From Fiducial: {} => {}'.format(coord, p_Ijk))

        pt = p_Ijk[0:3]
        logging.info('Current Fiducials-Point[{}]: {}'.format(index, pt))
        return pt

    def onClickAnnotation(self):
        if not self.getPermissionForImageDataUpload():
            return

        start = time.time()

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
            pointSet = self.getFiducialPointsXYZ(self.annotationFiducialNode)
            self.updateServerSettings()
            result_file = self.logic.dextr3d(model, pointSet, inputVolume, modelInfo)
            result = 'FAILED'
            if self.updateSegmentationMask(pointSet, result_file, modelInfo, overwriteCurrentSegment=True):
                result = 'SUCCESS'
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
        self.onClickEditAnnotationFiducialPoints()

    def onClickDeepgrow(self, current_point):
        segment = self.currentSegment()
        if not segment:
            return

        if not self.getPermissionForImageDataUpload():
            return

        start = time.time()

        label = self.currentSegment().GetName()
        operationDescription = 'Run Deepgrow for segment: {}'.format(label)
        logging.debug(operationDescription)

        showProgressBar = not self.logic.is_doc_saved()
        if showProgressBar:
            self.setProgressBarLabelText(operationDescription)
            slicer.app.processEvents()

        try:
            qt.QApplication.setOverrideCursor(qt.Qt.WaitCursor)
            inputVolume = self.scriptedEffect.parameterSetNode().GetMasterVolumeNode()

            foreground_all = self.getFiducialPointsXYZ(self.dgPositiveFiducialNode)
            background_all = self.getFiducialPointsXYZ(self.dgNegativeFiducialNode)

            segment.SetTag("AIAA.ForegroundPoints", json.dumps(foreground_all))
            segment.SetTag("AIAA.BackgroundPoints", json.dumps(background_all))

            foreground = [x for x in foreground_all if x[2] == current_point[2]]
            background = [x for x in background_all if x[2] == current_point[2]]

            logging.debug('Foreground: {}'.format(foreground))
            logging.debug('Background: {}'.format(background))

            self.updateServerSettings()

            #TODO:: Slice Index is currently from window A
            slice_index = current_point[2]
            result_file = self.logic.deepgrow(foreground, background, slice_index, inputVolume)
            result = 'FAILED'

            if self.updateSegmentationMask(None, result_file, None, overwriteCurrentSegment=True, unionLabel=True):
                result = 'SUCCESS'
                self.updateGUIFromMRML()
        except:
            if showProgressBar:
                self.progressBar.hide()
            qt.QApplication.restoreOverrideCursor()
            slicer.util.errorDisplay(operationDescription + " - unexpected error.", detailedText=traceback.format_exc())
            return

        qt.QApplication.restoreOverrideCursor()
        if showProgressBar:
            self.progressBar.hide()
        msg = 'Run deepgrow for ({0}): {1}\t\nTime Consumed: {2:3.1f} (sec)'.format(label, result, (time.time() - start))
        logging.info(msg)

    def onClickEditAnnotationFiducialPoints(self):
        self.onEditFiducialPoints(self.annotationFiducialNode, "AIAA.DExtr3DExtremePoints")

    def onEditFiducialPoints(self, fiducialNode, tagName):
        segment = self.currentSegment()
        segmentId = self.currentSegmentID()
        logging.debug('Current SegmentID: {}; Segment: {}'.format(segmentId, segment))

        if fiducialNode and segment and segmentId:
            fiducialNode.RemoveAllMarkups()

            v = self.scriptedEffect.parameterSetNode().GetMasterVolumeNode()
            IjkToRasMatrix = vtk.vtkMatrix4x4()
            v.GetIJKToRASMatrix(IjkToRasMatrix)

            fPosStr = vtk.mutable("")
            segment.GetTag(tagName, fPosStr)
            pointset = str(fPosStr)
            logging.debug('{} => {} Fiducial points are: {}'.format(segmentId, segment.GetName(), pointset))

            if fPosStr is not None and len(pointset) > 0:
                points = json.loads(pointset)
                for p in points:
                    p_Ijk = [p[0], p[1], p[2], 1.0]
                    p_Ras = IjkToRasMatrix.MultiplyDoublePoint(p_Ijk)
                    logging.debug('Add Fiducial: {} => {}'.format(p_Ijk, p_Ras))
                    fiducialNode.AddFiducialFromArray(p_Ras[0:3])

            self.updateGUIFromMRML()

    def reset(self):
        self.resetFiducial(self.ui.annotationFiducialPlacementWidget, self.annotationFiducialNode, self.annotationFiducialNodeObservers)
        self.annotationFiducialNode = None

    def resetFiducial(self, fiducialWidget, fiducialNode, fiducialNodeObservers):
        if fiducialWidget.placeModeEnabled:
            fiducialWidget.setPlaceModeEnabled(False)

        if fiducialNode:
            slicer.mrmlScene.RemoveNode(fiducialNode)
            self.removeFiducialNodeObservers(fiducialNode, fiducialNodeObservers)

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
        if not self.annotationFiducialNode:
            self.annotationFiducialNode, self.annotationFiducialNodeObservers = self.createFiducialNode('A', self.onAnnotationFiducialNodeModified)
            self.ui.annotationFiducialPlacementWidget.setCurrentNode(self.annotationFiducialNode)
            self.ui.annotationFiducialPlacementWidget.setPlaceModeEnabled(False)

        # Create empty markup fiducial node for deep grow +ve and -ve
        if not self.dgPositiveFiducialNode:
            self.dgPositiveFiducialNode, self.dgPositiveFiducialNodeObservers = self.createFiducialNode('P', self.onDeepGrowFiducialNodeModified)
            self.ui.dgPositiveFiducialPlacementWidget.setCurrentNode(self.dgPositiveFiducialNode)
            self.ui.dgPositiveFiducialPlacementWidget.setPlaceModeEnabled(False)

        if not self.dgNegativeFiducialNode:
            self.dgNegativeFiducialNode, self.dgNegativeFiducialNodeObservers = self.createFiducialNode('N', self.onDeepGrowFiducialNodeModified)
            self.ui.dgNegativeFiducialPlacementWidget.setCurrentNode(self.dgNegativeFiducialNode)
            self.ui.dgNegativeFiducialPlacementWidget.setPlaceModeEnabled(False)

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
        self.scriptedEffect.setParameterDefault("AnnotationModelFiltered", 1)

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
        logging.debug("Parameter Node Modified: {}".format(event))
        if self.isActivated:
            self.onClickFetchModels()

            self.ignoreFiducialNodeAddEvent = True
            self.onEditFiducialPoints(self.annotationFiducialNode, "AIAA.DExtr3DExtremePoints")
            self.onEditFiducialPoints(self.dgPositiveFiducialNode, "AIAA.ForegroundPoints")
            self.onEditFiducialPoints(self.dgNegativeFiducialNode, "AIAA.BackgroundPoints")
            self.ignoreFiducialNodeAddEvent = False
        else:
            self.updateGUIFromMRML()

    def onSegmentationModified(self, caller, event):
        logging.debug("Segmentation Modified: {}".format(event))
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
            if len(model["labels"]) == 0:
                continue
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

        if self.currentSegment() and self.annotationFiducialNode and self.ui.annotationModelSelector.currentText:
            numberOfDefinedPoints = self.annotationFiducialNode.GetNumberOfDefinedControlPoints()
            if numberOfDefinedPoints >= 6:
                self.ui.annotationButton.setEnabled(True)
                self.ui.annotationButton.setToolTip("Segment the object based on specified boundary points")
            else:
                self.ui.annotationButton.setEnabled(False)
                self.ui.annotationButton.setToolTip("Not enough points. Place at least 6 points near the boundaries of the object (one or more on each side).")
        else:
            self.ui.annotationButton.setEnabled(False)
            self.ui.annotationButton.setToolTip("Select a segment from the segment list and place boundary points.")

    def updateMRMLFromGUI(self):
        wasModified = self.scriptedEffect.parameterSetNode().StartModify()

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

    def createFiducialNode(self, name, onMarkupNodeModified):
        displayNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLMarkupsDisplayNode")
        displayNode.SetTextScale(0)
        fiducialNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLMarkupsFiducialNode")
        fiducialNode.SetName(name)
        fiducialNode.SetAndObserveDisplayNodeID(displayNode.GetID())

        fiducialNodeObservers = []
        self.addFiducialNodeObserver(fiducialNode, onMarkupNodeModified)
        return fiducialNode, fiducialNodeObservers

    def removeFiducialNodeObservers(self, fiducialNode, fiducialNodeObservers):
        if fiducialNode and fiducialNodeObservers:
            for observer in fiducialNodeObservers:
                fiducialNode.RemoveObserver(observer)

    def addFiducialNodeObserver(self, fiducialNode, onMarkupNodeModified):
        fiducialNodeObservers = []
        if fiducialNode:
            eventIds = [slicer.vtkMRMLMarkupsNode.PointPositionDefinedEvent]
            for eventId in eventIds:
                fiducialNodeObservers.append(fiducialNode.AddObserver(eventId, onMarkupNodeModified))
        return fiducialNodeObservers

    def onAnnotationFiducialNodeModified(self, observer, eventid):
        self.updateGUIFromMRML()

    def onDeepGrowFiducialNodeModified(self, observer, eventid):
        self.updateGUIFromMRML()

        if self.ignoreFiducialNodeAddEvent:
            return

        markupsNode = observer
        movingMarkupIndex = markupsNode.GetDisplayNode().GetActiveControlPoint()
        logging.debug("Markup point added; point ID = {}".format(movingMarkupIndex))

        current_point = self.getFiducialPointXYZ(markupsNode, movingMarkupIndex)
        logging.debug("Current Point: {}".format(current_point))

        self.onClickDeepgrow(current_point)

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
        self.deepgrowModel = 'clara_deepgrow'

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

    def setDeepGrowModel(self, deepgrowModel):
        self.deepgrowModel = deepgrowModel

    def setProgressCallback(self, progress_callback=None):
        self.progress_callback = progress_callback

    def reportProgress(self, progress):
        if self.progress_callback:
            self.progress_callback(progress)

    def is_doc_saved(self):
        return self.aiaaClient.doc_id is not None

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
        params = self.aiaaClient.segmentation(model, in_file, result_file, save_doc=False)

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

    def deepgrow(self, foreground_point_set, background_point_set, slice_index, inputVolume):
        logging.debug('Preparing for Deepgrow Action (model: {})'.format(self.deepgrowModel))
        params = {
            'foreground': foreground_point_set,
            'background': background_point_set,
            'slice': int(slice_index)
        }
        logging.debug('Params: {}'.format(params))

        node_id = inputVolume.GetID()
        in_file = self.volumeToImageFiles.get(self.nodeCacheKey(inputVolume))
        logging.debug('Node Id: {} => {}'.format(node_id, in_file))

        showProgress = not self.is_doc_saved()
        if in_file is None:
            if showProgress:
                self.reportProgress(0)
            in_file = tempfile.NamedTemporaryFile(suffix=self.inputFileExtension(), dir=self.aiaa_tmpdir).name

            if showProgress:
                self.reportProgress(10)

            start = time.time()
            slicer.util.saveNode(inputVolume, in_file)
            logging.info('Saved Input Node into {0} in {1:3.1f}s'.format(in_file, time.time() - start))

            self.volumeToImageFiles[self.nodeCacheKey(inputVolume)] = in_file
            if showProgress:
                self.reportProgress(50)
        else:
            logging.debug('Using Saved Node from: {}'.format(in_file))

        result_file = tempfile.NamedTemporaryFile(suffix=self.outputFileExtension(), dir=self.aiaa_tmpdir).name
        self.aiaaClient.deepgrow(self.deepgrowModel, params, in_file, result_file, True)

        if showProgress:
            self.reportProgress(100)
        return result_file
