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

#include "../include/nvidia/aiaa/curlutils.h"
#include "../include/nvidia/aiaa/log.h"
#include "../include/nvidia/aiaa/exception.h"

#include <sstream>
#include <fstream>

#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTMLForm.h>
#include <Poco/Net/FilePartSource.h>
#include <Poco/Net/MultipartReader.h>
#include <Poco/Net/MessageHeader.h>

#include <Poco/StreamCopier.h>
#include <Poco/Path.h>
#include <Poco/URI.h>
#include <Poco/Exception.h>

namespace nvidia {
namespace aiaa {

const int CURL_CONNECT_TIMEOUT_IN_SEC = 5;
const std::string MULTI_PART_FIELD_PARAMS = "params";
const std::string MULTI_PART_FIELD_IMAGE = "image";

std::string CurlUtils::doMethod(const std::string &method, const std::string &uri, int timeoutInSec) {
  AIAA_LOG_DEBUG(method << ": " << uri << "; Timeout: " << timeoutInSec);
  std::stringstream response;

  try {
    Poco::URI u(uri);
    Poco::Net::HTTPClientSession session(u.getHost(), u.getPort());
    session.setKeepAlive(true);
    session.setTimeout(Poco::Timespan(CURL_CONNECT_TIMEOUT_IN_SEC, 0), Poco::Timespan(timeoutInSec, 0), Poco::Timespan(timeoutInSec, 0));

    std::string path(u.getPathAndQuery());
    if (path.empty()) {
      path = "/";
    }

    // send request
    AIAA_LOG_DEBUG("Request Path: " << path);
    Poco::Net::HTTPRequest req(method, path, Poco::Net::HTTPMessage::HTTP_1_1);
    session.sendRequest(req);

    // receive response
    Poco::Net::HTTPResponse res;
    std::istream &is = session.receiveResponse(res);
    AIAA_LOG_DEBUG("Status: " << res.getStatus() << "; Reason: " << res.getReason() << "; Content-type: " << res.getContentType());

    if (res.getStatus() == 440) {
      throw exception(exception::AIAA_SESSION_TIMEOUT, res.getReason().c_str());
    }
    if (res.getStatus() != 200) {
      throw exception(exception::AIAA_SERVER_ERROR, res.getReason().c_str());
    }

    Poco::StreamCopier::copyStream(is, response);
    AIAA_LOG_DEBUG("Received response from server: \n" << response.str());
  } catch (Poco::Exception &e) {
    AIAA_LOG_ERROR(e.displayText());
    throw exception(exception::AIAA_SERVER_ERROR, e.displayText().c_str());
  }

  return response.str();
}

std::string CurlUtils::doMethod(const std::string &method, const std::string &uri, const std::string &paramStr, const std::string &uploadFilePath,
                                int timeoutInSec) {
  AIAA_LOG_DEBUG(method << ": " << uri << "; Timeout: " << timeoutInSec);
  AIAA_LOG_DEBUG("ParamStr: " << paramStr);
  AIAA_LOG_DEBUG("UploadFilePath: " << uploadFilePath);
  std::stringstream response;

  try {
    Poco::URI u(uri);
    Poco::Net::HTTPClientSession session(u.getHost(), u.getPort());
    session.setKeepAlive(true);
    session.setTimeout(Poco::Timespan(CURL_CONNECT_TIMEOUT_IN_SEC, 0), Poco::Timespan(timeoutInSec, 0), Poco::Timespan(timeoutInSec, 0));

    std::string path(u.getPathAndQuery());
    if (path.empty()) {
      path = "/";
    }

    // send request
    AIAA_LOG_DEBUG("Request Path: " << path);
    Poco::Net::HTTPRequest req(method, path, Poco::Net::HTTPMessage::HTTP_1_0);

    Poco::Net::HTMLForm form;
    form.setEncoding(Poco::Net::HTMLForm::ENCODING_MULTIPART);
    form.set(MULTI_PART_FIELD_PARAMS, paramStr);
    if (!uploadFilePath.empty()) {
      form.addPart(MULTI_PART_FIELD_IMAGE, new Poco::Net::FilePartSource(uploadFilePath));
    }
    form.prepareSubmit(req);

    form.write(session.sendRequest(req));

    // receive response
    Poco::Net::HTTPResponse res;
    std::istream &is = session.receiveResponse(res);
    AIAA_LOG_DEBUG("Status: " << res.getStatus() << "; Reason: " << res.getReason() << "; Content-type: " << res.getContentType());

    if (res.getStatus() == 440) {
      throw exception(exception::AIAA_SESSION_TIMEOUT, res.getReason().c_str());
    }
    if (res.getStatus() != 200) {
      throw exception(exception::AIAA_SERVER_ERROR, res.getReason().c_str());
    }

    Poco::StreamCopier::copyStream(is, response);
    AIAA_LOG_DEBUG("Received response from server: \n" << response.str());
  } catch (Poco::Exception &e) {
    AIAA_LOG_ERROR(e.displayText());
    throw exception(exception::AIAA_SERVER_ERROR, e.displayText().c_str());
  }

  return response.str();
}

std::string CurlUtils::doMethod(const std::string &method, const std::string &uri, const std::string &paramStr, const std::string &uploadFilePath,
                                const std::string &resultFileName, int timeoutInSec) {
  AIAA_LOG_DEBUG(method << ": " << uri << "; Timeout: " << timeoutInSec);
  AIAA_LOG_DEBUG("ParamStr: " << paramStr);
  AIAA_LOG_DEBUG("UploadFilePath: " << uploadFilePath);
  AIAA_LOG_DEBUG("ResultFileName: " << resultFileName);
  std::string textReponse;

  try {
    Poco::URI u(uri);
    Poco::Net::HTTPClientSession session(u.getHost(), u.getPort());
    session.setKeepAlive(true);
    session.setTimeout(Poco::Timespan(CURL_CONNECT_TIMEOUT_IN_SEC, 0), Poco::Timespan(timeoutInSec, 0), Poco::Timespan(timeoutInSec, 0));

    std::string path(u.getPathAndQuery());
    if (path.empty()) {
      path = "/";
    }

    // send request
    AIAA_LOG_DEBUG("Request Path: " << path);
    Poco::Net::HTTPRequest req(method, path, Poco::Net::HTTPMessage::HTTP_1_0);

    Poco::Net::HTMLForm form;
    form.setEncoding(Poco::Net::HTMLForm::ENCODING_MULTIPART);
    form.set(MULTI_PART_FIELD_PARAMS, paramStr);
    if (!uploadFilePath.empty()) {
      form.addPart(MULTI_PART_FIELD_IMAGE, new Poco::Net::FilePartSource(uploadFilePath));
    }
    form.prepareSubmit(req);

    form.write(session.sendRequest(req));

    // receive response
    Poco::Net::HTTPResponse res;
    std::istream &is = session.receiveResponse(res);
    AIAA_LOG_DEBUG("Status: " << res.getStatus() << "; Reason: " << res.getReason() << "; Content-type: " << res.getContentType());

    std::stringstream response;
    Poco::StreamCopier::copyStream(is, response);

    if (res.getStatus() == 440) {
      throw exception(exception::AIAA_SESSION_TIMEOUT, response.str().c_str());
    }
    if (res.getStatus() != 200) {
      AIAA_LOG_INFO("Response: " << response.str());
      throw exception(exception::AIAA_RESPONSE_ERROR, (res.getReason() + " => " + response.str()).c_str());
    }

    if (res.getContentType().find("multipart") == std::string::npos) {
      if (!resultFileName.empty()) {
        AIAA_LOG_INFO("Expected Multipart Response but received: " << res.getContentType());
      }
      AIAA_LOG_DEBUG("Received response from server: \n" << response.str());

      textReponse = response.str();
      return textReponse;
    }

    Poco::Net::MultipartReader r(response);
    int i = 0;
    while (r.hasNextPart()) {
      Poco::Net::MessageHeader h;
      r.nextPart(h);

      bool isText = true;
      for (auto it = h.begin(); it != h.end(); it++) {
        AIAA_LOG_DEBUG("PART-" << i << ":: Header >>>> " << it->first << ": " << it->second);
        if (it->second.find("filename=\"") != std::string::npos || it->second.find("octet-stream") != std::string::npos) {
          isText = false;
        }
      }

      AIAA_LOG_DEBUG("PART-" << i << ":: Is Type Text: " << (isText ? "TRUE" : "FALSE"));

      std::istream &ii = r.stream();
      std::stringstream part;
      Poco::StreamCopier::copyStream(ii, part);

      if (isText) {
        AIAA_LOG_DEBUG("PART-" << i << ":: Data: " << part.str());
        textReponse = part.str();
      } else {
        AIAA_LOG_DEBUG("PART-" << i << ":: DataSize: " << part.str().size());

        std::ofstream file;
        file.open(resultFileName, std::ios::out | std::ios::binary | std::ios_base::trunc);

        std::string data = part.str();
        file.write(data.c_str(), data.size());
        file.flush();
      }
      i++;
    }
  } catch (Poco::Exception &e) {
    AIAA_LOG_ERROR(e.displayText());
    throw exception(exception::AIAA_SERVER_ERROR, e.displayText().c_str());
  }

  return textReponse;
}

std::string CurlUtils::encode(const std::string &param) {
  std::string encoded;
  Poco::URI::encode(param, "", encoded);
  return encoded;
}

}
}
