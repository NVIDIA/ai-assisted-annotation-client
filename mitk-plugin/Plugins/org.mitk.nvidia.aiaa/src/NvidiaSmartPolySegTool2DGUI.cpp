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

#include <NvidiaSmartPolySegTool2DGUI.h>
#include <ui_NvidiaSmartPolySegTool2DGUI.h>
#include <NvidiaDextrSegTool3DGUI.h>
#include <internal/QmitkNvidiaAIAAPreferencePage.h>
#include <berryIPreferences.h>
#include <berryIPreferencesService.h>
#include <berryPlatform.h>

MITK_TOOL_GUI_MACRO(NVIDIA_AIAA_EXPORT, NvidiaSmartPolySegTool2DGUI, "")

NvidiaSmartPolySegTool2DGUI::NvidiaSmartPolySegTool2DGUI() : m_Ui(new Ui::NvidiaSmartPolySegTool2DGUI)
{
  m_Ui->setupUi(this);
  connect(this, SIGNAL(NewToolAssociated(mitk::Tool *)), this, SLOT(OnNewToolAssociated(mitk::Tool *)));

  m_SliceIsConnected = false;
}

NvidiaSmartPolySegTool2DGUI::~NvidiaSmartPolySegTool2DGUI() {
}

void NvidiaSmartPolySegTool2DGUI::updateConfigs() {
  if (m_NvidiaSmartPolySegTool2D.IsNotNull()) {
    auto preferencesService = berry::Platform::GetPreferencesService();
    auto systemPreferences = preferencesService->GetSystemPreferences();
    auto preferences = systemPreferences->Node("/org.mitk.preferences.nvidia.aiaa");
    auto serverURI = preferences->Get(QmitkNvidiaAIAAPreferencePage::SERVER_URI, QmitkNvidiaAIAAPreferencePage::DEFAULT_SERVER_URI);
    auto serverTimeout = preferences->GetInt(QmitkNvidiaAIAAPreferencePage::SERVER_TIMEOUT, QmitkNvidiaAIAAPreferencePage::DEFAULT_SERVER_TIMEOUT);
    auto neighborhoodSize = preferences->GetInt(QmitkNvidiaAIAAPreferencePage::NEIGHBORHOOD_SIZE,
                                                QmitkNvidiaAIAAPreferencePage::DEFAULT_NEIGHBORHOOD_SIZE);
    auto flipPoly = preferences->GetBool(QmitkNvidiaAIAAPreferencePage::FLIP_POLY, QmitkNvidiaAIAAPreferencePage::DEFAULT_FLIP_POLY);

    m_NvidiaSmartPolySegTool2D->SetServerURI(serverURI.toStdString(), serverTimeout);
    m_NvidiaSmartPolySegTool2D->SetNeighborhoodSize(neighborhoodSize);
    m_NvidiaSmartPolySegTool2D->SetFlipPoly(flipPoly);
  }
}

void NvidiaSmartPolySegTool2DGUI::OnNewToolAssociated(mitk::Tool *tool) {
  m_NvidiaSmartPolySegTool2D = dynamic_cast<NvidiaSmartPolySegTool2D *>(tool);

  if (m_NvidiaSmartPolySegTool2D.IsNotNull()) {
    updateConfigs();

    mitk::BaseRenderer::Pointer renderer = mitk::BaseRenderer::GetInstance(mitk::BaseRenderer::GetRenderWindowByName("stdmulti.widget1"));
    if (renderer.IsNotNull() && !m_SliceIsConnected) {
      new QmitkStepperAdapter(this, renderer->GetSliceNavigationController()->GetSlice(), "stepper");
      m_SliceIsConnected = true;
    }

    // convert current segmentation mask to polygon
    m_NvidiaSmartPolySegTool2D->Mask2Polygon();
  }
}

void NvidiaSmartPolySegTool2DGUI::SetStepper(mitk::Stepper *stepper) {
  updateConfigs();
  this->m_SliceStepper = stepper;
}

void NvidiaSmartPolySegTool2DGUI::Refetch() {
  // event from image navigator received - time step has changed
  updateConfigs();
  m_NvidiaSmartPolySegTool2D->SetCurrentSlice(m_SliceStepper->GetPos());
}
