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

#include <itkResampleImageFilter.h>
#include <itkRegionOfInterestImageFilter.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkNearestNeighborInterpolateImageFunction.h>

#include <itkMinimumMaximumImageCalculator.h>
#include <itkBinaryThresholdImageFilter.h>

#include <nvidia/aiaa/utils.h>
#include <chrono>

#ifdef __GNUG__ // gnu C++ compiler
#include <cxxabi.h>

std::string demangle(const char* mangled_name) {
  std::size_t len = 0;
  int status = 0;
  std::unique_ptr< char, decltype(&std::free) > ptr(
      __cxxabiv1::__cxa_demangle( mangled_name, nullptr, &len, &status ), &std::free );
  return ptr.get();
}

#else
std::string demangle(const char* name) {
  return name;
}
#endif

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

// BEGIN::
// This is AIAA code borrowed from itkutils.cpp
// to make MITK faster for pre-processing the different types image and generate sampled input for segmentation
template<typename TPixel, unsigned int VImageDimension>
typename itk::Image<TPixel, VImageDimension>::Pointer resizeImage(itk::Image<TPixel, VImageDimension> *itkImage,
                                                                  typename itk::Image<TPixel, VImageDimension>::SizeType targetSize,
                                                                  bool linearInterpolate) {

  auto imageSize = itkImage->GetLargestPossibleRegion().GetSize();
  auto imageSpacing = itkImage->GetSpacing();

  typedef itk::Image<TPixel, VImageDimension> ImageType;
  typename ImageType::SpacingType targetSpacing;
  for (unsigned int i = 0; i < VImageDimension; i++) {
    targetSpacing[i] = imageSpacing[i] * (static_cast<double>(imageSize[i]) / static_cast<double>(targetSize[i]));
  }

  auto filter = itk::ResampleImageFilter<ImageType, ImageType>::New();
  filter->SetInput(itkImage);

  filter->SetSize(targetSize);
  filter->SetOutputSpacing(targetSpacing);
  filter->SetTransform(itk::IdentityTransform<double, VImageDimension>::New());
  filter->SetOutputOrigin(itkImage->GetOrigin());
  filter->SetOutputDirection(itkImage->GetDirection());

  if (linearInterpolate) {
    filter->SetInterpolator(itk::LinearInterpolateImageFunction<ImageType, double>::New());
  } else {
    filter->SetInterpolator(itk::NearestNeighborInterpolateImageFunction<ImageType, double>::New());
  }

  filter->UpdateLargestPossibleRegion();
  return filter->GetOutput();
}

