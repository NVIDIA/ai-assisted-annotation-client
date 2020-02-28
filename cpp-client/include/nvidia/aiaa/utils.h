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

#include "common.h"
#include "pointset.h"
#include "exception.h"

#include <string>
#include <vector>
#include <locale>
#include <sstream>

namespace nvidia {
namespace aiaa {

/*!
 @brief AIAA Utils
 */

class AIAA_CLIENT_API Utils {
 public:
  /*!
   @brief compare if 2 strings are same with ignore-case
   @param[in] a left string
   @param[in] b right string
   @retval true if a == b
   @retval false if a != b
   */
  static bool iequals(const std::string &a, const std::string &b);

  /*!
   @brief convert string to lower case
   @param[in] str input string
   @return lower case version of input string
   */
  static std::string to_lower(std::string str);

  /*!
   @brief get temporary filename
   @return full path for temporary file
   */
  static std::string tempfilename();

  /*!
   @brief split the sting
   @param[in] str input string
   @param[in] delim delimiter character
   @return vector of split strings
   */
  static std::vector<std::string> split(const std::string &str, char delim);

  /*!
   @brief 3D point
   @param[in] str input string
   @param[in] delim delimiter character
   @return Point which represents x,y,z
   */
  static Point stringToPoint(const std::string &str, char delim);

  /*!
   @brief Lexical Cast with locale support
   @param[in] in input string/numeric
   @param[in] loc local by default "C"
   @return numberic/string based for the given locale
   */
  template<typename T, typename U>
  static auto lexical_cast(U const &in, const std::locale &loc = std::locale::classic()) {
    std::stringstream istr;
    istr.imbue(loc);
    istr << in;

    std::string str = istr.str();  // save string in case of exception

    T val;
    istr >> val;

    if (istr.fail()) {
      throw exception(exception::INVALID_ARGS_ERROR, str.c_str());
    }

    return val;
  }
};

}
}
