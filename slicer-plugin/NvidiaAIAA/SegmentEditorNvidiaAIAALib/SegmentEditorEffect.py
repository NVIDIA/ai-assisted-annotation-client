import json
import logging
import os
import tempfile

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
        return """Use NVIDIA AI-Assisted features for segmentation and annotation."""

    def setupOptionsFrame(self):
        nvidiaFormLayout = qt.QFormLayout()

        ##################################################
        # AIAA Server + Fetch/Refresh Models
        nvidiaFormLayout.addRow(qt.QLabel())
        aiaaGroupBox = qt.QGroupBox("AI-Assisted Annotation Server: ")
        aiaaGroupLayout = qt.QFormLayout(aiaaGroupBox)
        self.serverUrl = qt.QLineEdit("http://0.0.0.0:5000")
        self.filterByLabel = qt.QCheckBox("Filter By Label")
        self.modelsButton = qt.QPushButton("Fetch Models")

        self.filterByLabel.setChecked(True)

        aiaaGroupLayout.addRow("Server URL: ", self.serverUrl)
        aiaaGroupLayout.addRow(self.filterByLabel)
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

        progressBar = slicer.util.createProgressDialog(windowTitle="Wait...", labelText="Fetching models", maximum=100)
        slicer.app.processEvents()
        try:
            logic = AIAALogic(self.serverUrl.text,
                              progress_callback=lambda progressPercentage,
                                                       progressBar=progressBar: SegmentEditorEffect.report_progress(
                                  progressBar, progressPercentage))
            label = self.getActiveLabel()
            logging.info('Active Label: {}'.format(label))
            models = logic.list_models(label if self.filterByLabel.checked else None)
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
        if self.filterByLabel.checked:
            msg += 'Using Label: \t\t' + label + '\t\n'
        msg += 'Total Models Loaded: \t' + str(len(models)) + '\t\n'
        msg += '-----------------------------------------------------\t\n'
        msg += 'Segmentation Models: \t' + str(self.segmentationModelSelector.count) + '\t\n'
        msg += 'Annotation Models: \t' + str(self.annotationModelSelector.count) + '\t\n'
        msg += '-----------------------------------------------------\t\n'
        qt.QMessageBox.information(slicer.util.mainWindow(), 'NVIDIA AIAA', msg)

    def updateSegmentationMask(self, json, in_file):
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
                points = None if json is None else json.get('points')
                logging.info('Extreme Points: {}'.format(points))
                if points is not None:
                    segment.SetTag("DExtr3DExtremePoints", points)

            else:
                segment.SetName(label + '_' + str(addedSegments))
            addedSegments = addedSegments + 1
        logging.info('Total Added Segments for {}: {}'.format(label, addedSegments))

        self.extremePoints[label] = json
        os.unlink(in_file)
        return True

    @staticmethod
    def report_progress(progressBar, progressPercentage):
        progressBar.show()
        progressBar.activateWindow()
        progressBar.setValue(progressPercentage)
        slicer.app.processEvents()

    def onClickSegmentation(self):
        model = self.segmentationModelSelector.currentText
        label = self.getActiveLabel()
        operationDescription = 'Run Segmentation for model: {} for label: {}'.format(model, label)
        logging.info(operationDescription)
        progressBar = slicer.util.createProgressDialog(windowTitle="Wait...", labelText=operationDescription,
                                                       maximum=100)
        slicer.app.processEvents()
        try:
            qt.QApplication.setOverrideCursor(qt.Qt.WaitCursor)
            logic = AIAALogic(self.serverUrl.text,
                              progress_callback=lambda progressPercentage,
                                                       progressBar=progressBar: SegmentEditorEffect.report_progress(
                                  progressBar, progressPercentage))

            inputVolume = self.scriptedEffect.parameterSetNode().GetMasterVolumeNode()
            json, result_file = logic.segmentation(model, inputVolume)
            result = 'SUCCESS' if self.updateSegmentationMask(json, result_file) is True else 'FAILED'

            if result is 'SUCCESS':
                self.segmentMarkupNode.RemoveAllMarkups()
                self.updateGUIFromMRML()
        except:
            qt.QApplication.restoreOverrideCursor()
            progressBar.close()
            slicer.util.errorDisplay(operationDescription + " - unexpected error.", detailedText=traceback.format_exc())
            return
        else:
            qt.QApplication.restoreOverrideCursor()
            progressBar.close()

        qt.QMessageBox.information(
            slicer.util.mainWindow(),
            'NVIDIA AIAA',
            'Run segmentation for (' + model + '): ' + result + '\t')

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
        model = self.annotationModelSelector.currentText
        label = self.getActiveLabel()
        operationDescription = 'Run Annotation for model: {} for label: {}'.format(model, label)
        logging.info(operationDescription)
        progressBar = slicer.util.createProgressDialog(windowTitle="Wait...", labelText=operationDescription,
                                                       maximum=100)
        slicer.app.processEvents()
        try:
            qt.QApplication.setOverrideCursor(qt.Qt.WaitCursor)
            logic = AIAALogic(self.serverUrl.text,
                              progress_callback=lambda progressPercentage,
                                                       progressBar=progressBar: SegmentEditorEffect.report_progress(
                                  progressBar, progressPercentage))

            inputVolume = self.scriptedEffect.parameterSetNode().GetMasterVolumeNode()
            modelInfo = self.models.get(model)
            pointSet = self.getFiducialPointsXYZ()
            json, result_file = logic.dextr3d(model, pointSet, inputVolume, modelInfo)
            result = 'SUCCESS' if self.updateSegmentationMask(json, result_file) is True else 'FAILED'

            if result is 'SUCCESS':
                self.updateGUIFromMRML()
        except:
            qt.QApplication.restoreOverrideCursor()
            progressBar.close()
            slicer.util.errorDisplay(operationDescription + " - unexpected error.", detailedText=traceback.format_exc())
            return
        else:
            qt.QApplication.restoreOverrideCursor()
            progressBar.close()

        qt.QMessageBox.information(
            slicer.util.mainWindow(),
            'NVIDIA AIAA',
            'Run dextr3D for (' + model + '): ' + result + '\t')

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
        logging.info('{} => {} Extreme points are: '.format(segmentID, label, fPosStr))

        if fPosStr is not None and len(fPosStr) > 0:
            points = json.loads(str(fPosStr))
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


class AIAALogic():
    def __init__(self, server_url='http://0.0.0.0:5000', server_version='v1', progress_callback=None):
        logging.info('Using AIAA: {}'.format(server_url))
        self.progress_callback = progress_callback
        self.client = AIAAClient(server_url, server_version)

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
        in_file = tempfile.NamedTemporaryFile(suffix='.nii.gz').name

        self.reportProgress(5)
        slicer.util.saveNode(inputVolume, in_file)
        logging.info('Saved Input Node into: {}'.format(in_file))
        self.reportProgress(30)

        result_file = tempfile.NamedTemporaryFile(suffix='.nii.gz').name
        extreme_points = self.client.segmentation(model, in_file, result_file)

        os.unlink(in_file)
        self.reportProgress(100)
        return extreme_points, result_file

    def dextr3d(self, model, pointset, inputVolume, modelInfo):
        self.reportProgress(0)
        logging.info('Preparing for Annotation/Dextr3D Action')
        in_file = tempfile.NamedTemporaryFile(suffix='.nii.gz').name

        self.reportProgress(5)
        slicer.util.saveNode(inputVolume, in_file)
        logging.info('Saved Input Node into: {}'.format(in_file))
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
        return {'points': json.dumps(pointset)}, result_file