template<typename TPixel, unsigned int VImageDimension>
nvidia::aiaa::PointSet imagePreProcess(const nvidia::aiaa::PointSet &pointSet, itk::Image<TPixel, VImageDimension> *itkImage,
                                       const std::string &outputImage, nvidia::aiaa::ImageInfo &imageInfo, double PAD,
                                       const nvidia::aiaa::Point& ROI) {
  MITK_DEBUG("nvidia") << "Total Points: " << pointSet.points.size();
  MITK_DEBUG("nvidia") << "PAD: " << PAD;
  //MITK_DEBUG("nvidia") << "ROI: " << ROI;

  MITK_DEBUG("nvidia") << "Input Image: " << itkImage;
  MITK_DEBUG("nvidia") << "Input Image Region: " << itkImage->GetLargestPossibleRegion();

  try {
    // Preprocessing (crop and resize)
    typedef itk::Image<TPixel, VImageDimension> ImageType;
    typename ImageType::SizeType imageSize = itkImage->GetLargestPossibleRegion().GetSize();
    typename ImageType::IndexType indexMin;
    typename ImageType::IndexType indexMax;
    typename ImageType::SpacingType spacing = itkImage->GetSpacing();
    for (unsigned int i = 0; i < VImageDimension; i++) {
      indexMin[i] = INT_MAX;
      indexMax[i] = INT_MIN;
    }

    typename ImageType::IndexType index;
    int pointCount = 0;
    for (auto point : pointSet.points) {
      pointCount++;
      for (unsigned int i = 0; i < VImageDimension; i++) {
        int vxPad = (int) (spacing[i] > 0 ? (PAD / spacing[i]) : PAD);
        if (pointCount == 1) {
          MITK_DEBUG("nvidia") << "[DIM " << i << "] Padding: " << PAD << "; Spacing: " << spacing[i] << "; VOXEL Padding: " << vxPad;
        }

        index[i] = point[i];
        indexMin[i] = std::min(std::max((int) (index[i] - vxPad), 0), (int) (indexMin[i]));
        indexMax[i] = std::max(std::min((int) (index[i] + vxPad), (int) (imageSize[i] - 1)), (int) (indexMax[i]));

        if (indexMin[i] > indexMax[i]) {
          MITK_ERROR("nvidia") << "Invalid PointSet w.r.t. input Image; [i=" << i << "] MinIndex: " << indexMin[i] << "; MaxIndex: " << indexMax[i];
          throw nvidia::aiaa::exception(nvidia::aiaa::exception::INVALID_ARGS_ERROR, "Invalid PointSet w.r.t. input Image");
        }
      }
    }

    // Output min max index (ROI region)
    MITK_DEBUG("nvidia") << "Min index: " << indexMin << "; Max index: " << indexMax;

    // Extract ROI image
    typename ImageType::IndexType cropIndex;
    typename ImageType::SizeType cropSize;
    for (unsigned int i = 0; i < VImageDimension; i++) {
      cropIndex[i] = indexMin[i];
      cropSize[i] = indexMax[i] - indexMin[i];

      imageInfo.cropSize[i] = cropSize[i];
      imageInfo.imageSize[i] = imageSize[i];
      imageInfo.cropIndex[i] = cropIndex[i];
    }

    MITK_DEBUG("nvidia") << "ImageInfo >>>> " << imageInfo.dump();

    auto cropFilter = itk::RegionOfInterestImageFilter<ImageType, ImageType>::New();
    cropFilter->SetInput(itkImage);

    typename ImageType::RegionType cropRegion(cropIndex, cropSize);
    cropFilter->SetRegionOfInterest(cropRegion);
    cropFilter->Update();

    auto croppedItkImage = cropFilter->GetOutput();
    MITK_DEBUG("nvidia") << "++++ Cropped Image: " << croppedItkImage->GetLargestPossibleRegion();

    // Resize to 128x128x128x128
    typename ImageType::SizeType roiSize;
    for (unsigned int i = 0; i < VImageDimension; i++) {
      roiSize[i] = ROI[i];
    }

    auto resampledImage = resizeImage(cropFilter->GetOutput(), roiSize, true);
    MITK_DEBUG("nvidia") << "ResampledImage completed";

    // Adjust extreme points index to cropped and resized image
    nvidia::aiaa::PointSet pointSetROI;
    for (auto p : pointSet.points) {
      for (unsigned int i = 0; i < VImageDimension; i++) {
        index[i] = p[i];
      }

      // First convert to world coordinates within original image
      // Then convert back to index within resized image
      typename ImageType::PointType point;
      itkImage->TransformIndexToPhysicalPoint(index, point);
      resampledImage->TransformPhysicalPointToIndex(point, index);

      nvidia::aiaa::Point pointROI;
      for (unsigned int i = 0; i < VImageDimension; i++) {
        pointROI.push_back(index[i]);
      }
      pointSetROI.points.push_back(pointROI);
    }

    MITK_DEBUG("nvidia") << "PointSetROI: " << pointSetROI.toJson();

    // Write the ROI image out to temp folder
    auto writer = itk::ImageFileWriter<ImageType>::New();
    writer->SetInput(resampledImage);
    writer->SetFileName(outputImage);
    writer->Update();

    return pointSetROI;
  } catch (itk::ExceptionObject& e) {
    MITK_ERROR("nvidia") << (e.what());
    throw nvidia::aiaa::exception(nvidia::aiaa::exception::ITK_PROCESS_ERROR, e.what());
  }
}

// END::
// This is AIAA code borrowed from itkutils.cpp
// to make MITK faster for pre-processing the different types image and generate sampled input for segmentation

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
  auto labelActiveLayerId = labelSetImage->GetActiveLayer();
  MITK_INFO("nvidia") << "labelActiveLayerId: " << labelActiveLayerId;

  auto labelSetActive = labelSetImage->GetActiveLabelSet();
  MITK_INFO("nvidia") << "labelSetActive: " << labelSetActive;

  auto labelActive = labelSetImage->GetActiveLabel(labelActiveLayerId);
  MITK_INFO("nvidia") << "labelActive: " << labelActive;

  std::string labelName = labelActive->GetName();
  MITK_INFO("nvidia") << "labelName: " << labelName;

  mitk::Color labelColor = labelActive->GetColor();
  MITK_INFO("nvidia") << "labelColor: " << labelColor;

  std::string tmpSampleFileName = nvidia::aiaa::Utils::tempfilename() + ".nii.gz";
  std::string tmpResultFileName = nvidia::aiaa::Utils::tempfilename() + ".nii.gz";

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
    nvidia::aiaa::Pixel::Type pixelType = nvidia::aiaa::getPixelType(demangle(typeid(TPixel).name()));
    MITK_INFO("nvidia") << "++++ typeid(TPixel): " << typeid(TPixel).name() << "; demangle(typeid(TPixel)): " << demangle(typeid(TPixel).name())
        << "; PixelType: " << nvidia::aiaa::getPixelTypeStr(pixelType);
    MITK_INFO("nvidia") << "++++ VImageDimension: " << VImageDimension;

    LATENCY_START_API_CALL()
    nvidia::aiaa::PointSet pointSetROI = imagePreProcess(pointSet, itkImage, tmpSampleFileName, imageInfo, model.padding, model.roi);
    //void* inputImage = itkImage;
    //client.sampling(model, pointSet, inputImage, pixelType, VImageDimension, tmpSampleFileName, imageInfo);

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
      //boundingBoxRender<TPixel, VImageDimension>(tmpSampleFileName, labelName);
      //MITK_INFO("nvidia") << "Added Bounding Box for sampled Image";

      displayResult<TPixel, VImageDimension>(tmpResultFileName);
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

