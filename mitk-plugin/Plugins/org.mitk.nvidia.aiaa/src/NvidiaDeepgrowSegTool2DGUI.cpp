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

#include <NvidiaDeepgrowSegTool2DGUI.h>
#include <ui_NvidiaDeepgrowSegTool2DGUI.h>
#include <internal/QmitkNvidiaAIAAPreferencePage.h>
#include <mitkTool.h>
#include <berryIPreferences.h>
#include <berryIPreferencesService.h>
#include <berryPlatform.h>

MITK_TOOL_GUI_MACRO(NVIDIA_AIAA_EXPORT, NvidiaDeepgrowSegTool2DGUI, "")

NvidiaDeepgrowSegTool2DGUI::NvidiaDeepgrowSegTool2DGUI()
  : m_Ui(new Ui::NvidiaDeepgrowSegTool2DGUI) {

  m_Ui->setupUi(this);

  connect(this, SIGNAL(NewToolAssociated(mitk::Tool *)), this, SLOT(OnNewToolAssociated(mitk::Tool *)));
  connect(m_Ui->clearPointsBtn, SIGNAL(clicked()), this, SLOT(OnClearPoints()));
}

NvidiaDeepgrowSegTool2DGUI::~NvidiaDeepgrowSegTool2DGUI() {
  if (m_NvidiaDeepgrowSegTool2D.IsNull()) {
    return;
  }

  if (m_NvidiaDeepgrowSegTool2D->GetPointSetNode().IsNotNull()) {
    m_NvidiaDeepgrowSegTool2D->GetPointSetNode()->GetData()->RemoveObserver(m_PointSetAddObserverTag);
    m_NvidiaDeepgrowSegTool2D->GetPointSetNode()->GetData()->RemoveObserver(m_PointSetMoveObserverTag);
  }
}

void NvidiaDeepgrowSegTool2DGUI::OnNewToolAssociated(mitk::Tool *tool) {
  m_NvidiaDeepgrowSegTool2D = dynamic_cast<NvidiaDeepgrowSegTool2D*>(tool);
  if (m_NvidiaDeepgrowSegTool2D.IsNull()) {
    return;
  }

  m_InputImageNode = m_NvidiaDeepgrowSegTool2D->GetReferenceData();

  itk::SimpleMemberCommand<NvidiaDeepgrowSegTool2DGUI>::Pointer pointAddedCommand = itk::SimpleMemberCommand<NvidiaDeepgrowSegTool2DGUI>::New();
  pointAddedCommand->SetCallbackFunction(this, &NvidiaDeepgrowSegTool2DGUI::OnPointAdded);

  m_PointSetAddObserverTag = m_NvidiaDeepgrowSegTool2D->GetPointSetNode()->GetData()->AddObserver(mitk::PointSetAddEvent(), pointAddedCommand);
  m_PointSetMoveObserverTag = m_NvidiaDeepgrowSegTool2D->GetPointSetNode()->GetData()->AddObserver(mitk::PointSetMoveEvent(), pointAddedCommand);

  UpdateConfigs();
}

void NvidiaDeepgrowSegTool2DGUI::OnClearPoints() {
  if (m_NvidiaDeepgrowSegTool2D.IsNotNull()) {
    m_NvidiaDeepgrowSegTool2D->ClearPoints();
  }
}

void NvidiaDeepgrowSegTool2DGUI::OnPointAdded() {
  if (m_NvidiaDeepgrowSegTool2D.IsNull()) {
    return;
  }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  try {
    bool background = m_Ui->backgroundBtn->isChecked();
    m_NvidiaDeepgrowSegTool2D->PointAdded(m_InputImageNode, background);
    m_NvidiaDeepgrowSegTool2D->RunDeepGrow(m_Ui->deepgrowCombo->currentText().toStdString(), m_InputImageNode);
  } catch (...) {
  }

  QApplication::restoreOverrideCursor();
}

void NvidiaDeepgrowSegTool2DGUI::UpdateConfigs() {
  if (m_NvidiaDeepgrowSegTool2D.IsNull()) {
    return;
  }

  auto preferencesService = berry::Platform::GetPreferencesService();
  auto systemPreferences = preferencesService->GetSystemPreferences();
  auto preferences = systemPreferences->Node("/org.mitk.preferences.nvidia.aiaa");

  auto serverURI = preferences->Get(QmitkNvidiaAIAAPreferencePage::SERVER_URI, QmitkNvidiaAIAAPreferencePage::DEFAULT_SERVER_URI);
  auto serverTimeout = preferences->GetInt(QmitkNvidiaAIAAPreferencePage::SERVER_TIMEOUT, QmitkNvidiaAIAAPreferencePage::DEFAULT_SERVER_TIMEOUT);

  m_NvidiaDeepgrowSegTool2D->SetServerURI(serverURI.toStdString(), serverTimeout);

  m_Ui->deepgrowCombo->clear();

  std::map<std::string, std::string> deepgrow;
  m_NvidiaDeepgrowSegTool2D->GetModelInfo(deepgrow);

  for (auto it = deepgrow.begin(); it != deepgrow.end(); it++) {
    m_Ui->deepgrowCombo->addItem(QString::fromUtf8(it->first.c_str()));
    m_Ui->deepgrowCombo->setItemData(m_Ui->deepgrowCombo->count() - 1, QVariant(it->second.c_str()), Qt::ToolTipRole);
  }
}

