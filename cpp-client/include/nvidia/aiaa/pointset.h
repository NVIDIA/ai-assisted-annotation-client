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

#include <vector>
#include <string>

namespace nvidia {
namespace aiaa {

////////////
// Point3DSet //
////////////

/*!
 @brief AIAA Point3DSet
 */

struct AIAA_CLIENT_API Point3DSet {
	typedef std::vector<int> Point3D;

	/// Array of 3D Points to represent [[x,y,z]+]
	std::vector<Point3D> points;

	/// Checks if points is empty
	bool empty() const;

	/// Count of points
	size_t size() const;

	/// Append new Point3D to points list
	void push_back(Point3D point);

	/*!
	 @brief create Model from JSON String
	 @param[in] json  JSON String

	 Example:
	 @code
	 [[70,172,86],[105,161,180],[125,147,164],[56,174,124],[91,119,143],[77,219,120]]
	 @endcode

	 @return Point3DSet object
	 */
	static Point3DSet fromJson(const std::string &json);

	/*!
	 @brief convert Point3DSet to JSON String
	 @param[in] space  If space > 0; then JSON string will be formatted accordingly
	 @return JSON String
	 */
	std::string toJson(int space = 0) const;
};

}
}

AIAA_CLIENT_API std::ostream& operator<<(std::ostream&, const nvidia::aiaa::Point3DSet&);
