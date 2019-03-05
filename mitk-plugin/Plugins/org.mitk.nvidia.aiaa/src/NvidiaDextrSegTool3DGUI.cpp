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

#include <NvidiaDextrSegTool3DGUI.h>
#include <ui_NvidiaDextrSegTool3DGUI.h>
#include <internal/QmitkNvidiaAIAAPreferencePage.h>
#include <berryIPreferences.h>
#include <berryIPreferencesService.h>
#include <berryPlatform.h>

MITK_TOOL_GUI_MACRO(NVIDIA_AIAA_EXPORT, NvidiaDextrSegTool3DGUI, "")

NvidiaDextrSegTool3DGUI::NvidiaDextrSegTool3DGUI() : m_Ui(new Ui::NvidiaDextrSegTool3DGUI)
{
  m_Ui->setupUi(this);

  connect(m_Ui->clearPointsBtn, SIGNAL(clicked()), this, SLOT(OnClearPoints()));
  connect(m_Ui->confirmPointsBtn, SIGNAL(clicked()), this, SLOT(OnConfirmPoints()));

  connect(this, SIGNAL(NewToolAssociated(mitk::Tool *)), this, SLOT(OnNewToolAssociated(mitk::Tool *)));
}

NvidiaDextrSegTool3DGUI::~NvidiaDextrSegTool3DGUI() {
}

void NvidiaDextrSegTool3DGUI::OnClearPoints() {
  if (m_NvidiaDextrSegTool3D.IsNotNull()) {
    m_NvidiaDextrSegTool3D->ClearPoints();
  }
}

void NvidiaDextrSegTool3DGUI::updateConfigs() {
  if (m_NvidiaDextrSegTool3D.IsNotNull()) {
    auto preferencesService = berry::Platform::GetPreferencesService();
    auto systemPreferences = preferencesService->GetSystemPreferences();
    auto preferences = systemPreferences->Node("/org.mitk.preferences.nvidia.aiaa");
    auto serverURI = preferences->Get(QmitkNvidiaAIAAPreferencePage::SERVER_URI, QmitkNvidiaAIAAPreferencePage::DEFAULT_SERVER_URI);
    auto serverTimeout = preferences->GetInt(QmitkNvidiaAIAAPreferencePage::SERVER_TIMEOUT, QmitkNvidiaAIAAPreferencePage::DEFAULT_SERVER_TIMEOUT);

    m_NvidiaDextrSegTool3D->SetServerURI(serverURI.toStdString(), serverTimeout);
  }
}

void NvidiaDextrSegTool3DGUI::OnConfirmPoints() {
  if (m_NvidiaDextrSegTool3D.IsNotNull()) {
    updateConfigs();
    m_NvidiaDextrSegTool3D->ConfirmPoints();
  }
}

void NvidiaDextrSegTool3DGUI::OnNewToolAssociated(mitk::Tool *tool) {
  m_NvidiaDextrSegTool3D = dynamic_cast<NvidiaDextrSegTool3D *>(tool);
}
