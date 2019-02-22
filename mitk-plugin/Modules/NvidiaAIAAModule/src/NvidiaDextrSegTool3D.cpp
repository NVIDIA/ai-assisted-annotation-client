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

#include <NvidiaDextrSegTool3D.h>

#include <mitkImageAccessByItk.h>
#include <mitkImageCast.h>
#include <mitkProgressBar.h>
#include <mitkToolManager.h>
#include <usGetModuleContext.h>
#include <usModuleResource.h>

#include <itkConnectedComponentImageFilter.h>
#include <itkConstantPadImageFilter.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkLabelShapeKeepNObjectsImageFilter.h>

#include <nvidia/aiaa/utils.h>
#include <chrono>

MITK_TOOL_MACRO(MITKNVIDIAAIAAMODULE_EXPORT, NvidiaDextrSegTool3D, "NVIDIA Segmentation tool");

#define LATENCY_INIT_API_CALL() \
    auto api_start_time = std::chrono::high_resolution_clock::now(); \
    auto api_end_time = std::chrono::high_resolution_clock::now(); \
    auto api_latency_ms = std::chrono::duration_cast < std::chrono::milliseconds > (api_end_time - api_start_time).count();

#define LATENCY_START_API_CALL() \
    api_start_time = std::chrono::high_resolution_clock::now(); \

#define LATENCY_END_API_CALL(api) \
    api_end_time = std::chrono::high_resolution_clock::now(); \
    api_latency_ms = std::chrono::duration_cast < std::chrono::milliseconds > (api_end_time - api_start_time).count(); \
    MITK_INFO("nvidia") << "API Latency for aiaa::client::" << api << "() = " << api_latency_ms << " milli sec"; \

NvidiaDextrSegTool3D::NvidiaDextrSegTool3D() {
  m_PointSet = mitk::PointSet::New();

  m_PointSetNode = mitk::DataNode::New();
  m_PointSetNode->SetData(m_PointSet);
  m_PointSetNode->SetName("Clicked points for DEXTR3D");
  m_PointSetNode->SetProperty("helper object", mitk::BoolProperty::New(true));
  m_PointSetNode->SetProperty("layer", mitk::IntProperty::New(1024));
  m_PointSetNode->SetProperty("pointsize", mitk::FloatProperty::New(15));

  m_SeedPointInteractor = mitk::PointSetDataInteractor::New();
  m_SeedPointInteractor->LoadStateMachine("PointSet.xml");
  m_SeedPointInteractor->SetEventConfig("PointSetConfig.xml");
  m_SeedPointInteractor->SetDataNode(m_PointSetNode);
}

NvidiaDextrSegTool3D::~NvidiaDextrSegTool3D() {
}

us::ModuleResource NvidiaDextrSegTool3D::GetIconResource() const {
  auto moduleContext = us::GetModuleContext();
  auto module = moduleContext->GetModule();
  auto resource = module->GetResource("nvidia_logo.png");
  return resource;
}

const char *NvidiaDextrSegTool3D::GetName() const {
  return "Nvidia Segmentation";
}

const char **NvidiaDextrSegTool3D::GetXPM() const {
  return nullptr;
}

void NvidiaDextrSegTool3D::Activated() {
  Superclass::Activated();

  if (mitk::DataStorage *dataStorage = m_ToolManager->GetDataStorage()) {
    m_OriginalImageNode = m_ToolManager->GetReferenceData(0);
    m_WorkingData = m_ToolManager->GetWorkingData(0);

    // add to data storage and enable interaction
    if (!dataStorage->Exists(m_PointSetNode)) {
      dataStorage->Add(m_PointSetNode);
    }
  }
}

void NvidiaDextrSegTool3D::Deactivated() {
  m_PointSet->Clear();
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();

  Superclass::Deactivated();
}

void NvidiaDextrSegTool3D::SetServerURI(const std::string &serverURI) {
  m_AIAAServerUri = serverURI;
}

