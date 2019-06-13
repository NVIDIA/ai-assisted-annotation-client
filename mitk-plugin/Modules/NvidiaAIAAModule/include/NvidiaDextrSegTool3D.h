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

#ifndef NvidiaDextrSegTool3D_h
#define NvidiaDextrSegTool3D_h

#include <mitkAutoSegmentationTool.h>
#include <mitkDataNode.h>
#include <mitkPointSet.h>
#include <mitkSinglePointDataInteractor.h>

#include <itkImage.h>
#include <nvidia/aiaa/client.h>

#include <MitkNvidiaAIAAModuleExports.h>
#include <map>

namespace us {
class ModuleResource;
}

class MITKNVIDIAAIAAMODULE_EXPORT NvidiaDextrSegTool3D : public mitk::AutoSegmentationTool
{
public:
  mitkClassMacro(NvidiaDextrSegTool3D, AutoSegmentationTool);
  itkFactorylessNewMacro(Self);

  us::ModuleResource GetIconResource() const override;
  const char *GetName() const override;
  const char **GetXPM() const override;

  void SetServerURI(const std::string &serverURI, const int serverTimeout);
  void GetModelInfo(std::map<std::string, std::string>& seg, std::map<std::string, std::string>& ann);
  void ClearPoints();
  void ConfirmPoints(const std::string &modelName);
  void RunAutoSegmentation(const std::string &modelName);

protected:
  NvidiaDextrSegTool3D();
  ~NvidiaDextrSegTool3D() override;

  void Activated() override;
  void Deactivated() override;

private:
  std::string m_AIAAServerUri;
  int m_AIAAServerTimeout;
  nvidia::aiaa::ModelList m_AIAAModelList;
  std::string m_AIAACurrentModelName;

  mitk::PointSet::Pointer m_PointSet;
  mitk::DataNode::Pointer m_PointSetNode;
  mitk::PointSetDataInteractor::Pointer m_SeedPointInteractor;

  mitk::DataNode::Pointer m_WorkingData;
  mitk::DataNode::Pointer m_OriginalImageNode;

  template <typename TPixel, unsigned int VImageDimension>
  void ItkImageProcessDextr3D(itk::Image<TPixel, VImageDimension> *itkImage, mitk::BaseGeometry *imageGeometry);

  template <typename TPixel, unsigned int VImageDimension>
  void ItkImageProcessAutoSegmentation(itk::Image<TPixel, VImageDimension> *itkImage, mitk::BaseGeometry *imageGeometry);

  template <typename TPixel, unsigned int VImageDimension>
  void addToPointSet(const nvidia::aiaa::PointSet& pointSet, mitk::BaseGeometry *imageGeometry);

  template <typename TPixel, unsigned int VImageDimension>
  nvidia::aiaa::PointSet getPointSet(mitk::BaseGeometry *imageGeometry);

  template <typename TPixel, unsigned int VImageDimension>
  void boundingBoxRender(const std::string &tmpResultFileName, const std::string &organName);

  template <typename TPixel, unsigned int VImageDimension>
  void displayResult(const std::string &tmpResultFileName);
};

#endif
