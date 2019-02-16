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

#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <chrono>
#include <iomanip>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

static std::string timestamp() {
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
#pragma warning(suppress:4996)
  ss << std::put_time(std::localtime(&in_time_t), "%H:%M:%S");
  return ss.str();
}

#ifndef AIAA_LOG_DEBUG_ENABLED
#define AIAA_LOG_DEBUG_ENABLED 1
#endif

#ifndef AIAA_LOG_INFO_ENABLED
#define AIAA_LOG_INFO_ENABLED 1
#endif

#ifndef AIAA_LOG_DEBUG
#if AIAA_LOG_DEBUG_ENABLED
#define AIAA_LOG_DEBUG(s) std::cout << timestamp() << " [DEBUG] [" << __FILENAME__ << ":" << __LINE__ << " - " << __func__ << "()] " << s << std::endl
#else
#define AIAA_LOG_DEBUG(s)
#endif
#endif

#ifndef AIAA_LOG_INFO
#if AIAA_LOG_INFO_ENABLED
#define AIAA_LOG_INFO(s) std::cout << timestamp() << " [INFO ] [" << __FILENAME__ << ":" << __LINE__ << " - " << __func__ << "()] " << s << std::endl
#else
#define AIAA_LOG_INFO(s)
#endif
#endif

#ifndef AIAA_LOG_WARN
#define AIAA_LOG_WARN(s) std::cerr << timestamp() << " [WARN ] [" << __FILENAME__ << ":" << __LINE__ << " - " << __func__ << "()] " << s << std::endl
#endif

#ifndef AIAA_LOG_ERROR
#define AIAA_LOG_ERROR(s) std::cerr << timestamp() << " [ERROR] [" << __FILENAME__ << ":" << __LINE__ << " - " << __func__ << "()] " << s << std::endl
#endif
