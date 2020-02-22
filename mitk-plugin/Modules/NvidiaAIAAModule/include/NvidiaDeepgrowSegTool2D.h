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

#ifndef NvidiaDeepgrowSegTool2D_h
#define NvidiaDeepgrowSegTool2D_h

#include <MitkNvidiaAIAAModuleExports.h>

#include <mitkAutoSegmentationTool.h>
#include <mitkCommon.h>
#include <mitkDataStorage.h>
#include <mitkPointSet.h>
#include <mitkPointSetDataInteractor.h>

#include <itkImage.h>
#include <nvidia/aiaa/model.h>
#include <nvidia/aiaa/pointset.h>

namespace us {
class ModuleResource;
}

class MITKNVIDIAAIAAMODULE_EXPORT NvidiaDeepgrowSegTool2D : public mitk::AutoSegmentationTool
{
public:
  mitkClassMacro(NvidiaDeepgrowSegTool2D, AutoSegmentationTool);
  itkFactorylessNewMacro(Self);

  us::ModuleResource GetIconResource() const override;

  bool CanHandle(mitk::BaseData *referenceData) const override;
  const char* GetName() const override;
  const char** GetXPM() const override;

  void Activated() override;
  void Deactivated() override;

  virtual mitk::DataNode::Pointer GetPointSetNode();
  mitk::DataNode* GetReferenceData();
  mitk::DataNode* GetWorkingData();
  mitk::DataStorage* GetDataStorage();

  void SetServerURI(const std::string &serverURI, const int serverTimeout);
  void GetModelInfo(std::map<std::string, std::string>& deepgrow);
  void PointAdded(mitk::DataNode::Pointer imageNode, bool background);
  void ClearPoints();

  void RunDeepGrow(const std::string &model, mitk::DataNode::Pointer imageNode);


 protected:
  NvidiaDeepgrowSegTool2D();
  ~NvidiaDeepgrowSegTool2D() override;

  template <typename TPixel, unsigned int VImageDimension>
  void ItkImageProcessRunDeepgrow(itk::Image<TPixel, VImageDimension> *itkImage, std::string imageNodeId);

  template <typename TPixel, unsigned int VImageDimension>
  void DisplayResult(const std::string &tmpResultFileName, const int sliceIndex);

  int GetCurrentSlice(const nvidia::aiaa::PointSet &points);
  nvidia::aiaa::PointSet GetPointsForCurrentSlice(const nvidia::aiaa::PointSet &points, int sliceIndex);

 private:
  mitk::PointSet::Pointer m_PointSet;
  mitk::PointSetDataInteractor::Pointer m_pointInteractor;
  mitk::DataNode::Pointer m_PointSetNode;

  std::map<std::string, std::string> m_AIAASessions;
  nvidia::aiaa::PointSet m_foregroundPoints;
  nvidia::aiaa::PointSet m_backgroundPoints;

  std::string m_AIAAServerUri;
  int m_AIAAServerTimeout;
  nvidia::aiaa::ModelList m_AIAAModelList;
  std::string m_AIAACurrentModelName;
};

#endif
