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

#include <stdexcept>
#include <string>

namespace nvidia {
namespace aiaa {

/*!
 @brief exception indicating a parse error

 This exception is thrown by the library when error occurs. These errors
 can occur during the deserialization of JSON text, CURL, as well
 as when using invalid Arguments.


 Exceptions have ids 1xx.

 name / id                      | description
 ------------------------------ | -------------------------
 nvidia.aiaa.error.101 | Failed to communicate to AIAA Server.
 nvidia.aiaa.error.102 | Failed to parse AIAA Server Response.
 nvidia.aiaa.error.103 | Failed to process ITK Operations.
 nvidia.aiaa.error.104 | Invalid Arguments.
 nvidia.aiaa.error.105 | System/Unknown Error.
 */

class exception: public std::exception {
public:
	enum errorType {
		AIAA_SERVER_ERROR = 101, RESPONSE_PARSE_ERROR = 102, ITK_PROCESS_ERROR = 103, INVALID_ARGS_ERROR = 104, SYSTEM_ERROR = 105
	};

	const std::string messages[5] = { "Failed to communicate to AIAA Server", "Failed to parse AIAA Server Response", "Failed to process ITK Operations", "Invalid Arguments",
			"System/Unknown Error" };

	/// returns the explanatory string
	const char* what() const noexcept override
	{
		return m.what();
	}

	/// the id of the exception
	errorType id = SYSTEM_ERROR;

	exception(errorType id_, const char* what_arg) :
			id(id_), m(what_arg) {
	}

	std::string name() const {
		return messages[id - AIAA_SERVER_ERROR];
	}

private:
	/// an exception object as storage for error messages
	std::runtime_error m;
};

}
}
