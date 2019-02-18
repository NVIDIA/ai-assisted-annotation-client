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

#ifndef NvidiaSmartPolySegTool2D_h
#define NvidiaSmartPolySegTool2D_h

#include <MitkNvidiaAIAAModuleExports.h>
#include <mitkAutoSegmentationTool.h>

#include <mitkDataNode.h>
#include <mitkPointSet.h>

#include "NvidiaPointSetDataInteractor.h"
#include <nvidia/aiaa/polygon.h>

namespace us {
class ModuleResource;
}

class MITKNVIDIAAIAAMODULE_EXPORT NvidiaSmartPolySegTool2D : public mitk::AutoSegmentationTool
{
public:
  mitkClassMacro(NvidiaSmartPolySegTool2D, AutoSegmentationTool);
  itkFactorylessNewMacro(Self);

  us::ModuleResource GetIconResource() const override;
  const char *GetName() const override;
  const char **GetXPM() const override;

  void Activated() override;
  void Deactivated() override;

  void ClearPoints();
  void PolygonFix();
  void Mask2Polygon();

  void SetCurrentSlice(unsigned int slice);
  void SetServerURI(const std::string &serverURI);
  void SetNeighborhoodSize(int neighborhoodSize);
  void SetFlipPoly(bool flipPoly);

protected:
  NvidiaSmartPolySegTool2D();
  ~NvidiaSmartPolySegTool2D() override;

  std::string create2DSliceImage();
  void displayResult(const std::string &tmpResultFileName);

private:
  std::string m_AIAAServerUri;
  int m_NeighborhoodSize;
  bool m_FlipPoly;

  mitk::PointSet::Pointer m_PointSetPolygon;
  mitk::DataNode::Pointer m_PointSetPolygonNode;
  NvidiaPointSetDataInteractor::Pointer m_PointInteractor;

  mitk::DataNode::Pointer m_WorkingData;
  mitk::DataNode::Pointer m_OriginalImageNode;
  mitk::DataNode::Pointer m_ResultNode;

  std::vector<int> m_MinIndex;
  std::vector<int> m_MaxIndex;
  unsigned int m_currentSlice;

  unsigned int *m_imageSize;
  mitk::BaseGeometry::Pointer m_imageGeometry;

  nvidia::aiaa::PolygonsList m_polygonsList;
};

#endif
