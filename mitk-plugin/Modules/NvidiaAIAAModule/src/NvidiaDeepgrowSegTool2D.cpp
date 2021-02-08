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

#include <NvidiaDeepgrowSegTool2D.h>

#include <usGetModuleContext.h>
#include <usModuleResource.h>

#include <mitkIOUtil.h>
#include <mitkImageCast.h>
#include <mitkLabelSetImage.h>
#include <mitkLevelWindowPreset.h>
#include <mitkPointSetShapeProperty.h>
#include <mitkToolManager.h>
#include <mitkImageAccessByItk.h>
#include <mitkProgressBar.h>

#include <itkExtractImageFilter.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkIntensityWindowingImageFilter.h>
#include <itkPasteImageFilter.h>
#include <itkAddImageFilter.h>

#include <nvidia/aiaa/client.h>
#include <nvidia/aiaa/utils.h>
#include <chrono>

MITK_TOOL_MACRO(MITKNVIDIAAIAAMODULE_EXPORT, NvidiaDeepgrowSegTool2D, "NVIDIA Deepgrow Tool");

NvidiaDeepgrowSegTool2D::NvidiaDeepgrowSegTool2D() {
  m_PointSetNode = mitk::DataNode::New();
  m_PointSetNode->GetPropertyList()->SetProperty("name", mitk::StringProperty::New("Deepgrow_Foreground_Points"));
  m_PointSetNode->GetPropertyList()->SetProperty("helper object", mitk::BoolProperty::New(true));

  m_PointSet = mitk::PointSet::New();
  m_PointSetNode->SetData(m_PointSet);
}

NvidiaDeepgrowSegTool2D::~NvidiaDeepgrowSegTool2D() {
}

us::ModuleResource NvidiaDeepgrowSegTool2D::GetIconResource() const {
  auto moduleContext = us::GetModuleContext();
  auto module = moduleContext->GetModule();
  auto resource = module->GetResource("nvidia_logo.png");
  return resource;
}

bool NvidiaDeepgrowSegTool2D::CanHandle(const mitk::BaseData * referenceData, const mitk::BaseData * workingData) const {
  if (!Superclass::CanHandle(referenceData, workingData))
    return false;

  if (referenceData == nullptr)
    return false;

  const auto* image = dynamic_cast<const mitk::Image*>(referenceData);
  if (image == nullptr)
    return false;

  if (image->GetDimension() != 3)
    return false;

  return true;
}

const char* NvidiaDeepgrowSegTool2D::GetName() const {
  return "Deepgrow";
}

const char** NvidiaDeepgrowSegTool2D::GetXPM() const {
  return nullptr;
}

void NvidiaDeepgrowSegTool2D::Activated() {
  Superclass::Activated();

  if (!GetDataStorage()->Exists(m_PointSetNode))
    GetDataStorage()->Add(m_PointSetNode, GetWorkingData());

  m_pointInteractor = mitk::PointSetDataInteractor::New();
  m_pointInteractor->LoadStateMachine("PointSet.xml");
  m_pointInteractor->SetEventConfig("PointSetConfig.xml");
  m_pointInteractor->SetDataNode(m_PointSetNode);
}

void NvidiaDeepgrowSegTool2D::Deactivated() {
  m_PointSet->Clear();
  GetDataStorage()->Remove(m_PointSetNode);

  Superclass::Deactivated();
}

mitk::DataNode* NvidiaDeepgrowSegTool2D::GetReferenceData() {
  return this->m_ToolManager->GetReferenceData(0);
}

mitk::DataStorage* NvidiaDeepgrowSegTool2D::GetDataStorage() {
  return this->m_ToolManager->GetDataStorage();
}

mitk::DataNode* NvidiaDeepgrowSegTool2D::GetWorkingData() {
  return this->m_ToolManager->GetWorkingData(0);
}

mitk::DataNode::Pointer NvidiaDeepgrowSegTool2D::GetPointSetNode() {
  return m_PointSetNode;
}