void NvidiaDextrSegTool3D::ClearPoints() {
  m_PointSet->Clear();
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void NvidiaDextrSegTool3D::ConfirmPoints() {
  MITK_INFO("nvidia") << "+++++++++= ConfirmPoints";

  if (m_ToolManager->GetWorkingData(0)->GetData()) {
    mitk::Image::Pointer image = dynamic_cast<mitk::Image *>(m_OriginalImageNode->GetData());
    if (image) {
      if (m_PointSet->GetSize() < nvidia::aiaa::Client::MIN_POINTS_FOR_SEGMENTATION) {
        Tool::GeneralMessage(
            "Please set at least '" + std::to_string(nvidia::aiaa::Client::MIN_POINTS_FOR_SEGMENTATION)
                + "' seed points in the image first (hold Shift key and click left mouse button on the image)");
        return;
      }

      AccessByItk_1(image, ItkImageProcessDextr3D, image->GetGeometry());
    }
  }
}

template<typename TPixel, unsigned int VImageDimension>
nvidia::aiaa::PointSet NvidiaDextrSegTool3D::getPointSet(mitk::BaseGeometry *imageGeometry) {
  using ImageType = itk::Image<TPixel, VImageDimension>;

  nvidia::aiaa::PointSet pointSet;
  auto points = m_PointSet->GetPointSet()->GetPoints();
  for (auto pointsIterator = points->Begin(); pointsIterator != points->End(); ++pointsIterator) {
    if (!imageGeometry->IsInside(pointsIterator.Value())) {
      continue;
    }

    typename ImageType::IndexType index;
    imageGeometry->WorldToIndex(pointsIterator.Value(), index);

    nvidia::aiaa::Point point;
    for (unsigned int i = 0; i < VImageDimension; i++) {
      point.push_back(index[i]);
    }
    pointSet.points.push_back(point);
  }
  return pointSet;
}

template<typename TPixel, unsigned int VImageDimension>
void NvidiaDextrSegTool3D::ItkImageProcessDextr3D(itk::Image<TPixel, VImageDimension> *itkImage, mitk::BaseGeometry *imageGeometry) {
  if (VImageDimension != 3) {
    Tool::GeneralMessage("Currently 3D Images are only supported for Nvidia Segmentation");
    return;
  }

  MITK_INFO("nvidia") << "++++++++ Nvidia Segmentation begins";

  nvidia::aiaa::PointSet pointSet = getPointSet<TPixel, VImageDimension>(imageGeometry);
  MITK_INFO("nvidia") << "PointSet: " << pointSet.toJson();

  // Collect information for working segmentation label
  auto labelSetImage = dynamic_cast<mitk::LabelSetImage *>(m_ToolManager->GetWorkingData(0)->GetData());
  std::string labelName = labelSetImage->GetActiveLabel(labelSetImage->GetActiveLayer())->GetName();
  std::string tmpSampleFileName = nvidia::aiaa::Utils::tempfilename() + ".nii.gz";
  std::string tmpResultFileName = nvidia::aiaa::Utils::tempfilename() + ".nii.gz";

  MITK_INFO("nvidia") << "Organ name: " << labelName;
  MITK_INFO("nvidia") << "Sample Image: " << tmpSampleFileName;
  MITK_INFO("nvidia") << "Output Image: " << tmpResultFileName;
  MITK_INFO("nvidia") << "aiaa::server URI >>> " << m_AIAAServerUri;

  if (m_AIAAServerUri.empty()) {
    Tool::GeneralMessage("aiaa::server URI is not set");
    return;
  }

  // Call AIAA segmentation DEXTR3D
  int totalSteps = 4;
  int currentSteps = 0;
  mitk::ProgressBar::GetInstance()->AddStepsToDo(totalSteps);
  try {
    LATENCY_INIT_API_CALL()
    nvidia::aiaa::Client client(m_AIAAServerUri);

    LATENCY_START_API_CALL()
    nvidia::aiaa::ModelList models = client.models();
    nvidia::aiaa::Model model = models.getMatchingModel(labelName);

    currentSteps++;
    mitk::ProgressBar::GetInstance()->Progress(1);
    LATENCY_END_API_CALL("models")
    MITK_INFO("nvidia") << "aiaa::models (Supported Models): " << models.toJson();
    MITK_INFO("nvidia") << "AIAA Selected Model for [" << labelName << "]: " << model.toJson();

    // Do Sampling
    nvidia::aiaa::ImageInfo imageInfo;
    MITK_INFO("nvidia") << "++++ typeid(TPixel): " << typeid(TPixel).name() << std::endl;
    MITK_INFO("nvidia") << "++++ VImageDimension: " << VImageDimension << std::endl;

    void* inputImage = itkImage;
    nvidia::aiaa::Pixel::Type pixelType = nvidia::aiaa::getPixelType(typeid(TPixel).name());

    LATENCY_START_API_CALL()
    nvidia::aiaa::PointSet pointSetROI = client.sampling(model, pointSet, inputImage, pixelType, VImageDimension, tmpSampleFileName, imageInfo);

    currentSteps++;
    mitk::ProgressBar::GetInstance()->Progress(1);
    LATENCY_END_API_CALL("sampling")
    MITK_INFO("nvidia") << "PointSetROI (sampled): " << pointSetROI.toJson();

    // Call Inference
    LATENCY_START_API_CALL()
    int ret = client.segmentation(model, pointSetROI, tmpSampleFileName, VImageDimension, tmpResultFileName, imageInfo);

    currentSteps++;
    mitk::ProgressBar::GetInstance()->Progress(1);
    LATENCY_END_API_CALL("segmentation")

    if (ret) {
      Tool::GeneralMessage("Failed to execute segmentation on Nvidia AIAA Server");
    } else {
      MITK_INFO("nvidia") << "aiaa::dextr3d SUCCESSFUL";

      // Generate Sample Image for adding bounding box
      boundingBoxRender<TPixel, VImageDimension>(tmpSampleFileName, labelName);
      displayResult<TPixel, VImageDimension>(tmpResultFileName);
      MITK_INFO("nvidia") << "Added Bounding Box for sampled Image";
    }
  } catch (nvidia::aiaa::exception &e) {
    std::string msg = "nvidia.aiaa.error." + std::to_string(e.id) + "\ndescription: " + e.name();
    Tool::GeneralMessage("Failed to execute 'dextr3d' on Nvidia AIAA Server\n\n" + msg);
  }

  std::remove(tmpSampleFileName.c_str());
  std::remove(tmpResultFileName.c_str());
  mitk::ProgressBar::GetInstance()->Progress(totalSteps - currentSteps);
  MITK_INFO("nvidia") << "++++++++ Nvidia Segmentation ends";
}

template<class TImageType>
typename TImageType::Pointer NvidiaDextrSegTool3D::getLargestConnectedComponent(TImageType *itkImage) {
  typedef itk::ConnectedComponentImageFilter<TImageType, TImageType> ConnectedComponentImageFilterType;

  typename ConnectedComponentImageFilterType::Pointer connected = ConnectedComponentImageFilterType::New();
  connected->SetInput(itkImage);
  connected->Update();

  MITK_INFO("nvidia") << "Number of objects: " << connected->GetObjectCount();

  typedef itk::LabelShapeKeepNObjectsImageFilter<TImageType> LabelShapeKeepNObjectsImageFilterType;
  typename LabelShapeKeepNObjectsImageFilterType::Pointer labelShapeKeepNObjectsImageFilter = LabelShapeKeepNObjectsImageFilterType::New();
  labelShapeKeepNObjectsImageFilter->SetInput(connected->GetOutput());
  labelShapeKeepNObjectsImageFilter->SetBackgroundValue(0);
  labelShapeKeepNObjectsImageFilter->SetNumberOfObjects(1);
  labelShapeKeepNObjectsImageFilter->SetAttribute(LabelShapeKeepNObjectsImageFilterType::LabelObjectType::NUMBER_OF_PIXELS);
  labelShapeKeepNObjectsImageFilter->Update();

  return labelShapeKeepNObjectsImageFilter->GetOutput();
}

template<typename TPixel, unsigned int VImageDimension>
void NvidiaDextrSegTool3D::displayResult(const std::string &tmpResultFileName) {
  typedef itk::Image<mitk::Label::PixelType, VImageDimension> LabelImageType;
  typedef itk::ImageFileReader<LabelImageType> DextrReaderType;

  typename DextrReaderType::Pointer reader = DextrReaderType::New();

  // Read result and display with current settings
  reader->SetFileName(tmpResultFileName);
  reader->Update();

  // select largest connected component (TODO:: May be not required)
  auto itk_resultImage = getLargestConnectedComponent < LabelImageType > (reader->GetOutput());

  // add the ROI segmentation to data tree just for rendering
  // set to helper object making it invisible in Data manager
  mitk::Image::Pointer resultImage;
  mitk::CastToMitkImage(itk_resultImage, resultImage);

  auto labelSetImage = dynamic_cast<mitk::LabelSetImage *>(m_ToolManager->GetWorkingData(0)->GetData());
  std::string labelName = labelSetImage->GetActiveLabel(labelSetImage->GetActiveLayer())->GetName();
  mitk::Color labelColor = labelSetImage->GetActiveLabel(labelSetImage->GetActiveLayer())->GetColor();

  std::string labelNameSurf = labelName + "Surface3D";
  mitk::DataNode::Pointer newNode = mitk::DataNode::New();
  newNode->SetData(resultImage);
  newNode->SetProperty("binary", mitk::BoolProperty::New(true));
  newNode->SetProperty("name", mitk::StringProperty::New(labelNameSurf));
  newNode->SetProperty("color", mitk::ColorProperty::New(labelColor));
  newNode->SetProperty("volumerendering", mitk::BoolProperty::New(true));
  newNode->SetProperty("opacity", mitk::FloatProperty::New(0));

  // check if node with same name exist already, if so, update instead of adding new
  auto allNodes = m_ToolManager->GetDataStorage()->GetAll();
  for (auto itNodes = allNodes->Begin(); itNodes != allNodes->End(); ++itNodes) {
    std::string nodeNameCheck = itNodes.Value()->GetName();
    if (labelNameSurf == nodeNameCheck) {
      // if already exist, delete the current and update with the new one
      mitk::DataNode::Pointer nodeToDelete = itNodes.Value();
      m_ToolManager->GetDataStorage()->Remove(nodeToDelete);
      nodeToDelete->SetData(NULL);
    }
  }
  m_ToolManager->GetDataStorage()->Add(newNode, m_WorkingData);
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();

  // Record the just-created layer ID
  unsigned int layerTotal = labelSetImage->GetNumberOfLayers();
  unsigned int layerToReplace = labelSetImage->GetActiveLayer();

  MITK_INFO("nvidia") << "Total Layers: " << layerTotal << "; Layer To Replace: " << layerToReplace;

  mitk::Image::Pointer recoverImage = mitk::Image::New();
  mitk::CastToMitkImage < LabelImageType > (reader->GetOutput(), recoverImage);

  // labelSetImage can only push a new layer at the end, so need to reload whole image for change
  for (unsigned int layerID = 0; layerID < layerTotal; layerID++) {
    mitk::Image::Pointer tempLayerImage = labelSetImage->GetLayerImage(0);
    mitk::Label::Pointer updateLabel = labelSetImage->GetActiveLabel(0);
    if (layerID != layerToReplace) {
      labelSetImage->AddLayer(tempLayerImage);
    } else {
      labelSetImage->AddLayer(recoverImage);
    }

    labelSetImage->GetLabelSet(labelSetImage->GetActiveLayer())->AddLabel(updateLabel);

    // Now remove the first layer
    labelSetImage->SetActiveLayer(0);
    labelSetImage->RemoveLayer();
  }

  // Set active layer back to the active layer before updating
  labelSetImage->SetActiveLayer(layerToReplace);
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

template<typename TPixel, unsigned int VImageDimension>
void NvidiaDextrSegTool3D::boundingBoxRender(const std::string &tmpResultFileName, const std::string &organName) {
  typedef itk::Image<TPixel, VImageDimension> InputImageType;
  typedef itk::ImageFileReader<InputImageType> DextrReaderType;

  auto reader = DextrReaderType::New();
  reader->SetFileName(tmpResultFileName);
  reader->Update();
  auto roiImage = reader->GetOutput();

  std::string nodeName = organName + "BoundingBox";

  // generate all 1 image with same specifications of roiImage
  typename InputImageType::RegionType region;
  region.SetSize(roiImage->GetLargestPossibleRegion().GetSize());

  auto coreBoxImage = InputImageType::New();
  coreBoxImage->SetSpacing(roiImage->GetSpacing());
  coreBoxImage->SetOrigin(roiImage->GetOrigin());
  coreBoxImage->SetDirection(roiImage->GetDirection());
  coreBoxImage->SetRegions(region);
  coreBoxImage->Allocate();
  coreBoxImage->FillBuffer(1);

  // pad with 0 plates for correct bounding box rendering
  typedef itk::ConstantPadImageFilter<InputImageType, InputImageType> PadBoxImageFilterType;
  typename InputImageType::PixelType constantZeroPixel = 0;
  typename InputImageType::SizeType padBoxLowerBound;
  typename InputImageType::SizeType padBoxUpperBound;
  for (unsigned int i = 0; i < VImageDimension; i++) {
    padBoxLowerBound[i] = 1;
    padBoxUpperBound[i] = 1;
  }

  auto padBoxFilter = PadBoxImageFilterType::New();
  padBoxFilter->SetInput(coreBoxImage);
  padBoxFilter->SetPadLowerBound(padBoxLowerBound);
  padBoxFilter->SetPadUpperBound(padBoxUpperBound);
  padBoxFilter->SetConstant(constantZeroPixel);
  padBoxFilter->Update();

  auto boundingBoxImage = InputImageType::New();
  boundingBoxImage = padBoxFilter->GetOutput();
  boundingBoxImage->SetOrigin(coreBoxImage->GetOrigin());

  // cast to mitk for render
  mitk::Image::Pointer bBoxImage;
  mitk::CastToMitkImage(boundingBoxImage, bBoxImage);
  mitk::DataNode::Pointer boundingBoxNode = mitk::DataNode::New();
  boundingBoxNode->SetData(bBoxImage);
  float r = 1;
  float g = 0.2;
  float b = 1;
  boundingBoxNode->SetProperty("binary", mitk::BoolProperty::New(true));
  boundingBoxNode->SetProperty("name", mitk::StringProperty::New(nodeName));
  boundingBoxNode->SetProperty("color", mitk::ColorProperty::New(r, g, b));
  boundingBoxNode->SetProperty("volumerendering", mitk::BoolProperty::New(true));
  boundingBoxNode->SetProperty("opacity", mitk::FloatProperty::New(0));
  // boundingBoxNode->SetProperty("helper object", mitk::BoolProperty::New(true));
  // boundingBoxNode->SetProperty("visible", mitk::BoolProperty::New(false));

  // check if node with same name exist already, if so, update instead of adding new
  mitk::DataStorage::SetOfObjects::ConstPointer allNodes = m_ToolManager->GetDataStorage()->GetAll();
  mitk::DataStorage::SetOfObjects::ConstIterator itNodes;
  std::string nodeNameCheck;
  for (itNodes = allNodes->Begin(); itNodes != allNodes->End(); ++itNodes) {
    nodeNameCheck = itNodes.Value()->GetName();
    if (nodeName == nodeNameCheck) {
      // if already exist, delete the current and update with the new one
      mitk::DataNode::Pointer nodeToDelete = itNodes.Value();
      m_ToolManager->GetDataStorage()->Remove(nodeToDelete);
      nodeToDelete->SetData(NULL);
    }
  }
  m_ToolManager->GetDataStorage()->Add(boundingBoxNode, m_WorkingData);
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}
