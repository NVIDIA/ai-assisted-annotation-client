import atexit
import json
import logging
import os
import shutil
import tempfile
import time

import SimpleITK as sitk
import qt
import sitkUtils
import slicer
import vtk
from SegmentEditorEffects import *

from AIAAClient import AIAAClient


class SegmentEditorEffect(AbstractScriptedSegmentEditorEffect):
    """This effect uses Watershed algorithm to partition the input volume"""

    def __init__(self, scriptedEffect):
        scriptedEffect.name = 'Nvidia AIAA'
        scriptedEffect.perSegment = False  # this effect operates on all segments at once (not on a single selected segment)
        try:
            # this effect can create its own segments, so we do not require any pre-existing segment
            scriptedEffect.requireSegments = False
        except AttributeError:
            # requireSegments option is not available in this Slicer version,
            # therefore the user has to create a segment to make the effect enabled
            pass
        AbstractScriptedSegmentEditorEffect.__init__(self, scriptedEffect)

        self.extremePoints = dict()
        self.models = dict()

        # Effect-specific members
        self.segmentMarkupNode = None
        self.segmentMarkupNodeObservers = []

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
        return """<html>Use NVIDIA AI-Assisted features for segmentation and annotation.<br/> 
            (Click Edit &rarr; Application Settings &rarr; <b>Nvidia</b> for related settings)</html>"""

    def fetchSettings(self):
        settings = qt.QSettings()
        self.serverUrl = settings.value("NVIDIA-AIAA/serverUrl", "http://0.0.0.0:5000")
        self.filterByLabel = True if int(settings.value("NVIDIA-AIAA/filterByLabel", "1")) > 0 else False

    def setupOptionsFrame(self):
        nvidiaFormLayout = qt.QFormLayout()

        ##################################################
        # AIAA Server + Fetch/Refresh Models
        nvidiaFormLayout.addRow(qt.QLabel())
        aiaaGroupBox = qt.QGroupBox("AI-Assisted Annotation Server: ")
        aiaaGroupLayout = qt.QFormLayout(aiaaGroupBox)

        self.modelsButton = qt.QPushButton("Fetch Models")

        aiaaGroupLayout.addRow(self.modelsButton)

        aiaaGroupBox.setLayout(aiaaGroupLayout)
        nvidiaFormLayout.addRow(aiaaGroupBox)
        ##################################################

        ##################################################
        # Segmentation
        nvidiaFormLayout.addRow(qt.QLabel())
        segGroupBox = qt.QGroupBox("Auto Segmentation: ")
        segGroupLayout = qt.QFormLayout(segGroupBox)
        self.segmentationModelSelector = qt.QComboBox()
        self.segmentationButton = qt.QPushButton(" Run Segmentation")
        self.segmentationButton.setIcon(self.icon('nvidia-icon.png'))
        self.segmentationButton.setEnabled(False)

        # self.segmentationModelSelector.addItems(['segmentation_ct_spleen', 'segmentation_ct_liver_and_tumor'])
        self.segmentationModelSelector.setToolTip("Pick the input Segmentation model")

        segGroupLayout.addRow("Model: ", self.segmentationModelSelector)
        segGroupLayout.addRow(self.segmentationButton)

        segGroupBox.setLayout(segGroupLayout)
        ##################################################

        ##################################################
        # Annotation
        annGroupBox = qt.QGroupBox("Annotation: ")
        annGroupLayout = qt.QFormLayout(annGroupBox)

        # Fiducial Placement widget
        self.fiducialPlacementToggle = slicer.qSlicerMarkupsPlaceWidget()
        self.fiducialPlacementToggle.setMRMLScene(slicer.mrmlScene)
        self.fiducialPlacementToggle.placeMultipleMarkups = self.fiducialPlacementToggle.ForcePlaceMultipleMarkups
        self.fiducialPlacementToggle.buttonsVisible = False
        self.fiducialPlacementToggle.show()
        self.fiducialPlacementToggle.placeButton().show()
        self.fiducialPlacementToggle.deleteButton().show()

        # Edit surface button
        self.annoEditButton = qt.QPushButton()
        self.annoEditButton.setIcon(self.icon('edit-icon.png'))
        self.annoEditButton.objectName = self.__class__.__name__ + 'Edit'
        self.annoEditButton.setToolTip("Edit the previously placed group of fiducials.")
        self.annoEditButton.setEnabled(False)

        fiducialActionLayout = qt.QHBoxLayout()
        fiducialActionLayout.addWidget(self.fiducialPlacementToggle)
        fiducialActionLayout.addWidget(self.annoEditButton)

        self.annotationModelSelector = qt.QComboBox()
        self.dextr3DButton = qt.QPushButton(" Run DExtr3D")
        self.dextr3DButton.setIcon(self.icon('nvidia-icon.png'))
        self.dextr3DButton.setEnabled(False)

        # self.annotationModelSelector.addItems(['annotation_ct_spleen'])
        self.annotationModelSelector.setToolTip("Pick the input Annotation model")

        annGroupLayout.addRow("Model: ", self.annotationModelSelector)
        annGroupLayout.addRow(fiducialActionLayout)
        annGroupLayout.addRow(self.dextr3DButton)

        annGroupBox.setLayout(annGroupLayout)
        ##################################################
        aiaaAction = qt.QHBoxLayout()
        aiaaAction.addWidget(segGroupBox)
        aiaaAction.addWidget(annGroupBox)
        nvidiaFormLayout.addRow(aiaaAction)

        self.scriptedEffect.addOptionsWidget(nvidiaFormLayout)

        ##################################################
        # connections
        self.modelsButton.connect('clicked(bool)', self.onClickModels)
        self.segmentationButton.connect('clicked(bool)', self.onClickSegmentation)
        self.annoEditButton.connect('clicked(bool)', self.onClickEditPoints)
        self.fiducialPlacementToggle.placeButton().clicked.connect(self.onFiducialPlacementToggleChanged)
        self.dextr3DButton.connect('clicked(bool)', self.onClickAnnotation)
        ##################################################

    def getActiveLabel(self):
        node = self.scriptedEffect.parameterSetNode().GetSegmentationNode()
        id = self.scriptedEffect.parameterSetNode().GetSelectedSegmentID()
        name = node.GetSegmentation().GetSegment(id).GetName()
        return name

    def onClickModels(self):
        start = time.time()
        progressBar = slicer.util.createProgressDialog(windowTitle="Wait...", labelText="Fetching models", maximum=100)
        slicer.app.processEvents()
        try:
            self.fetchSettings()
            logic = AIAALogic(self.serverUrl,
                              progress_callback=lambda progressPercentage,
                                                       progressBar=progressBar: SegmentEditorEffect.report_progress(
                                  progressBar, progressPercentage))
            label = self.getActiveLabel()
            logging.info('Active Label: {}'.format(label))
            models = logic.list_models(label if self.filterByLabel else None)
        except:
            progressBar.close()
            slicer.util.errorDisplay(
                "Failed to fetch models from remote server. Make sure server address is correct and retry.",
                detailedText=traceback.format_exc())
            return
        else:
            progressBar.close()

        self.segmentationModelSelector.clear()
        self.annotationModelSelector.clear()
        for model in models:
            model_name = model['name']
            model_type = model['type']
            logging.info('{} = {}'.format(model_name, model_type))
            self.models[model_name] = model

            if model_type == 'segmentation':
                self.segmentationModelSelector.addItem(model_name)
            else:
                self.annotationModelSelector.addItem(model_name)

        self.segmentationButton.enabled = self.segmentationModelSelector.count > 0
        self.updateGUIFromMRML()

        msg = ''
        msg += '-----------------------------------------------------\t\n'
        if self.filterByLabel:
            msg += 'Using Label: \t\t' + label + '\t\n'
        msg += 'Total Models Loaded: \t' + str(len(models)) + '\t\n'
        msg += '-----------------------------------------------------\t\n'
        msg += 'Segmentation Models: \t' + str(self.segmentationModelSelector.count) + '\t\n'
        msg += 'Annotation Models: \t' + str(self.annotationModelSelector.count) + '\t\n'
        msg += '-----------------------------------------------------\t\n'
        # qt.QMessageBox.information(slicer.util.mainWindow(), 'NVIDIA AIAA', msg)
        logging.info(msg)
        logging.info("++ Time consumed by onClickModels: {}".format(time.time() - start))

    def updateSegmentationMask(self, extreme_points, in_file, modelInfo):
        start = time.time()
        logging.info('Update Segmentation Mask from: {}'.format(in_file))
        if in_file is None:
            return False

        segmentationNode = self.scriptedEffect.parameterSetNode().GetSegmentationNode()
        selectedSegmentId = self.scriptedEffect.parameterSetNode().GetSelectedSegmentID()

        segmentation = segmentationNode.GetSegmentation()
        segment = segmentation.GetSegment(selectedSegmentId)
        color = segment.GetColor()
        label = segment.GetName()

        labelImage = sitk.ReadImage(in_file)
        labelmapVolumeNode = sitkUtils.PushVolumeToSlicer(labelImage, None, className='vtkMRMLLabelMapVolumeNode')
        labelmapVolumeNode.SetName(label)
        # [success, labelmapVolumeNode] = slicer.util.loadLabelVolume(in_file, {'name': label}, returnNode=True)

        logging.info('Removing temp segmentation with id: {} with color: {}'.format(selectedSegmentId, color))
        segmentationNode.RemoveSegment(selectedSegmentId)

        originalSegments = dict()
        for i in range(segmentation.GetNumberOfSegments()):
            segmentId = segmentation.GetNthSegmentID(i)
            originalSegments[segmentId] = i

        slicer.modules.segmentations.logic().ImportLabelmapToSegmentationNode(labelmapVolumeNode, segmentationNode)
        slicer.mrmlScene.RemoveNode(labelmapVolumeNode)

        addedSegments = 0
        modelLabels = modelInfo.get('labels')
        for i in range(segmentation.GetNumberOfSegments()):
            segmentId = segmentation.GetNthSegmentID(i)
            segment = segmentation.GetSegment(segmentId)
            if originalSegments.get(segmentId) is not None:
                logging.info('No change for existing segment with id: {} => {}'.format(segmentId, segment.GetName()))
                continue

            logging.info('Setting new segmentation with id: {} => {}'.format(segmentId, segment.GetName()))
            if addedSegments == 0:
                segment.SetColor(color)
                segment.SetName(label)

                self.scriptedEffect.parameterSetNode().SetSelectedSegmentID(segmentId)
                logging.info('Extreme Points: {}'.format(extreme_points))
                if extreme_points is not None:
                    segment.SetTag("DExtr3DExtremePoints", json.dumps(extreme_points))

            else:
                segment.SetName(modelLabels[addedSegments])
            addedSegments = addedSegments + 1
        logging.info('Total Added Segments for {}: {}'.format(label, addedSegments))

        self.extremePoints[label] = extreme_points
        os.unlink(in_file)
        logging.info("++ Time consumed by updateSegmentationMask: {}".format(time.time() - start))
        return True

    @staticmethod
    def report_progress(progressBar, progressPercentage):
        progressBar.show()
        progressBar.activateWindow()
        progressBar.setValue(progressPercentage)
        slicer.app.processEvents()

    def onClickSegmentation(self):
        start = time.time()
        model = self.segmentationModelSelector.currentText
        label = self.getActiveLabel()
        modelInfo = self.models.get(model)

        operationDescription = 'Run Segmentation for model: {} for label: {}'.format(model, label)
        logging.info(operationDescription)
        progressBar = slicer.util.createProgressDialog(windowTitle="Wait...", labelText=operationDescription,
                                                       maximum=100)
        slicer.app.processEvents()
        try:
            qt.QApplication.setOverrideCursor(qt.Qt.WaitCursor)
            self.fetchSettings()
            logic = AIAALogic(self.serverUrl,
                              progress_callback=lambda progressPercentage,
                                                       progressBar=progressBar: SegmentEditorEffect.report_progress(
                                  progressBar, progressPercentage))

            inputVolume = self.scriptedEffect.parameterSetNode().GetMasterVolumeNode()
            extreme_points, result_file = logic.segmentation(model, inputVolume)
            if self.updateSegmentationMask(extreme_points, result_file, modelInfo):
                result = 'SUCCESS'
                self.segmentMarkupNode.RemoveAllMarkups()
                self.updateGUIFromMRML()
        except:
            qt.QApplication.restoreOverrideCursor()
            progressBar.close()
            slicer.util.errorDisplay(operationDescription + " - unexpected error.", detailedText=traceback.format_exc())
            return

        qt.QApplication.restoreOverrideCursor()
        progressBar.close()

        logging.info("++ Time consumed by onClickSegmentation: {}".format(time.time() - start))
        msg = 'Run segmentation for ({}): {}\t\nTime Consumed: {} (sec)'.format(model, result, (time.time() - start))
        logging.info(msg)
        qt.QMessageBox.information(slicer.util.mainWindow(), 'NVIDIA AIAA', msg)

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

            logging.info('From Fiducial: {} => {}'.format(coord, p_Ijk))
            point_set.append(p_Ijk[0:3])

        logging.info('Current Fiducials-Points: {}'.format(point_set))
        return point_set

    def onClickAnnotation(self):
        start = time.time()
        model = self.annotationModelSelector.currentText
        label = self.getActiveLabel()
        operationDescription = 'Run Annotation for model: {} for label: {}'.format(model, label)
        logging.info(operationDescription)
        progressBar = slicer.util.createProgressDialog(windowTitle="Wait...", labelText=operationDescription,
                                                       maximum=100)
        slicer.app.processEvents()
        try:
            qt.QApplication.setOverrideCursor(qt.Qt.WaitCursor)
            self.fetchSettings()
            logic = AIAALogic(self.serverUrl,
                              progress_callback=lambda progressPercentage,
                                                       progressBar=progressBar: SegmentEditorEffect.report_progress(
                                  progressBar, progressPercentage))

            inputVolume = self.scriptedEffect.parameterSetNode().GetMasterVolumeNode()
            modelInfo = self.models.get(model)
            pointSet = self.getFiducialPointsXYZ()

            result_file = logic.dextr3d(model, pointSet, inputVolume, modelInfo)
            result = 'FAILED'
            if self.updateSegmentationMask(pointSet, result_file, modelInfo):
                result = 'SUCCESS'
                self.updateGUIFromMRML()
        except:
            qt.QApplication.restoreOverrideCursor()
            progressBar.close()
            slicer.util.errorDisplay(operationDescription + " - unexpected error.", detailedText=traceback.format_exc())
            return

        qt.QApplication.restoreOverrideCursor()
        progressBar.close()

        logging.info("++ Time consumed by onClickAnnotation: {}".format(time.time() - start))
        msg = 'Run annotation for ({}): {}\t\nTime Consumed: {} (sec)'.format(model, result, (time.time() - start))
        logging.info(msg)
        qt.QMessageBox.information(slicer.util.mainWindow(), 'NVIDIA AIAA', msg)

    def onClickEditPoints(self):
        segmentID = self.scriptedEffect.parameterSetNode().GetSelectedSegmentID()
        segmentationNode = self.scriptedEffect.parameterSetNode().GetSegmentationNode()
        segment = segmentationNode.GetSegmentation().GetSegment(segmentID)
        label = segment.GetName()

        self.segmentMarkupNode.RemoveAllMarkups()

        v = self.scriptedEffect.parameterSetNode().GetMasterVolumeNode()
        IjkToRasMatrix = vtk.vtkMatrix4x4()
        v.GetIJKToRASMatrix(IjkToRasMatrix)

        fPosStr = vtk.mutable("")
        segment.GetTag("DExtr3DExtremePoints", fPosStr)
        pointset = str(fPosStr)
        logging.info('{} => {} Extreme points are: {}'.format(segmentID, label, pointset))

        if fPosStr is not None and len(pointset) > 0:
            points = json.loads(pointset)
            for p in points:
                p_Ijk = [p[0], p[1], p[2], 1.0]
                p_Ras = IjkToRasMatrix.MultiplyDoublePoint(p_Ijk)
                logging.info('Add Fiducial: {} => {}'.format(p_Ijk, p_Ras))
                self.segmentMarkupNode.AddFiducialFromArray(p_Ras[0:3])
        else:
            qt.QMessageBox.information(
                slicer.util.mainWindow(),
                'NVIDIA AIAA',
                'There are no pre-existing extreme points available for (' + segment.GetName() + ')' + '\t')
        self.updateGUIFromMRML()

    def reset(self):
        if self.fiducialPlacementToggle.placeModeEnabled:
            self.fiducialPlacementToggle.setPlaceModeEnabled(False)

        if self.segmentMarkupNode:
            slicer.mrmlScene.RemoveNode(self.segmentMarkupNode)
            self.setAndObserveSegmentMarkupNode(None)

    def activate(self):
        self.scriptedEffect.showEffectCursorInSliceView = False

        # Create empty markup fiducial node
        if not self.segmentMarkupNode:
            self.createNewMarkupNode()
            self.fiducialPlacementToggle.setCurrentNode(self.segmentMarkupNode)
            self.setAndObserveSegmentMarkupNode(self.segmentMarkupNode)
            self.fiducialPlacementToggle.setPlaceModeEnabled(False)

        self.updateGUIFromMRML()

    def deactivate(self):
        self.reset()

    def createCursor(self, widget):
        # Turn off effect-specific cursor for this effect
        return slicer.util.mainWindow().cursor

    def setMRMLDefaults(self):
        pass

    def updateGUIFromMRML(self):
        if self.segmentMarkupNode:
            self.dextr3DButton.setEnabled(self.segmentMarkupNode.GetNumberOfFiducials() >= 6)
        else:
            self.dextr3DButton.setEnabled(False)

        segmentID = self.scriptedEffect.parameterSetNode().GetSelectedSegmentID()
        segmentationNode = self.scriptedEffect.parameterSetNode().GetSegmentationNode()
        if segmentID and segmentationNode and self.annotationModelSelector.count > 0:
            segment = segmentationNode.GetSegmentation().GetSegment(segmentID)
            self.annoEditButton.setEnabled(True)
            self.annoEditButton.setVisible(segment.HasTag("DExtr3DExtremePoints"))

    def updateMRMLFromGUI(self):
        pass

    def onFiducialPlacementToggleChanged(self):
        if self.fiducialPlacementToggle.placeButton().isChecked():
            # Create empty markup fiducial node
            if self.segmentMarkupNode is None:
                self.createNewMarkupNode()
                self.fiducialPlacementToggle.setCurrentNode(self.segmentMarkupNode)

    def createNewMarkupNode(self):
        # Create empty markup fiducial node
        if self.segmentMarkupNode is None:
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
            if (slicer.app.majorVersion >= 5) or (slicer.app.majorVersion >= 4 and slicer.app.minorVersion >= 11):
                eventIds = [vtk.vtkCommand.ModifiedEvent,
                            slicer.vtkMRMLMarkupsNode.PointModifiedEvent,
                            slicer.vtkMRMLMarkupsNode.PointAddedEvent,
                            slicer.vtkMRMLMarkupsNode.PointRemovedEvent]
            else:
                eventIds = [vtk.vtkCommand.ModifiedEvent]
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