void NvidiaDeepgrowSegTool2D::SetServerURI(const std::string &serverURI, const int serverTimeout) {
  m_AIAAServerUri = serverURI;
  m_AIAAServerTimeout = serverTimeout;
  m_AIAAModelList = nvidia::aiaa::ModelList();

  try {
    nvidia::aiaa::Client client(serverURI, serverTimeout);
    m_AIAAModelList = client.models("", nvidia::aiaa::Model::deepgrow);
  } catch (nvidia::aiaa::exception &e) {
    std::string msg = "nvidia.aiaa.error." + std::to_string(e.id) + "\ndescription: " + e.name();
    Tool::GeneralMessage("Failed to communicate with Nvidia AIAA Server\nFix server URI in Nvidia AIAA preferences (Ctrl+P)\n\n" + msg);
  }
}

void NvidiaDeepgrowSegTool2D::GetModelInfo(std::map<std::string, std::string> &deepgrow) {
  for (size_t i = 0; i < m_AIAAModelList.size(); i++) {
    auto &model = m_AIAAModelList.models[i];
    std::string tooltip = model.description + "<br/>";

    if (model.type == nvidia::aiaa::Model::deepgrow) {
      deepgrow[model.name] = tooltip;
    }
  }

  MITK_INFO("nvidia") << "Total Deepgrow Models: " << deepgrow.size();
}

