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

#include "PluginActivator.h"
#include "QmitkNvidiaAIAAPreferencePage.h"

#include <Poco/Net/IPAddress.h>

void PluginActivator::start(ctkPluginContext* context) {
  // Dummy code to force linkage to PocoNet (runtime issue with GCC 7.3)
  Poco::Net::IPAddress forcePocoNetLinkage;
  forcePocoNetLinkage.af();

  BERRY_REGISTER_EXTENSION_CLASS(QmitkNvidiaAIAAPreferencePage, context)
}

void PluginActivator::stop(ctkPluginContext*) {
}
