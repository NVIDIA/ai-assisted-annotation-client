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

#ifndef NvidiaDextrSegTool3DGUI_h
#define NvidiaDextrSegTool3DGUI_h

#include <org_mitk_nvidia_aiaa_Export.h>

#include <QScopedPointer>
#include <QmitkToolGUI.h>

#include "NvidiaDextrSegTool3D.h"

namespace Ui {
class NvidiaDextrSegTool3DGUI;
}

class NVIDIA_AIAA_EXPORT NvidiaDextrSegTool3DGUI : public QmitkToolGUI
{
  Q_OBJECT

public:
  mitkClassMacro(NvidiaDextrSegTool3DGUI, QmitkToolGUI)
  itkFactorylessNewMacro(Self)

protected slots:
  void OnClearPoints();
  void OnConfirmPoints();
  void OnAutoSegmentation();
  void OnNewToolAssociated(mitk::Tool *);

protected:
  NvidiaDextrSegTool3DGUI();
  ~NvidiaDextrSegTool3DGUI() override;
  void updateConfigs();

private:
  QScopedPointer<Ui::NvidiaDextrSegTool3DGUI> m_Ui;
  NvidiaDextrSegTool3D::Pointer m_NvidiaDextrSegTool3D;
};

#endif