template<typename TPixel, unsigned int VImageDimension>
void NvidiaDextrSegTool3D::displayResult(const std::string &tmpResultFileName) {
  typedef itk::Image<mitk::Label::PixelType, VImageDimension> LabelImageType;
  typedef itk::ImageFileReader<LabelImageType> DextrReaderType;

  auto reader = DextrReaderType::New();
  reader->SetFileName(tmpResultFileName);
  reader->Update();
  auto itk_resultImage = reader->GetOutput();

  // add the ROI segmentation to data tree just for rendering
  // set to helper object making it invisible in Data manager
  mitk::Image::Pointer resultImage;
  mitk::CastToMitkImage(itk_resultImage, resultImage);

  auto labelSetImage = dynamic_cast<mitk::LabelSetImage *>(m_ToolManager->GetWorkingData(0)->GetData());
  auto labelSetActive = labelSetImage->GetActiveLabelSet();
  auto labelActive = labelSetImage->GetActiveLabel(labelSetImage->GetActiveLayer());
  std::string labelName = labelActive->GetName();
  mitk::Color labelColor = labelActive->GetColor();

  MITK_INFO("nvidia") << "(display) labelSetActive: " << labelSetActive;
  MITK_INFO("nvidia") << "(display) labelActive: " << labelActive;
  MITK_INFO("nvidia") << "(display) labelName: " << labelName;
  MITK_INFO("nvidia") << "(display) labelColor: " << labelColor;

  // Multi-Label support
  bool multiLabelSupport = true;
  if (multiLabelSupport) {
    typedef itk::MinimumMaximumImageCalculator<LabelImageType> MinimumMaximumImageCalculatorType;
    auto minMaxImageCalculator = MinimumMaximumImageCalculatorType::New();
    minMaxImageCalculator->SetImage(itk_resultImage);
    minMaxImageCalculator->Compute();
    MITK_INFO("nvidia") << "MinLabel: " << minMaxImageCalculator->GetMinimum() << "; MaxLabel: " << minMaxImageCalculator->GetMaximum();

    // Add labels for class labels higher than 1
    for (unsigned int classID = 2; classID <= minMaxImageCalculator->GetMaximum(); classID++) {
      std::string multiLabelName = labelName + std::to_string(classID);
      MITK_INFO("nvidia") << "adding multi-class " << multiLabelName;

      auto multiLabel = mitk::Label::New();
      multiLabel->SetName(multiLabelName);
      multiLabel->SetValue(classID);
      labelSetActive->AddLabel(multiLabel);

      // Set back previously active label
      labelSetActive->SetActiveLabel(labelActive->GetValue());
    }
  }
  // Multi-Label support

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

  // labelSetImage can only push a new layer at the end, so need to reload whole image for change
  for (unsigned int layerID = 0; layerID < layerTotal; layerID++) {
    // Always working on first layer-id (append, and delete moves the layer one-by-one)
    auto tempLayerImage = labelSetImage->GetLayerImage(0);
    auto updateLabelSet = labelSetImage->GetLabelSet(0);
    if (layerID != layerToReplace) {
      labelSetImage->AddLayer(tempLayerImage);
    } else {
      labelSetImage->AddLayer(resultImage);
    }

    // Copy LabelSet to Newly added Layer
    labelSetImage->AddLabelSetToLayer(labelSetImage->GetActiveLayer(), updateLabelSet);

    // Now remove the first layer (which also removes its LabelSet)
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
  auto allNodes = m_ToolManager->GetDataStorage()->GetAll();
  for (auto itNodes = allNodes->Begin(); itNodes != allNodes->End(); ++itNodes) {
    std::string nodeNameCheck = itNodes.Value()->GetName();
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
