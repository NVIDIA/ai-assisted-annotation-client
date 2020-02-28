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

from NvidiaAIAAClientAPI.client_api import AIAAClient, AIAAException, AIAAError, urlparse


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
        return """NVIDIA AI-Assisted Annotation for automatic and boundary points based segmentation. 
        The module requires access to an NVidia Clara AIAA server. 
        See <a href="https://github.com/NVIDIA/ai-assisted-annotation-client/tree/master/slicer-plugin">
        module documentation</a> for more information."""

    def serverUrl(self):
        serverUrl = self.ui.serverComboBox.currentText
        if not serverUrl:
            # Default Slicer AIAA server
            serverUrl = "http://skull.cs.queensu.ca:8123"
        return serverUrl

    def setupOptionsFrame(self):
        if (slicer.app.majorVersion == 4 and slicer.app.minorVersion <= 10):
            self.scriptedEffect.addOptionsWidget(qt.QLabel("This effect only works in "
                                                           "recent 3D Slicer Preview Release (Slicer-4.11.x)."))
            return

        # Load widget from .ui file. This .ui file can be edited using Qt Designer
        # (Edit / Application Settings / Developer / Qt Designer -> launch).
        uiWidget = slicer.util.loadUI(os.path.join(os.path.dirname(__file__), "SegmentEditorNvidiaAIAA.ui"))
        self.scriptedEffect.addOptionsWidget(uiWidget)
        self.ui = slicer.util.childWidgetVariables(uiWidget)

        # Set icons and tune widget properties

        self.ui.fetchModelsButton.setIcon(self.icon('refresh-icon.png'))
        self.ui.segmentationButton.setIcon(self.icon('nvidia-icon.png'))
        self.ui.annotationModelFilterPushButton.setIcon(self.icon('filter-icon.png'))
        self.ui.annotationButton.setIcon(self.icon('nvidia-icon.png'))
        self.ui.annotationFiducialEditButton.setIcon(self.icon('edit-icon.png'))

        self.ui.annotationFiducialPlacementWidget.setMRMLScene(slicer.mrmlScene)
        self.ui.annotationFiducialPlacementWidget.buttonsVisible = False
        self.ui.annotationFiducialPlacementWidget.placeButton().show()
        self.ui.annotationFiducialPlacementWidget.deleteButton().show()

        self.ui.dgPositiveFiducialPlacementWidget.setMRMLScene(slicer.mrmlScene)
        self.ui.dgPositiveFiducialPlacementWidget.placeButton().toolTip = "Select +ve points"
        self.ui.dgPositiveFiducialPlacementWidget.buttonsVisible = False
        self.ui.dgPositiveFiducialPlacementWidget.placeButton().show()
        self.ui.dgPositiveFiducialPlacementWidget.deleteButton().show()

        self.ui.dgNegativeFiducialPlacementWidget.setMRMLScene(slicer.mrmlScene)
        self.ui.dgNegativeFiducialPlacementWidget.placeButton().toolTip = "Select -ve points"
        self.ui.dgNegativeFiducialPlacementWidget.buttonsVisible = False
        self.ui.dgNegativeFiducialPlacementWidget.placeButton().show()
        self.ui.dgNegativeFiducialPlacementWidget.deleteButton().show()

        # Connections
        self.ui.fetchModelsButton.connect('clicked(bool)', self.onClickFetchModels)
        self.ui.serverComboBox.connect('currentIndexChanged(int)', self.onClickFetchModels)
        self.ui.segmentationModelSelector.connect("currentIndexChanged(int)", self.updateMRMLFromGUI)
        self.ui.segmentationButton.connect('clicked(bool)', self.onClickSegmentation)
        self.ui.annotationModelSelector.connect("currentIndexChanged(int)", self.updateMRMLFromGUI)
        self.ui.annotationModelFilterPushButton.connect('toggled(bool)', self.updateMRMLFromGUI)
        self.ui.annotationFiducialEditButton.connect('clicked(bool)', self.onClickEditAnnotationFiducialPoints)
        self.ui.annotationButton.connect('clicked(bool)', self.onClickAnnotation)
        self.ui.deepgrowModelSelector.connect("currentIndexChanged(int)", self.updateMRMLFromGUI)

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
        self.logic.setUseCompression(slicer.util.settingsValue(
            "NVIDIA-AIAA/compressData",
            True, converter=slicer.util.toBool))
        self.logic.setUseSession(slicer.util.settingsValue(
            "NVIDIA-AIAA/aiaaSession",
            True, converter=slicer.util.toBool))

        self.saveServerUrl()

    def saveServerUrl(self):
        self.updateMRMLFromGUI()

        # Save selected server URL
        settings = qt.QSettings()
        serverUrl = self.serverUrl()
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

    def onClickFetchModels(self):
        self.fetchAIAAModels(showInfo=False)

    def fetchAIAAModels(self, showInfo=False):
        if not self.logic:
            return

        start = time.time()
        try:
            self.updateServerSettings()
            models = self.logic.list_models()
        except:
            slicer.util.errorDisplay("Failed to fetch models from remote server. "
                                     "Make sure server address is correct and {}/v1/models "
                                     "is accessible in browser".format(self.serverUrl()),
                                     detailedText=traceback.format_exc())
            return

        self.models.clear()
        model_count = {}
        for model in models:
            model_name = model['name']
            model_type = model['type']
            model_count[model_type] = model_count.get(model_type, 0) + 1

            logging.debug('{} = {}'.format(model_name, model_type))
            self.models[model_name] = model

        self.updateGUIFromMRML()

        msg = ''
        msg += '-----------------------------------------------------\t\n'
        msg += 'Total Models Available: \t' + str(len(models)) + '\t\n'
        msg += '-----------------------------------------------------\t\n'
        for model_type in model_count.keys():
            msg += model_type.capitalize() + ' Models: \t' + str(model_count[model_type]) + '\t\n'
        msg += '-----------------------------------------------------\t\n'

        if showInfo:
            qt.QMessageBox.information(slicer.util.mainWindow(), 'NVIDIA AIAA', msg)
        logging.debug(msg)
        logging.info("Time consumed by fetchAIAAModels: {0:3.1f}".format(time.time() - start))

    def updateSegmentationMask(self, extreme_points, in_file, modelInfo, overwriteCurrentSegment=False,
                               sliceIndex=None):
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

        addedSegmentIds = [segmentation.GetNthSegmentID(numberOfExistingSegments + i)
                           for i in range(numberOfAddedSegments)]
        for i, segmentId in enumerate(addedSegmentIds):
            segment = segmentation.GetSegment(segmentId)
            if i == 0 and overwriteCurrentSegment and currentSegment:
                logging.debug('Update current segment with id: {} => {}'.format(segmentId, segment.GetName()))

                # Copy labelmap representation to the current segment then remove the imported segment
                labelmap = slicer.vtkOrientedImageData()
                segmentationNode.GetBinaryLabelmapRepresentation(segmentId, labelmap)

                # TODO:: May be there is a better option (faster than this)
                if sliceIndex:
                    selectedSegmentLabelmap = self.scriptedEffect.selectedSegmentLabelmap()
                    dims = selectedSegmentLabelmap.GetDimensions()
                    count = 0

                    for i in range(dims[0]):
                        for j in range(dims[1]):
                            v = selectedSegmentLabelmap.GetScalarComponentAsDouble(i, j, sliceIndex, 0)
                            if v:
                                count = count + 1
                            selectedSegmentLabelmap.SetScalarComponentFromDouble(i, j, sliceIndex, 0, 0)

                    logging.debug('Total Non Zero: {}'.format(count))

                    # Clear the Slice
                    if count:
                        self.scriptedEffect.modifySelectedSegmentByLabelmap(
                            selectedSegmentLabelmap,
                            slicer.qSlicerSegmentEditorAbstractEffect.ModificationModeSet)

                    # Union LableMap
                    self.scriptedEffect.modifySelectedSegmentByLabelmap(
                        labelmap,
                        slicer.qSlicerSegmentEditorAbstractEffect.ModificationModeAdd)
                else:
                    self.scriptedEffect.modifySelectedSegmentByLabelmap(
                        labelmap,
                        slicer.qSlicerSegmentEditorAbstractEffect.ModificationModeSet)

                segmentationNode.RemoveSegment(segmentId)
            else:
                logging.debug('Setting new segmentation with id: {} => {}'.format(segmentId, segment.GetName()))
                if i < len(modelLabels):
                    segment.SetName(modelLabels[i])
                else:
                    # we did not get enough labels (for example annotation_mri_prostate_cg_and_pz model
                    # returns a labelmap with
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
        return slicer.util.confirmOkCancelDisplay(
            "Master volume - without any additional patient information -"
            " will be sent to remote data processing server: {0}.\n\n"
            "Click 'OK' to proceed with the segmentation.\n"
            "Click 'Cancel' to not upload any data and cancel segmentation.\n".format(self.serverUrl()),
            dontShowAgainSettingsKey="NVIDIA-AIAA/showImageDataSendWarning")

    def closeAiaaSession(self):
        inputVolume = self.scriptedEffect.parameterSetNode().GetMasterVolumeNode()
        self.logic.closeSession(inputVolume)

    def createAiaaSessionIfNotExists(self):
        operationDescription = 'Please wait while uploading the volume to AIAA Server'
        logging.debug(operationDescription)

        self.updateServerSettings()
        inputVolume = self.scriptedEffect.parameterSetNode().GetMasterVolumeNode()

        in_file, session_id = self.logic.getSession(inputVolume)
        if in_file or session_id:
            return in_file, session_id

        if not self.getPermissionForImageDataUpload():
            return None, None

        try:
            qt.QApplication.setOverrideCursor(qt.Qt.WaitCursor)
            self.setProgressBarLabelText(operationDescription)
            slicer.app.processEvents()

            in_file, session_id = self.logic.createSession(inputVolume)
            qt.QApplication.restoreOverrideCursor()
            self.progressBar.hide()
        except:
            qt.QApplication.restoreOverrideCursor()
            self.progressBar.hide()
            slicer.util.errorDisplay(operationDescription + " - unexpected error.", detailedText=traceback.format_exc())
            return None, None

        return in_file, session_id

    def onClickSegmentation(self):
        in_file, session_id = self.createAiaaSessionIfNotExists()
        if in_file is None and session_id is None:
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
            extreme_points, result_file = self.logic.segmentation(in_file, session_id, model)

            if self.updateSegmentationMask(extreme_points, result_file, modelInfo):
                result = 'SUCCESS'
                self.updateGUIFromMRML()
                self.onClickEditAnnotationFiducialPoints()
        except AIAAException as ae:
            logging.exception("AIAA Exception")
            if ae.error == AIAAError.SESSION_EXPIRED:
                self.closeAiaaSession()
                slicer.util.warningDisplay(operationDescription + " - session expired.  Retry Again!")
            else:
                slicer.util.errorDisplay(operationDescription + " - " + ae.msg, detailedText=traceback.format_exc())
        except:
            logging.exception("Unknown Exception")
            slicer.util.errorDisplay(operationDescription + " - unexpected error.", detailedText=traceback.format_exc())
        finally:
            qt.QApplication.restoreOverrideCursor()
            self.progressBar.hide()

        msg = 'Run segmentation for ({0}): {1}\t\n' \
              'Time Consumed: {2:3.1f} (sec)'.format(model, result, (time.time() - start))
        logging.info(msg)

    def getFiducialPointsXYZ(self, fiducialNode):
        v = self.scriptedEffect.parameterSetNode().GetMasterVolumeNode()
        RasToIjkMatrix = vtk.vtkMatrix4x4()
        v.GetRASToIJKMatrix(RasToIjkMatrix)

        point_set = []
        n = fiducialNode.GetNumberOfFiducials()
        for i in range(n):
            coord = [0.0, 0.0, 0.0]
            fiducialNode.GetNthFiducialPosition(i, coord)

            world = [0, 0, 0, 0]
            fiducialNode.GetNthFiducialWorldCoordinates(i, world)

            p_Ras = [coord[0], coord[1], coord[2], 1.0]
            p_Ijk = RasToIjkMatrix.MultiplyDoublePoint(p_Ras)
            p_Ijk = [round(i) for i in p_Ijk]

            logging.debug('RAS: {}; WORLD: {}; IJK: '.format(coord, world, p_Ijk))
            point_set.append(p_Ijk[0:3])

        logging.info('Current Fiducials-Points: {}'.format(point_set))
        return point_set

    def getFiducialPointXYZ(self, fiducialNode, index):
        v = self.scriptedEffect.parameterSetNode().GetMasterVolumeNode()
        RasToIjkMatrix = vtk.vtkMatrix4x4()
        v.GetRASToIJKMatrix(RasToIjkMatrix)

        coord = [0.0, 0.0, 0.0]
        fiducialNode.GetNthFiducialPosition(index, coord)

        world = [0, 0, 0, 0]
        fiducialNode.GetNthFiducialWorldCoordinates(index, world)

        p_Ras = [coord[0], coord[1], coord[2], 1.0]
        p_Ijk = RasToIjkMatrix.MultiplyDoublePoint(p_Ras)
        p_Ijk = [round(i) for i in p_Ijk]

        logging.debug('RAS: {}; WORLD: {}; IJK: '.format(coord, world, p_Ijk))
        return p_Ijk[0:3]

    def onClickAnnotation(self):
        in_file, session_id = self.createAiaaSessionIfNotExists()
        if in_file is None and session_id is None:
            return

        start = time.time()

        model = self.ui.annotationModelSelector.currentText
        label = self.currentSegment().GetName()
        operationDescription = 'Run Annotation for model: {} for segment: {}'.format(model, label)
        logging.debug(operationDescription)

        result = 'FAILED'
        try:
            qt.QApplication.setOverrideCursor(qt.Qt.WaitCursor)
            pointSet = self.getFiducialPointsXYZ(self.annotationFiducialNode)
            result_file = self.logic.dextr3d(in_file, session_id, model, pointSet)

            if self.updateSegmentationMask(pointSet, result_file, None, overwriteCurrentSegment=True):
                result = 'SUCCESS'
                self.updateGUIFromMRML()
        except AIAAException as ae:
            logging.exception("AIAA Exception")
            if ae.error == AIAAError.SESSION_EXPIRED:
                self.closeAiaaSession()
                slicer.util.warningDisplay(operationDescription + " - session expired.  Retry Again!")
            else:
                logging.exception("Unknown Exception")
                slicer.util.errorDisplay(operationDescription + " - " + ae.msg, detailedText=traceback.format_exc())
        except:
            slicer.util.errorDisplay(operationDescription + " - unexpected error.", detailedText=traceback.format_exc())
        finally:
            qt.QApplication.restoreOverrideCursor()

        msg = 'Run annotation for ({0}): {1}\t\n' \
              'Time Consumed: {2:3.1f} (sec)'.format(model, result, (time.time() - start))
        logging.info(msg)

    def onClickDeepgrow(self, current_point):
        segment = self.currentSegment()
        if not segment:
            slicer.util.warningDisplay("Please add/select a segment to run deepgrow")
            return

        in_file, session_id = self.createAiaaSessionIfNotExists()
        if in_file is None and session_id is None:
            return

        start = time.time()

        model = self.ui.deepgrowModelSelector.currentText
        if not model:
            slicer.util.warningDisplay("Please select a deepgrow model")
            return

        label = self.currentSegment().GetName()
        operationDescription = 'Run Deepgrow for segment: {}; model: {}'.format(label, model)
        logging.debug(operationDescription)

        try:
            qt.QApplication.setOverrideCursor(qt.Qt.WaitCursor)

            foreground_all = self.getFiducialPointsXYZ(self.dgPositiveFiducialNode)
            background_all = self.getFiducialPointsXYZ(self.dgNegativeFiducialNode)

            segment.SetTag("AIAA.ForegroundPoints", json.dumps(foreground_all))
            segment.SetTag("AIAA.BackgroundPoints", json.dumps(background_all))

            sliceIndex = current_point[2]
            logging.debug('Slice Index: {}'.format(sliceIndex))

            foreground = [x for x in foreground_all if x[2] == sliceIndex]
            background = [x for x in background_all if x[2] == sliceIndex]

            logging.debug('Foreground: {}'.format(foreground))
            logging.debug('Background: {}'.format(background))

            result_file = self.logic.deepgrow(in_file, session_id, model, foreground, background)
            result = 'FAILED'

            if self.updateSegmentationMask(None, result_file, None,
                                           overwriteCurrentSegment=True,
                                           sliceIndex=sliceIndex):
                result = 'SUCCESS'
                self.updateGUIFromMRML()
        except AIAAException as ae:
            logging.exception("AIAA Exception")
            if ae.error == AIAAError.SERVER_ERROR:
                self.closeAiaaSession()
                slicer.util.warningDisplay(operationDescription + " - session expired.  Retry Again!")
            else:
                slicer.util.errorDisplay(operationDescription + " - " + ae.msg, detailedText=traceback.format_exc())
        except:
            logging.exception("Unknown Exception")
            slicer.util.errorDisplay(operationDescription + " - unexpected error.", detailedText=traceback.format_exc())
        finally:
            qt.QApplication.restoreOverrideCursor()

        msg = 'Run deepgrow for ({0}): {1}\t\n' \
              'Time Consumed: {2:3.1f} (sec)'.format(label, result, (time.time() - start))
        logging.info(msg)

    def onClickEditAnnotationFiducialPoints(self):
        self.onEditFiducialPoints(self.annotationFiducialNode, "AIAA.DExtr3DExtremePoints")

    def onEditFiducialPoints(self, fiducialNode, tagName):
        segment = self.currentSegment()
        segmentId = self.currentSegmentID()
        logging.debug('Current SegmentID: {}; Segment: {}'.format(segmentId, segment))

        if fiducialNode:
            fiducialNode.RemoveAllMarkups()

            if segment and segmentId:
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
        self.resetFiducial(self.ui.annotationFiducialPlacementWidget,
                           self.annotationFiducialNode,
                           self.annotationFiducialNodeObservers)
        self.annotationFiducialNode = None
        self.resetFiducial(self.ui.dgPositiveFiducialPlacementWidget,
                           self.dgPositiveFiducialNode,
                           self.dgPositiveFiducialNodeObservers)
        self.dgPositiveFiducialNode = None
        self.resetFiducial(self.ui.dgNegativeFiducialPlacementWidget,
                           self.dgNegativeFiducialNode,
                           self.dgNegativeFiducialNodeObservers)
        self.dgNegativeFiducialNode = None

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
            self.logic.setProgressCallback(
                progress_callback=lambda progressPercentage: self.reportProgress(progressPercentage))

        self.isActivated = True
        self.scriptedEffect.showEffectCursorInSliceView = False

        self.updateServerUrlGUIFromSettings()

        # Create empty markup fiducial node
        if not self.annotationFiducialNode:
            self.annotationFiducialNode, self.annotationFiducialNodeObservers = self.createFiducialNode(
                'A',
                self.onAnnotationFiducialNodeModified,
                [1, 0.5, 0.5])
            self.ui.annotationFiducialPlacementWidget.setCurrentNode(self.annotationFiducialNode)
            self.ui.annotationFiducialPlacementWidget.setPlaceModeEnabled(False)

        # Create empty markup fiducial node for deep grow +ve and -ve
        if not self.dgPositiveFiducialNode:
            self.dgPositiveFiducialNode, self.dgPositiveFiducialNodeObservers = self.createFiducialNode(
                'P',
                self.onDeepGrowFiducialNodeModified,
                [0.5, 1, 0.5])
            self.ui.dgPositiveFiducialPlacementWidget.setCurrentNode(self.dgPositiveFiducialNode)
            self.ui.dgPositiveFiducialPlacementWidget.setPlaceModeEnabled(False)

        if not self.dgNegativeFiducialNode:
            self.dgNegativeFiducialNode, self.dgNegativeFiducialNodeObservers = self.createFiducialNode(
                'N',
                self.onDeepGrowFiducialNodeModified,
                [0.5, 0.5, 1])
            self.ui.dgNegativeFiducialPlacementWidget.setCurrentNode(self.dgNegativeFiducialNode)
            self.ui.dgNegativeFiducialPlacementWidget.setPlaceModeEnabled(False)

        self.updateGUIFromMRML()
        self.saveServerUrl()
        self.fetchAIAAModels()

        self.observeParameterNode(True)
        self.observeSegmentation(True)

    def deactivate(self):
        logging.debug('NVidia AIAA effect deactivated')
        self.isActivated = False
        self.observeSegmentation(False)
        self.observeParameterNode(False)
        self.reset()

        # TODO:: Call this on Exit event
        # self.logic.closeAllSessions()

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
                self.parameterSetNodeObserverTags.append(
                    self.observedParameterSetNode.AddObserver(eventId, self.onParameterSetNodeModified))

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
                self.segmentationNodeObserverTags.append(
                    self.observedSegmentation.AddObserver(eventId, self.onSegmentationModified))

    def onParameterSetNodeModified(self, caller, event):
        logging.debug("Parameter Node Modified: {}".format(event))
        if self.isActivated:
            self.fetchAIAAModels()

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

    def updateSelector(self, selector, model_type, param, filtered, defaultIndex=0):
        wasSelectorBlocked = selector.blockSignals(True)
        selector.clear()

        currentSegment = self.currentSegment()
        currentSegmentName = currentSegment.GetName().lower() if currentSegment else ""

        for model_name, model in self.models.items():
            if model['type'] == model_type:
                if filtered and not (currentSegmentName in model_name.lower()):
                    continue
                selector.addItem(model_name)
                selector.setItemData(selector.count - 1, model['description'], qt.Qt.ToolTipRole)

        model = self.scriptedEffect.parameter(param) if self.scriptedEffect.parameterDefined(param) else ""
        if not model and currentSegment:
            model = vtk.mutable("")
            currentSegment.GetTag(param, model)

        modelIndex = selector.findText(model)
        modelIndex = defaultIndex if modelIndex < 0 < selector.count else modelIndex
        selector.setCurrentIndex(modelIndex)

        try:
            modelInfo = self.models[model]
            selector.setToolTip(modelInfo["description"])
        except:
            selector.setToolTip("")
        selector.blockSignals(wasSelectorBlocked)

    def updateGUIFromMRML(self):
        annotationModelFiltered = self.scriptedEffect.integerParameter("AnnotationModelFiltered") != 0
        wasBlocked = self.ui.annotationModelFilterPushButton.blockSignals(True)
        self.ui.annotationModelFilterPushButton.checked = annotationModelFiltered
        self.ui.annotationModelFilterPushButton.blockSignals(wasBlocked)

        self.updateSelector(self.ui.segmentationModelSelector, 'segmentation', 'SegmentationModel', False, 0)
        self.updateSelector(self.ui.annotationModelSelector, 'annotation', 'AIAA.AnnotationModel',
                            annotationModelFiltered, -1)
        self.updateSelector(self.ui.deepgrowModelSelector, 'deepgrow', 'DeepgrowModel', False, 0)

        # Enable/Disable
        self.ui.segmentationButton.setEnabled(self.ui.segmentationModelSelector.currentText)

        currentSegment = self.currentSegment()
        if currentSegment:
            self.ui.dgPositiveFiducialPlacementWidget.setEnabled(self.ui.deepgrowModelSelector.currentText)
            self.ui.dgNegativeFiducialPlacementWidget.setEnabled(self.ui.deepgrowModelSelector.currentText)

        if currentSegment and self.annotationFiducialNode and self.ui.annotationModelSelector.currentText:
            numberOfDefinedPoints = self.annotationFiducialNode.GetNumberOfDefinedControlPoints()
            if numberOfDefinedPoints >= 6:
                self.ui.annotationButton.setEnabled(True)
                self.ui.annotationButton.setToolTip("Segment the object based on specified boundary points")
            else:
                self.ui.annotationButton.setEnabled(False)
                self.ui.annotationButton.setToolTip(
                    "Not enough points. Place at least 6 points near the boundaries of the object (one or more on each side).")
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

        deepgrowModelIndex = self.ui.deepgrowModelSelector.currentIndex
        if deepgrowModelIndex >= 0:
            deepgrowModel = self.ui.deepgrowModelSelector.itemText(deepgrowModelIndex)
            self.scriptedEffect.setParameter("DeepgrowModel", deepgrowModel)

        annotationModelIndex = self.ui.annotationModelSelector.currentIndex
        if annotationModelIndex >= 0:
            # Only overwrite annotation model in MRML node if there is a valid selection
            # (to not clear the model if it is temporarily not available)
            annotationModel = self.ui.annotationModelSelector.itemText(annotationModelIndex)
            currentSegment = self.currentSegment()
            if currentSegment:
                currentSegment.SetTag("AIAA.AnnotationModel", annotationModel)

        self.scriptedEffect.setParameter("AnnotationModelFiltered",
                                         1 if self.ui.annotationModelFilterPushButton.checked else 0)
        self.scriptedEffect.parameterSetNode().EndModify(wasModified)

    def createFiducialNode(self, name, onMarkupNodeModified, color):
        displayNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLMarkupsDisplayNode")
        displayNode.SetTextScale(0)
        displayNode.SetSelectedColor(color)

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
    def __init__(self, server_url=None, progress_callback=None):

        self.aiaa_tmpdir = slicer.util.tempDirectory('slicer-aiaa')
        self.volumeToAiaaSessions = dict()
        self.progress_callback = progress_callback

        self.server_url = server_url
        self.useCompression = True
        self.useSession = False

    def __del__(self):
        shutil.rmtree(self.aiaa_tmpdir, ignore_errors=True)

    def inputFileExtension(self):
        return ".nii.gz" if self.useCompression else ".nii"

    def outputFileExtension(self):
        return ".nii.gz"

    def setServer(self, server_url=None):
        if not server_url:
            server_url = "http://skull.cs.queensu.ca:8123"
        self.server_url = server_url

    def setUseCompression(self, useCompression):
        self.useCompression = useCompression

    def setUseSession(self, useSession):
        self.useSession = useSession

    def setProgressCallback(self, progress_callback=None):
        self.progress_callback = progress_callback

    def reportProgress(self, progress):
        if self.progress_callback:
            self.progress_callback(progress)

    def getSession(self, inputVolume):
        t = self.volumeToAiaaSessions.get(self.nodeCacheKey(inputVolume))
        if t:
            in_file = t[0]
            session_id = t[1]
            server_url = t[2]

            if not self.useSession:
                return in_file, session_id

            parsed_uri1 = urlparse(server_url)
            result1 = '{uri.scheme}://{uri.netloc}/'.format(uri=parsed_uri1)

            parsed_uri2 = urlparse(self.server_url)
            result2 = '{uri.scheme}://{uri.netloc}/'.format(uri=parsed_uri2)
            logging.debug("Compare URL-1: {} v/s URL-2: {}".format(result1, result2))

            if result1 == result2 and session_id:
                logging.debug('Session already exists; session-id: {}'.format(session_id))
                return in_file, session_id

            logging.info('Close Mismatched Session; url {} => {}'.format(result1, result2))
            self.closeSession(inputVolume)
        return None, None

    def closeSession(self, inputVolume):
        t = self.volumeToAiaaSessions.get(self.nodeCacheKey(inputVolume))
        if t:
            session_id = t[1]
            server_url = t[2]

            if self.useSession:
                aiaaClient = AIAAClient(server_url)
                aiaaClient.close_session(session_id)
            self.volumeToAiaaSessions.pop(self.nodeCacheKey(inputVolume))

    def createSession(self, inputVolume):
        t = self.volumeToAiaaSessions.get(self.nodeCacheKey(inputVolume))
        if t is None or t[0] is None:
            in_file = tempfile.NamedTemporaryFile(suffix=self.inputFileExtension(), dir=self.aiaa_tmpdir).name
            self.reportProgress(5)
            start = time.time()
            slicer.util.saveNode(inputVolume, in_file)

            logging.info('Saved Input Node into {0} in {1:3.1f}s'.format(in_file, time.time() - start))
        else:
            in_file = t[0]
        self.reportProgress(30)

        session_id = None
        if self.useSession:
            aiaaClient = AIAAClient(self.server_url)
            response = aiaaClient.create_session(in_file)
            logging.info('AIAA Session Response Json: {}'.format(response))

            session_id = response.get('session_id')
            logging.info('Created AIAA session ({0}) in {1:3.1f}s'.format(session_id, time.time() - start))

        self.volumeToAiaaSessions[self.nodeCacheKey(inputVolume)] = (in_file, session_id, self.server_url)
        self.reportProgress(100)
        return in_file, session_id

    def closeAllSessions(self):
        for k in self.volumeToAiaaSessions.keys():
            t = self.volumeToAiaaSessions[k]
            in_file = t[0]
            session_id = t[1]
            server_url = t[2]

            if self.useSession:
                aiaaClient = AIAAClient(server_url)
                aiaaClient.close_session(session_id)

            if os.path.exists(in_file):
                os.unlink(in_file)
        self.volumeToAiaaSessions.clear()

    def list_models(self, label=None):
        logging.debug('Fetching List of Models for label: {}'.format(label))
        aiaaClient = AIAAClient(self.server_url)
        return aiaaClient.model_list(label)

    def nodeCacheKey(self, mrmlNode):
        return mrmlNode.GetID() + "*" + str(mrmlNode.GetMTime())

    def segmentation(self, image_in, session_id, model):
        logging.debug('Preparing input data for segmentation')
        self.reportProgress(0)

        result_file = tempfile.NamedTemporaryFile(suffix=self.outputFileExtension(), dir=self.aiaa_tmpdir).name
        aiaaClient = AIAAClient(self.server_url)
        params = aiaaClient.segmentation(model, image_in, result_file, session_id=session_id)

        extreme_points = params.get('points', params.get('extreme_points'))
        logging.debug('Extreme Points: {}'.format(extreme_points))

        self.reportProgress(100)
        return extreme_points, result_file

    def dextr3d(self, image_in, session_id, model, pointset):
        logging.debug('Preparing for Annotation/Dextr3D Action')

        result_file = tempfile.NamedTemporaryFile(suffix=self.outputFileExtension(), dir=self.aiaa_tmpdir).name
        aiaaClient = AIAAClient(self.server_url)
        aiaaClient.dextr3d(model, pointset, image_in, result_file,
                           pre_process=(not self.useSession),
                           session_id=session_id)
        return result_file

    def deepgrow(self, image_in, session_id, model, foreground_point_set, background_point_set):
        logging.debug('Preparing for Deepgrow Action (model: {})'.format(model))

        result_file = tempfile.NamedTemporaryFile(suffix=self.outputFileExtension(), dir=self.aiaa_tmpdir).name
        aiaaClient = AIAAClient(self.server_url)
        aiaaClient.deepgrow(model, foreground_point_set, background_point_set, image_in, result_file,
                            session_id=session_id)
        return result_file