# Create Single AIAA Client Instance
aiaaClient = AIAAClient()
volumeToImageFiles = dict()

# TODO:: Support multiple slicer instances running in same box
aiaa_tmpdir = os.path.join(tempfile.gettempdir(), 'slicer-aiaa')
shutil.rmtree(aiaa_tmpdir, ignore_errors=True)
if not os.path.exists(aiaa_tmpdir):
    os.makedirs(aiaa_tmpdir)


def goodbye():
    print('GOOD BYE!! Removing Temp Dir: {}'.format(aiaa_tmpdir))
    shutil.rmtree(aiaa_tmpdir, ignore_errors=True)


atexit.register(goodbye)


class AIAALogic():
    def __init__(self, server_url='http://0.0.0.0:5000', server_version='v1', progress_callback=None):
        logging.info('Using AIAA: {}'.format(server_url))
        self.progress_callback = progress_callback

        aiaaClient.server_url = server_url
        aiaaClient.api_version = server_version
        self.client = aiaaClient
        logging.info('DOC ID inside client: {}'.format(aiaaClient.doc_id))

    def reportProgress(self, progress):
        if self.progress_callback:
            self.progress_callback(progress)

    def list_models(self, label=None):
        self.reportProgress(0)
        logging.info('Fetching List of Models for label: {}'.format(label))
        result = self.client.model_list(label)
        self.reportProgress(100)
        return result

    def segmentation(self, model, inputVolume):
        self.reportProgress(0)
        logging.info('Preparing for Segmentation Action')

        node_id = inputVolume.GetID()
        in_file = volumeToImageFiles.get(node_id)
        logging.info('Node Id: {} => {}'.format(node_id, in_file))

        if in_file is None:
            in_file = tempfile.NamedTemporaryFile(suffix='.nii.gz', dir=aiaa_tmpdir).name

            self.reportProgress(5)
            slicer.util.saveNode(inputVolume, in_file)

            volumeToImageFiles[node_id] = in_file
            logging.info('Saved Input Node into: {}'.format(in_file))
        else:
            logging.info('Using Saved Node from: {}'.format(in_file))

        self.reportProgress(30)

        result_file = tempfile.NamedTemporaryFile(suffix='.nii.gz').name
        params = self.client.segmentation(model, in_file, result_file, save_doc=True)

        extreme_points = params.get('points', params.get('extreme_points'))
        logging.info('Extreme Points: {}'.format(extreme_points))

        self.reportProgress(100)
        return extreme_points, result_file

    def dextr3d(self, model, pointset, inputVolume, modelInfo):
        self.reportProgress(0)
        logging.info('Preparing for Annotation/Dextr3D Action')

        node_id = inputVolume.GetID()
        in_file = volumeToImageFiles.get(node_id)
        logging.info('Node Id: {} => {}'.format(node_id, in_file))

        if in_file is None:
            in_file = tempfile.NamedTemporaryFile(suffix='.nii.gz', dir=aiaa_tmpdir).name

            self.reportProgress(5)
            slicer.util.saveNode(inputVolume, in_file)

            volumeToImageFiles[node_id] = in_file
            logging.info('Saved Input Node into: {}'.format(in_file))
        else:
            logging.info('Using Saved Node from: {}'.format(in_file))

        self.reportProgress(30)

        # Pre Process
        pad = 20
        roi_size = '128x128x128'
        if (modelInfo is not None) and ('padding' in modelInfo):
            pad = modelInfo['padding']
            roi_size = 'x'.join(map(str, modelInfo['roi']))

        result_file = tempfile.NamedTemporaryFile(suffix='.nii.gz').name
        self.client.dextr3d(model, pointset, in_file, result_file, pad, roi_size)

        self.reportProgress(100)
        return result_file