void NvidiaDeepgrowSegTool2D::ClearPoints() {
  m_foregroundPoints = nvidia::aiaa::PointSet();
  m_backgroundPoints = nvidia::aiaa::PointSet();

  m_PointSet->Clear();
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void NvidiaDeepgrowSegTool2D::PointAdded(mitk::DataNode::Pointer imageNode, bool background) {
  mitk::DataNode *node = GetPointSetNode();
  if (node == nullptr) {
    return;
  }

  mitk::PointSet::Pointer pointSet = dynamic_cast<mitk::PointSet*>(node->GetData());
  if (pointSet.IsNull()) {
    mitk::Tool::ErrorMessage("PointSetNode does not contain a pointset");
    return;
  }

  nvidia::aiaa::PointSet finalPointSet;
  mitk::Image *image = dynamic_cast<mitk::Image*>(imageNode->GetData());
  auto points = pointSet->GetPointSet()->GetPoints();

  nvidia::aiaa::Point point;
  if (points->Size()) {
    auto pt = (--points->End())->Value();
    MITK_DEBUG("nvidia") << "(Deepgrow) Added Point: " << pt;

    if (!image->GetGeometry()->IsInside(pt)) {
      MITK_INFO("nvidia") << "(Deepgrow) Point Not in Geometry (IGNORED)";
      return;
    }

    itk::Index<3> index;
    image->GetGeometry(0)->WorldToIndex(pt, index);

    point.push_back(index[0]);
    point.push_back(index[1]);
    point.push_back(index[2]);

    MITK_DEBUG("nvidia") << "(Deepgrow) " << pt << " => [" << index[0] << "," << index[1] << "," << index[2] << "]";
  }

  if (background) {
    m_backgroundPoints.push_back(point);
    MITK_INFO("nvidia") << "(Deepgrow) All Background Points: " << finalPointSet.toJson();
  } else {
    m_foregroundPoints.push_back(point);
    MITK_INFO("nvidia") << "(Deepgrow) All Foreground Points: " << finalPointSet.toJson();
  }
}

void NvidiaDeepgrowSegTool2D::RunDeepGrow(const std::string &model, mitk::DataNode::Pointer imageNode) {
  m_AIAACurrentModelName = model;
  if (m_AIAACurrentModelName.empty()) {
    Tool::GeneralMessage("Deepgrow Model is not selected\n\n");
    return;
  }

  mitk::Image *image = dynamic_cast<mitk::Image*>(imageNode->GetData());
  std::string imageId = image->GetUID();
  MITK_INFO("nvidia") << "(Deepgrow) Image ID: " << imageId;

  AccessByItk_1(image, ItkImageProcessRunDeepgrow, imageId);
}

int NvidiaDeepgrowSegTool2D::GetCurrentSlice(const nvidia::aiaa::PointSet &points) {
  if (points.empty()) {
    return -1;
  }
  return points.points[points.points.size() - 1][2];
}

nvidia::aiaa::PointSet NvidiaDeepgrowSegTool2D::GetPointsForCurrentSlice(const nvidia::aiaa::PointSet &points, int sliceIndex) {
  if (points.empty() || sliceIndex < 0) {
    return points;
  }

  nvidia::aiaa::PointSet filtered;
  for (auto point : points.points) {
    if (point[2] == sliceIndex) {
      filtered.push_back(point);
    }
  }
  return filtered;
}

template<typename TPixel, unsigned int VImageDimension>
void NvidiaDeepgrowSegTool2D::ItkImageProcessRunDeepgrow(itk::Image<TPixel, VImageDimension> *itkImage, std::string imageId) {
  MITK_INFO("nvidia") << "(Deepgrow) ++++++++ Nvidia Deepgrow begins";

  nvidia::aiaa::Client client(m_AIAAServerUri, m_AIAAServerTimeout);
  std::string tmpInputFileName = nvidia::aiaa::Utils::tempfilename() + ".nii.gz";
  std::string tmpResultFileName = nvidia::aiaa::Utils::tempfilename() + ".nii.gz";

  int totalSteps = 3;
  int currentSteps = 0;
  mitk::ProgressBar::GetInstance()->AddStepsToDo(totalSteps);

  try {
    std::string aiaaSessionId;
    auto it = m_AIAASessions.find(imageId);
    if (it != m_AIAASessions.end()) {
      aiaaSessionId = it->second;
      MITK_INFO("nvidia") << "(Deepgrow) AIAA Session already exists: " << aiaaSessionId;
    }

    if (aiaaSessionId.empty()) {
      MITK_INFO("nvidia") << "(Deepgrow) Trying to add current image into AIAA session";

      typedef itk::Image<TPixel, VImageDimension> ImageType;
      auto writer = itk::ImageFileWriter<ImageType>::New();
      writer->SetInput(itkImage);
      writer->SetFileName(tmpInputFileName);
      writer->Update();

      aiaaSessionId = client.createSession(tmpInputFileName);
      m_AIAASessions[imageId] = aiaaSessionId;
    }
    MITK_INFO("nvidia") << "(Deepgrow) AIAA session ID: " << aiaaSessionId;
    currentSteps++;
    mitk::ProgressBar::GetInstance()->Progress(1);

    int sliceIndex = GetCurrentSlice(m_foregroundPoints);
    if (sliceIndex < 0) {
      sliceIndex = GetCurrentSlice(m_backgroundPoints);
    }
    MITK_INFO("nvidia") << "(Deepgrow) current slice: " << sliceIndex;

    auto foreground = GetPointsForCurrentSlice(m_foregroundPoints, sliceIndex);
    auto background = GetPointsForCurrentSlice(m_backgroundPoints, sliceIndex);
    MITK_INFO("nvidia") << "(Deepgrow) Foreground Points for current slice: " << foreground.toJson();
    MITK_INFO("nvidia") << "(Deepgrow) Background Points for current slice: " << background.toJson();

    nvidia::aiaa::Model model = client.model(m_AIAACurrentModelName);
    client.deepgrow(model, foreground, background, tmpInputFileName, tmpResultFileName, aiaaSessionId);
    currentSteps++;
    mitk::ProgressBar::GetInstance()->Progress(1);

    DisplayResult<TPixel, VImageDimension>(tmpResultFileName, sliceIndex);
    currentSteps++;
    mitk::ProgressBar::GetInstance()->Progress(1);
  } catch (nvidia::aiaa::exception &e) {
    if (e.id == nvidia::aiaa::exception::AIAA_SESSION_TIMEOUT) {
      m_AIAASessions.erase(imageId);
    }
    std::string msg = "nvidia.aiaa.error." + std::to_string(e.id) + "\ndescription: " + e.name();
    Tool::GeneralMessage("Failed to execute 'deepgrow' on Nvidia AIAA Server (Retry Again)\n\n" + msg);
  }

  std::remove(tmpInputFileName.c_str());
  std::remove(tmpResultFileName.c_str());

  mitk::ProgressBar::GetInstance()->Progress(totalSteps - currentSteps);
  MITK_INFO("nvidia") << "(Deepgrow) ++++++++ Nvidia Deepgrow ends";
}

template<typename TPixel, unsigned int VImageDimension>
void NvidiaDeepgrowSegTool2D::DisplayResult(const std::string &tmpResultFileName, int sliceIndex) {
  using LabelImageType = itk::Image<mitk::Label::PixelType, VImageDimension> ;
  using ImageReaderType = itk::ImageFileReader<LabelImageType> ;

  auto labelSetImage = dynamic_cast<mitk::LabelSetImage*>(m_ToolManager->GetWorkingData(0)->GetData());
  auto labelSetActive = labelSetImage->GetActiveLabelSet();
  auto labelActive = labelSetImage->GetActiveLabel(labelSetImage->GetActiveLayer());

  std::string labelName = labelActive->GetName();
  mitk::Color labelColor = labelActive->GetColor();

  MITK_INFO("nvidia") << "(display) labelSetActive: " << labelSetActive;
  MITK_INFO("nvidia") << "(display) labelActive: " << labelActive;
  MITK_INFO("nvidia") << "(display) labelName: " << labelName;
  MITK_INFO("nvidia") << "(display) labelColor: " << labelColor;

  std::string labelNameDeepgrow = labelName + "Deepgrow";

  mitk::DataNode::Pointer newNode = mitk::DataNode::New();
  newNode->SetProperty("binary", mitk::BoolProperty::New(true));
  newNode->SetProperty("name", mitk::StringProperty::New(labelNameDeepgrow));
  newNode->SetProperty("color", mitk::ColorProperty::New(labelColor));
  newNode->SetProperty("opacity", mitk::FloatProperty::New(0));
  newNode->SetProperty("helper object", mitk::BoolProperty::New(true));

  // new image mask
  auto reader = ImageReaderType::New();
  reader->SetFileName(tmpResultFileName);
  reader->Update();
  auto itkResultImage = reader->GetOutput();

  // Record the just-created layer ID
  unsigned int layerTotal = labelSetImage->GetNumberOfLayers();
  unsigned int layerToReplace = labelSetImage->GetActiveLayer();
  MITK_INFO("nvidia") << "Total Layers: " << layerTotal << "; Layer To Replace: " << layerToReplace;

  mitk::Image::Pointer resultImage;
  auto tempLayerImage = labelSetImage->GetLayerImage(layerToReplace);
  if (tempLayerImage) {
    typename LabelImageType::Pointer itkExistingImage;
    mitk::CastToItkImage(tempLayerImage, itkExistingImage);

    MITK_INFO("nvidia") << "++++ Result Image: " << itkResultImage->GetLargestPossibleRegion();
    MITK_INFO("nvidia") << "++++ Existing Image: " << itkExistingImage->GetLargestPossibleRegion();

    auto inputRegion = itkResultImage->GetBufferedRegion();
    auto inputSize = inputRegion.GetSize();
    inputSize[2] = 1;  // we extract along z direction

    auto inputIndex = inputRegion.GetIndex();
    inputIndex[2] = sliceIndex;

    typename LabelImageType::RegionType desiredRegion;
    desiredRegion.SetSize(inputSize);
    desiredRegion.SetIndex(inputIndex);

    itk::ImageRegionIterator<LabelImageType> imageIterator(itkExistingImage, desiredRegion);
    while (!imageIterator.IsAtEnd()) {
      imageIterator.Set(0);
      ++imageIterator;
    }
    MITK_INFO("nvidia") << "Current Slice Reset: " << sliceIndex;


    using AddImageFilterType = itk::AddImageFilter<LabelImageType, LabelImageType>;
    auto addFilter = AddImageFilterType::New();

    addFilter->SetInput1(itkResultImage);
    addFilter->SetInput2(itkExistingImage);
    addFilter->Update();


    auto mergedImage = addFilter->GetOutput();
    mitk::CastToMitkImage(mergedImage, resultImage);
    MITK_INFO("nvidia") << "++++ Merged Result Image: " << mergedImage->GetLargestPossibleRegion();
  } else {
    mitk::CastToMitkImage(itkResultImage, resultImage);
  }

  newNode->SetData(resultImage);

  // delete the old image, if there was one:
  mitk::DataNode::Pointer binaryNode = m_ToolManager->GetDataStorage()->GetNamedNode(labelNameDeepgrow);
  m_ToolManager->GetDataStorage()->Remove(binaryNode);

  m_ToolManager->GetDataStorage()->Add(newNode, m_ToolManager->GetWorkingData(0));
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();


  // labelSetImage can only push a new layer at the end, so need to reload whole image for change
  for (unsigned int layerID = 0; layerID < layerTotal; layerID++) {
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

