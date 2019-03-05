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

#ifndef QmitkNvidiaAIAAPreferencePage_h
#define QmitkNvidiaAIAAPreferencePage_h

#include <berryIQtPreferencePage.h>

namespace Ui {
class QmitkNvidiaAIAAPreferencePage;
}

class QmitkNvidiaAIAAPreferencePage : public QObject, public berry::IQtPreferencePage {
  Q_OBJECT
  Q_INTERFACES(berry::IPreferencePage)

 public:
  static const QString SERVER_URI;
  static const QString SERVER_TIMEOUT;
  static const QString NEIGHBORHOOD_SIZE;
  static const QString FLIP_POLY;

  static const QString DEFAULT_SERVER_URI;
  static const int DEFAULT_SERVER_TIMEOUT;
  static const int DEFAULT_NEIGHBORHOOD_SIZE;
  static const bool DEFAULT_FLIP_POLY;

  QmitkNvidiaAIAAPreferencePage();
  ~QmitkNvidiaAIAAPreferencePage();

  void Init(berry::IWorkbench::Pointer workbench) override;
  bool PerformOk() override;
  void PerformCancel() override;
  void Update() override;

  void CreateQtControl(QWidget* parent) override;
  QWidget* GetQtControl() const override;

 private:
  QWidget* m_Widget;
  Ui::QmitkNvidiaAIAAPreferencePage* m_Ui;
  berry::IPreferences::Pointer m_Preferences;
};

#endif
