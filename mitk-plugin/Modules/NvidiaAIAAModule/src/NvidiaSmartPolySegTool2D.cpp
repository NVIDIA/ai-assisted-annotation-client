/*
 * Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <NvidiaSmartPolySegTool2D.h>

#include <usGetModuleContext.h>
#include <usModuleResource.h>

#include <mitkIOUtil.h>
#include <mitkImageCast.h>
#include <mitkLevelWindowPreset.h>
#include <mitkPointSetShapeProperty.h>
#include <mitkToolManager.h>

#include <itkExtractImageFilter.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkIntensityWindowingImageFilter.h>
#include <itkPasteImageFilter.h>

#include <nvidia/aiaa/client.h>
#include <nvidia/aiaa/utils.h>
#include <chrono>

MITK_TOOL_MACRO(MITKNVIDIAAIAAMODULE_EXPORT, NvidiaSmartPolySegTool2D, "NVIDIA SmartPoly Tool");

NvidiaSmartPolySegTool2D::NvidiaSmartPolySegTool2D()
    : m_NeighborhoodSize(1) {
  m_PointSetPolygon = mitk::PointSet::New();

  m_PointSetPolygonNode = mitk::DataNode::New();
  m_PointSetPolygonNode->SetProperty("name", mitk::StringProperty::New("Slice_Polygon"));
  m_PointSetPolygonNode->SetData(m_PointSetPolygon);

  m_PointInteractor = NvidiaPointSetDataInteractor::New();
  m_PointInteractor->LoadStateMachine("PointSet.xml");
  m_PointInteractor->SetEventConfig("PointSetConfig.xml");
  m_PointInteractor->SetDataNode(m_PointSetPolygonNode);
  m_PointInteractor->setNvidiaSmartPolySegTool(this);

  m_imageSize = nullptr;
  m_currentSlice = 0;
}

NvidiaSmartPolySegTool2D::~NvidiaSmartPolySegTool2D() {
}

us::ModuleResource NvidiaSmartPolySegTool2D::GetIconResource() const {
  auto moduleContext = us::GetModuleContext();
  auto module = moduleContext->GetModule();
  auto resource = module->GetResource("nvidia_logo.png");
  return resource;
}

const char *NvidiaSmartPolySegTool2D::GetName() const {
  return "Nvidia SmartPoly";
}

const char **NvidiaSmartPolySegTool2D::GetXPM() const {
  return nullptr;
}

void NvidiaSmartPolySegTool2D::Activated() {
  Superclass::Activated();

  m_PointSetPolygon->Clear();
  if (mitk::DataStorage *dataStorage = m_ToolManager->GetDataStorage()) {
    m_OriginalImageNode = m_ToolManager->GetReferenceData(0);
    m_WorkingData = m_ToolManager->GetWorkingData(0);

    // add to datastorage and enable interaction
    if (!dataStorage->Exists(m_PointSetPolygonNode)) {
      dataStorage->Add(m_PointSetPolygonNode, m_WorkingData);
    }

    m_imageGeometry = dynamic_cast<mitk::Image *>(m_OriginalImageNode->GetData())->GetGeometry();
    m_imageSize = dynamic_cast<mitk::Image *>(m_OriginalImageNode->GetData())->GetDimensions();

    // Toggle boundingbox and surface rendering off
    std::string targetNodeName1 = "Surface3D";
    std::string targetNodeName2 = "BoundingBox";
    auto allNodes = m_ToolManager->GetDataStorage()->GetAll();

    for (auto itNodes = allNodes->Begin(); itNodes != allNodes->End(); ++itNodes) {
      std::string nodeNameCheck = itNodes.Value()->GetName();
      if (nodeNameCheck.find(targetNodeName1) != std::string::npos) {
        itNodes.Value()->SetProperty("visible", mitk::BoolProperty::New(false));
      }
      if (nodeNameCheck.find(targetNodeName2) != std::string::npos) {
        itNodes.Value()->SetProperty("visible", mitk::BoolProperty::New(false));
      }
    }
  }
}

void NvidiaSmartPolySegTool2D::Deactivated() {
  // Remove the polygon pointset node
  if (mitk::DataStorage *dataStorage = m_ToolManager->GetDataStorage()) {
    dataStorage->Remove(m_PointSetPolygonNode);

    // Remove all polygon interaction nodes
    mitk::DataStorage::SetOfObjects::ConstPointer allNodes = dataStorage->GetAll();

    for (auto itNodes = allNodes->Begin(); itNodes != allNodes->End(); ++itNodes) {
      std::string nodeNameCheck = itNodes.Value()->GetName();
      std::string subCheck = "Polygon_";

      size_t pos = nodeNameCheck.find(subCheck);
      if (pos != std::string::npos) {
        mitk::DataNode::Pointer nodeToDelete = itNodes.Value();
        dataStorage->Remove(nodeToDelete);
        nodeToDelete->SetData(NULL);
      }
    }
  }

  Superclass::Deactivated();
}

void NvidiaSmartPolySegTool2D::SetServerURI(const std::string &serverURI, const int serverTimeout) {
  m_AIAAServerUri = serverURI;
  m_AIAAServerTimeout = serverTimeout;
}

void NvidiaSmartPolySegTool2D::SetNeighborhoodSize(int neighborhoodSize) {
  m_NeighborhoodSize = neighborhoodSize;
}

void NvidiaSmartPolySegTool2D::SetFlipPoly(bool flipPoly) {
  m_FlipPoly = flipPoly;
}

std::string NvidiaSmartPolySegTool2D::create2DSliceImage() {
  unsigned int curSliceNum = m_imageSize[2] - 1 - m_currentSlice;

  // Write the current image slice out
  typedef itk::Image<int, 3> InputImageType;
  typedef itk::Image<unsigned char, 3> WindowImageType;
  typedef itk::Image<unsigned char, 2> OutputImageType;

  // Get level window settings
  mitk::LevelWindowPreset *lw = mitk::LevelWindowPreset::New();
  double curLevel = 0;
  double curWindow = 0;
  if (lw->LoadPreset()) {
    mitk::LabelSetImage::Pointer labelSetImageTemp = dynamic_cast<mitk::LabelSetImage *>(m_ToolManager->GetWorkingData(0)->GetData());
    std::string organName = labelSetImageTemp->GetActiveLabel(labelSetImageTemp->GetActiveLayer())->GetName();
    curLevel = lw->getLevel(organName);
    curWindow = lw->getWindow(organName);

    if ((curLevel != 0) || (curWindow != 0)) {
      MITK_INFO("nvidia") << "Organ: " << organName << " found in Presets";
    } else {
      mitk::LevelWindow lwApply;
      m_ToolManager->GetReferenceData(0)->GetLevelWindow(lwApply);
      curWindow = lwApply.GetWindow();
      curLevel = lwApply.GetLevel();

      MITK_INFO("nvidia") << "Organ: " << organName << " not found in Presets, will use current window";
    }

    MITK_INFO("nvidia") << "Current level: " << curLevel;
    MITK_INFO("nvidia") << "Current window: " << curWindow;
  }

  // Window the image
  std::string tmpImage2DFileName = nvidia::aiaa::Utils::tempfilename() + ".png";  // image_slice_2D.png
  InputImageType::Pointer itkImage = InputImageType::New();
  mitk::CastToItkImage(dynamic_cast<mitk::Image *>(m_OriginalImageNode->GetData()), itkImage);

  typedef itk::IntensityWindowingImageFilter<InputImageType, WindowImageType> WindowFilterType;
  auto window = WindowFilterType::New();
  window->SetInput(itkImage);
  window->SetWindowMinimum(curLevel - curWindow / 2);
  window->SetWindowMaximum(curLevel + curWindow / 2);
  window->SetOutputMinimum(0);
  window->SetOutputMaximum(255);
  window->UpdateLargestPossibleRegion();
  window->UpdateOutputInformation();

  WindowImageType::RegionType inputRegion = window->GetOutput()->GetLargestPossibleRegion();
  WindowImageType::SizeType size = inputRegion.GetSize();
  size[2] = 0;

  WindowImageType::IndexType start = inputRegion.GetIndex();
  start[2] = curSliceNum;

  WindowImageType::RegionType desiredRegion;
  desiredRegion.SetSize(size);
  desiredRegion.SetIndex(start);

  typedef itk::ExtractImageFilter<WindowImageType, OutputImageType> ExtractFilterType;
  auto extract = ExtractFilterType::New();
  extract->InPlaceOn();
  extract->SetDirectionCollapseToSubmatrix();
  extract->SetExtractionRegion(desiredRegion);
  extract->SetInput(window->GetOutput());

  typedef itk::ImageFileWriter<OutputImageType> WriterType;
  auto writer = WriterType::New();
  writer->SetInput(extract->GetOutput());
  writer->SetFileName(tmpImage2DFileName);
  writer->Update();

  return tmpImage2DFileName;
}

void NvidiaSmartPolySegTool2D::PolygonFix() {
  if (!m_imageSize) {
    MITK_INFO("nvidia") << "ImageSize is NULL; Something is wrong";
    return;
  }

  if (m_AIAAServerUri.empty()) {
    Tool::GeneralMessage("aiaa::server URI is not set");
    return;
  }
  MITK_INFO("nvidia") << "aiaa::server URI >>> " << m_AIAAServerUri << "; Timeout: " << m_AIAAServerTimeout;
  MITK_INFO("nvidia") << "aiaa::server NeighborhoodSize >>> " << m_NeighborhoodSize;

  std::string tmpImage2DFileName = create2DSliceImage();
  MITK_INFO("nvidia") << "TmpImage2DFileName " << tmpImage2DFileName;

  unsigned int curSliceNum = m_imageSize[2] - 1 - m_currentSlice;
  MITK_INFO("nvidia") << "Current slice: " << curSliceNum << std::endl;

  if (curSliceNum >= m_polygonsList.list.size()) {
    MITK_INFO("nvidia") << "PolygonsList Empty; Something is wrong" << curSliceNum;
    return;
  }

  nvidia::aiaa::Polygons polygonsNew;
  nvidia::aiaa::Polygons polygonsOld = m_polygonsList.list[curSliceNum];
  if (polygonsOld.empty()) {
    MITK_INFO("nvidia") << "Polygons Empty; Something is wrong" << curSliceNum;
    return;
  }

  for (size_t polygonNum = 0; polygonNum < polygonsOld.polys.size(); polygonNum++) {
    std::string targetNodeName = "Polygon_" + std::to_string(polygonNum + 1);
    auto allNodes = m_ToolManager->GetDataStorage()->GetAll();
    mitk::PointSet::PointsContainer::Pointer polyVertices;

    for (auto itNodes = allNodes->Begin(); itNodes != allNodes->End(); ++itNodes) {
      if (itNodes.Value()->GetName() == targetNodeName) {
        polyVertices = dynamic_cast<mitk::PointSet *>(itNodes.Value()->GetData())->GetPointSet()->GetPoints();
      }
    }

    nvidia::aiaa::Polygons::Polygon polygonNew;
    for (auto vertexIt = polyVertices->Begin(); vertexIt != polyVertices->End(); vertexIt++) {
      mitk::PointSet::PointType index;
      m_imageGeometry->WorldToIndex(vertexIt.Value(), index);

      nvidia::aiaa::Polygons::Point pointNew;
      pointNew.push_back(int(index[0]));  // X
      pointNew.push_back(int(index[1]));  // Y

      polygonNew.push_back(pointNew);
    }

    polygonsNew.push_back(polygonNew);
  }

  MITK_INFO("nvidia") << "PolygonsNew: " << polygonsNew.toJson();
  MITK_INFO("nvidia") << "PolygonsOld: " << polygonsOld.toJson();

  if (m_FlipPoly) {
    polygonsNew.flipXY();
    polygonsOld.flipXY();
  }

  int polyIndex = -1;
  int vertexIndex = -1;
  if (!polygonsNew.findFirstNonMatching(polygonsOld, polyIndex, vertexIndex)) {
    MITK_INFO("nvidia") << "Could not find an non-matching polygon";
    return;
  }

  MITK_INFO("nvidia") << "FirstNonMatching Polygon (polyIndex" << polyIndex << "; vertexIndex: " << vertexIndex << ")";

  // Call AIAA maskToPolygonConversion
  nvidia::aiaa::Client client(m_AIAAServerUri, m_AIAAServerTimeout);
  std::string outputImageFile = nvidia::aiaa::Utils::tempfilename() + ".png";  // updated_image_2D.png

  try {
    auto begin = std::chrono::high_resolution_clock::now();
    nvidia::aiaa::Polygons polygonsUpdated = client.fixPolygon(polygonsNew, polygonsOld, m_NeighborhoodSize, polyIndex, vertexIndex,
                                                               tmpImage2DFileName, outputImageFile);

    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    MITK_INFO("nvidia") << "API Latency for aiaa::client::fixPolygon() = " << ms << " milli sec";

    MITK_INFO("nvidia") << "aiaa::fixPolygon (Updated Polygons): " << polygonsUpdated.toJson();

    if (!polygonsUpdated.empty()) {
      if (m_FlipPoly) {
        polygonsUpdated.flipXY();
      }
      m_polygonsList.list[curSliceNum].polys = polygonsUpdated.polys;
      displayResult(outputImageFile);
    } else {
      MITK_INFO("nvidia") << "PolygonsUpdated: " << polyIndex;
      Tool::GeneralMessage("Failed to fix the polygon.  Empty response received from AIAA");
    }

    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
  } catch (nvidia::aiaa::exception &e) {
    std::string msg = "nvidia.aiaa.error." + std::to_string(e.id) + "\ndescription: " + e.name();
    Tool::GeneralMessage("Failed to execute 'fixPolygon' on Nvidia AIAA Server\n\n" + msg);
  }

  // Remove TempFiles
  std::remove(tmpImage2DFileName.c_str());
  std::remove(outputImageFile.c_str());
}

void NvidiaSmartPolySegTool2D::Mask2Polygon() {
  if (m_AIAAServerUri.empty()) {
    Tool::GeneralMessage("aiaa::server URI is not set");
    return;
  }
  MITK_INFO("nvidia") << "aiaa::server URI >>> " << m_AIAAServerUri << "; Timeout: " << m_AIAAServerTimeout;

  // Save Label Image to TempFile
  std::string tmpImageFileName = nvidia::aiaa::Utils::tempfilename() + ".nii.gz";
  mitk::DataNode::Pointer workingNode = m_ToolManager->GetWorkingData(0);
  mitk::LabelSetImage::Pointer labelSetImage = dynamic_cast<mitk::LabelSetImage *>(workingNode->GetData());

  mitk::Image::Pointer labelMask = mitk::Image::New();
  labelMask = labelSetImage->CreateLabelMask(1);
  mitk::IOUtil::Save(labelMask, tmpImageFileName);

  // Call AIAA maskToPolygonConversion
  auto begin = std::chrono::high_resolution_clock::now();
  nvidia::aiaa::Client client(m_AIAAServerUri, m_AIAAServerTimeout);

  try {
    int pointRatio = 10;
    m_polygonsList = client.maskToPolygon(pointRatio, tmpImageFileName);
    if (m_FlipPoly) {
      m_polygonsList.flipXY();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    MITK_INFO("nvidia") << "API Latency for aiaa::client::mask2Polygon() = " << ms << " milli sec";

    MITK_INFO("nvidia") << "aiaa::mask2Polygon: " << m_polygonsList.toJson();
  } catch (nvidia::aiaa::exception &e) {
    std::string msg = "nvidia.aiaa.error." + std::to_string(e.id) + "\ndescription: " + e.name();
    Tool::GeneralMessage("Failed to execute 'mask2Polygon' on Nvidia AIAA Server\n\n" + msg);
  }

  // Remove TempFile
  std::remove(tmpImageFileName.c_str());
}

void NvidiaSmartPolySegTool2D::SetCurrentSlice(unsigned int slice) {
  if (m_polygonsList.empty()) {
    return;
  }
  if (!m_imageSize) {
    MITK_INFO("nvidia") << "ImageSize is NULL; Something is wrong; Current Slice = " << slice;
    return;
  }

  m_currentSlice = slice;
  unsigned int sliceNum = m_imageSize[2] - m_currentSlice - 1;
  if (sliceNum >= m_polygonsList.size()) {
    // something wrong here
    return;
  }

  const nvidia::aiaa::Polygons &polygons = m_polygonsList.list[sliceNum];
  if (polygons.polys.empty()) {
    return;
  }

  // Remove existing Polygon_x for the current slice
  std::set<std::string> polygonNames1To100;
  for (int i = 0; i < 100; i++) {
    polygonNames1To100.insert("Polygon_" + std::to_string(i));
  }

  // Check if node with polygon name exist already, if so, remove for later update
  auto allNodes = m_ToolManager->GetDataStorage()->GetAll();
  for (auto itNodes = allNodes->Begin(); itNodes != allNodes->End(); ++itNodes) {
    if (polygonNames1To100.find(itNodes.Value()->GetName()) != polygonNames1To100.end()) {
      mitk::DataNode::Pointer nodeToDelete = itNodes.Value();
      m_ToolManager->GetDataStorage()->Remove(nodeToDelete);
      nodeToDelete->SetData(NULL);
    }
  }

  // Sequentially add polygons on the current slice
  int polygonNum = 1;
  for (auto &polygon : polygons.polys) {
    mitk::PointSet::Pointer newPointSet = mitk::PointSet::New();
    mitk::PointSet::PointType index;
    mitk::PointSet::PointType point;

    for (auto &pt : polygon) {
      index[0] = pt[0];
      index[1] = pt[1];
      index[2] = sliceNum;

      m_imageGeometry->IndexToWorld(index, point);
      newPointSet->InsertPoint(point);
    }

    // Set the names
    std::string sliceName;
    sliceName = "Slice_" + std::to_string(sliceNum);
    m_PointSetPolygonNode->SetProperty("name", mitk::StringProperty::New(sliceName));

    std::string polygonName = "Polygon_" + std::to_string(polygonNum);
    mitk::DataNode::Pointer newPointSetNode = mitk::DataNode::New();
    newPointSetNode->SetData(newPointSet);
    newPointSetNode->SetProperty("name", mitk::StringProperty::New(polygonName));
    newPointSetNode->SetProperty("pointsize", mitk::FloatProperty::New(10));
    newPointSetNode->SetProperty("point 2D size", mitk::FloatProperty::New(5));
    newPointSetNode->SetProperty("show contour", mitk::BoolProperty::New(true));
    newPointSetNode->SetProperty("close contour", mitk::BoolProperty::New(true));
    newPointSetNode->SetProperty("Pointset.2D.shape", mitk::PointSetShapeProperty::New("Circle"));

    // Initiate new interactor and connect
    NvidiaPointSetDataInteractor::Pointer newPointInteractor = NvidiaPointSetDataInteractor::New();
    newPointInteractor->LoadStateMachine("PointSet.xml");
    newPointInteractor->SetEventConfig("PointSetConfig.xml");
    newPointInteractor->SetDataNode(newPointSetNode);
    newPointInteractor->setNvidiaSmartPolySegTool(this);

    // Add the polygon node under control node
    m_ToolManager->GetDataStorage()->Add(newPointSetNode, m_PointSetPolygonNode);
    polygonNum++;
  }
}

void NvidiaSmartPolySegTool2D::displayResult(const std::string &tmpResultFileName) {
  unsigned int curSliceNum = m_imageSize[2] - 1 - m_currentSlice;
  mitk::DataNode::Pointer workingNode = m_ToolManager->GetWorkingData(0);
  if (!workingNode) {
    MITK_INFO("nvidia") << "No working data selected!" << std::endl;
    return;
  }

  mitk::LabelSetImage::Pointer labelSetImage = dynamic_cast<mitk::LabelSetImage *>(workingNode->GetData());
  if (labelSetImage.IsNull()) {
    MITK_INFO("nvidia") << "Error in label set image" << std::endl;
    return;
  }

  int activeLayerID = labelSetImage->GetActiveLayer();
  std::string labelName = labelSetImage->GetActiveLabel(activeLayerID)->GetName();

  using LabelImageType = itk::Image<mitk::Tool::DefaultSegmentationDataType, 3>;
  using LabelImageReaderType = itk::ImageFileReader<LabelImageType>;

  // read as pseudo 3D image for pasting slice back
  auto reader = LabelImageReaderType::New();
  reader->SetFileName(tmpResultFileName);
  reader->Update();

  auto result_slice = reader->GetOutput();

  // Important: use clone to just get the image content, prevent the write lock for later update
  typename LabelImageType::Pointer itkCurrentLayer;
  mitk::CastToItkImage(labelSetImage->GetLayerImage(activeLayerID)->Clone(), itkCurrentLayer);

  LabelImageType::IndexType destinationIndex;
  destinationIndex[0] = 0;
  destinationIndex[1] = 0;
  destinationIndex[2] = curSliceNum;

  auto pasteFilter = itk::PasteImageFilter<LabelImageType, LabelImageType>::New();
  pasteFilter->SetSourceImage(result_slice);
  pasteFilter->SetDestinationImage(itkCurrentLayer);
  pasteFilter->SetSourceRegion(result_slice->GetLargestPossibleRegion());
  pasteFilter->SetDestinationIndex(destinationIndex);
  pasteFilter->Update();

  mitk::Image::Pointer updatedMask = mitk::Image::New();
  mitk::CastToMitkImage < LabelImageType > (pasteFilter->GetOutput(), updatedMask);

  // Record the just-created layer ID
  unsigned int layerTotal = labelSetImage->GetNumberOfLayers();
  unsigned int layerToReplace = labelSetImage->GetActiveLayer();

  MITK_INFO("nvidia") << "Total Layers: " << layerTotal << "; Layer To Replace: " << layerToReplace;

  // labelSetImage can only push a new layer at the end, so need to reload whole image for change
  for (unsigned int layerID = 0; layerID < layerTotal; layerID++) {
    mitk::Image::Pointer tempLayerImage = labelSetImage->GetLayerImage(0);
    mitk::Label::Pointer updateLabel = labelSetImage->GetActiveLabel(0);
    if (layerID != layerToReplace) {
      labelSetImage->AddLayer(tempLayerImage);
    } else {
      labelSetImage->AddLayer(updatedMask);
    }

    labelSetImage->GetLabelSet(labelSetImage->GetActiveLayer())->AddLabel(updateLabel);

    // Now remove the first layer
    labelSetImage->SetActiveLayer(0);
    labelSetImage->RemoveLayer();
  }

  // Set active layer back to the active layer before updating
  labelSetImage->SetActiveLayer(layerToReplace);
  MITK_INFO("nvidia") << "Set Active Layer Done";

  layerTotal = labelSetImage->GetNumberOfLayers();
  unsigned int layerID = labelSetImage->GetActiveLayer();

  mitk::Image::Pointer tempLayerImage = labelSetImage->GetLayerImage(layerID);
  mitk::Label::Pointer tempLabel = labelSetImage->GetActiveLabel(layerID);
  std::string tempLayerName = tempLabel->GetName() + "Surface3D";

  MITK_INFO("nvidia") << "Segmentation Layer: " << layerID << " Organ Surface: " << tempLayerName;
  auto allNodes = m_ToolManager->GetDataStorage()->GetAll();
  for (auto itNodes = allNodes->Begin(); itNodes != allNodes->End(); ++itNodes) {
    std::string nodeNameCheck = itNodes.Value()->GetName();
    if (tempLayerName == nodeNameCheck) {
      mitk::DataNode::Pointer nodeToUpdate = itNodes.Value();
      m_ToolManager->GetDataStorage()->Remove(nodeToUpdate);

      nodeToUpdate->SetData(tempLayerImage);
      m_ToolManager->GetDataStorage()->Add(nodeToUpdate, m_WorkingData);
    }
  }
}
