/*===================================================================

 The Medical Imaging Interaction Toolkit (MITK)

 Copyright (c) German Cancer Research Center,
 Division of Medical and Biological Informatics.
 All rights reserved.

 This software is distributed WITHOUT ANY WARRANTY; without
 even the implied warranty of MERCHANTABILITY or FITNESS FOR
 A PARTICULAR PURPOSE.

 See LICENSE.txt or http://www.mitk.org for details.

 ===================================================================*/

#include "QmitkNvidiaAIAAPreferencePage.h"

#include <berryIPreferencesService.h>
#include <berryPlatform.h>

#include <ui_QmitkNvidiaAIAAPreferencePage.h>

const QString QmitkNvidiaAIAAPreferencePage::SERVER_URI = "server uri";
const QString QmitkNvidiaAIAAPreferencePage::SERVER_TIMEOUT = "server timeout";
const QString QmitkNvidiaAIAAPreferencePage::FILTER_BY_LABEL = "filter models by label";
const QString QmitkNvidiaAIAAPreferencePage::NEIGHBORHOOD_SIZE = "neighborhood size";

const QString QmitkNvidiaAIAAPreferencePage::DEFAULT_SERVER_URI = "http://0.0.0.0:5000";
const int QmitkNvidiaAIAAPreferencePage::DEFAULT_SERVER_TIMEOUT = 60;
const bool QmitkNvidiaAIAAPreferencePage::DEFAULT_FILTER_BY_LABEL = true;
const int QmitkNvidiaAIAAPreferencePage::DEFAULT_NEIGHBORHOOD_SIZE = 1;

QmitkNvidiaAIAAPreferencePage::QmitkNvidiaAIAAPreferencePage()
    : m_Widget(nullptr),
      m_Ui(new Ui::QmitkNvidiaAIAAPreferencePage),
      m_Preferences(
          berry::Platform::GetPreferencesService()->GetSystemPreferences()->Node("/org.mitk.preferences.nvidia.aiaa")) {
}

QmitkNvidiaAIAAPreferencePage::~QmitkNvidiaAIAAPreferencePage() {
  delete m_Ui;
}

void QmitkNvidiaAIAAPreferencePage::Init(berry::IWorkbench::Pointer) {
}

bool QmitkNvidiaAIAAPreferencePage::PerformOk() {
  m_Preferences->Put(SERVER_URI, m_Ui->serverURILineEdit->text());
  m_Preferences->PutInt(SERVER_TIMEOUT, m_Ui->serverTimeoutSpinBox->value());
  m_Preferences->PutBool(FILTER_BY_LABEL, m_Ui->modelFilterCheckBox->isChecked());
  m_Preferences->PutInt(NEIGHBORHOOD_SIZE, m_Ui->neighborhoodSizeSpinBox->value());

  return true;
}

void QmitkNvidiaAIAAPreferencePage::PerformCancel() {
}

void QmitkNvidiaAIAAPreferencePage::Update() {
  m_Ui->serverURILineEdit->setText(m_Preferences->Get(SERVER_URI, DEFAULT_SERVER_URI));
  m_Ui->serverTimeoutSpinBox->setValue(m_Preferences->GetInt(SERVER_TIMEOUT, DEFAULT_SERVER_TIMEOUT));
  m_Ui->modelFilterCheckBox->setChecked(m_Preferences->GetBool(FILTER_BY_LABEL, DEFAULT_FILTER_BY_LABEL));
  m_Ui->neighborhoodSizeSpinBox->setValue(m_Preferences->GetInt(NEIGHBORHOOD_SIZE, DEFAULT_NEIGHBORHOOD_SIZE));
}

void QmitkNvidiaAIAAPreferencePage::CreateQtControl(QWidget* parent) {
  m_Widget = new QWidget(parent);
  m_Ui->setupUi(m_Widget);

  this->Update();
}

QWidget* QmitkNvidiaAIAAPreferencePage::GetQtControl() const {
  return m_Widget;
}
