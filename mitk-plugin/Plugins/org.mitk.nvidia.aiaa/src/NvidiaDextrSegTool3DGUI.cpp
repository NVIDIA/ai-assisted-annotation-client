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
  connect(m_Ui->autoSegmentationBtn, SIGNAL(clicked()), this, SLOT(OnAutoSegmentation()));

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
    auto filterByLabel = preferences->GetBool(QmitkNvidiaAIAAPreferencePage::FILTER_BY_LABEL, QmitkNvidiaAIAAPreferencePage::DEFAULT_FILTER_BY_LABEL);

    m_NvidiaDextrSegTool3D->SetServerURI(serverURI.toStdString(), serverTimeout, filterByLabel);

    // Update ComboBox for selecting the model
    m_Ui->segmentationCombo->clear();
    m_Ui->annotationCombo->clear();

    std::map<std::string, std::string> seg, ann;
    m_NvidiaDextrSegTool3D->GetModelInfo(seg, ann);

    for (auto it = seg.begin(); it != seg.end(); it++) {
      m_Ui->segmentationCombo->addItem(QString::fromUtf8(it->first.c_str()));
      m_Ui->segmentationCombo->setItemData(m_Ui->segmentationCombo->count() - 1, QVariant(it->second.c_str()), Qt::ToolTipRole);
    }

    for (auto it = ann.begin(); it != ann.end(); it++) {
      m_Ui->annotationCombo->addItem(QString::fromUtf8(it->first.c_str()));
      m_Ui->annotationCombo->setItemData(m_Ui->annotationCombo->count() - 1, QVariant(it->second.c_str()), Qt::ToolTipRole);
    }

    m_Ui->autoSegmentationBtn->setEnabled(m_Ui->segmentationCombo->count() > 0);
    m_Ui->confirmPointsBtn->setEnabled(m_Ui->annotationCombo->count() > 0);

    m_Ui->modelsLabel->setText("<p>Change server URI in Nvidia AIAA preferences (Ctrl+P).</p>"
        "<p><a href='" + serverURI + "/v1/models'>Click here</a> to see Details of available Models</p>");
  }
}

void NvidiaDextrSegTool3DGUI::OnAutoSegmentation() {
  if (m_NvidiaDextrSegTool3D.IsNotNull()) {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    try {
      m_NvidiaDextrSegTool3D->RunAutoSegmentation(m_Ui->segmentationCombo->currentText().toStdString());
    } catch(...) {
    }
    QApplication::restoreOverrideCursor();
  }
}

void NvidiaDextrSegTool3DGUI::OnConfirmPoints() {
  if (m_NvidiaDextrSegTool3D.IsNotNull()) {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    try {
      m_NvidiaDextrSegTool3D->ConfirmPoints(m_Ui->annotationCombo->currentText().toStdString());
    } catch(...) {
    }
    QApplication::restoreOverrideCursor();
  }
}

void NvidiaDextrSegTool3DGUI::OnNewToolAssociated(mitk::Tool *tool) {
  m_NvidiaDextrSegTool3D = dynamic_cast<NvidiaDextrSegTool3D *>(tool);
  updateConfigs();
}
