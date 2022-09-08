/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <AEGetWhiteListResponse.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>
#include <IAEMessage.h>

AEGetWhiteListResponse::AEGetWhiteListResponse()
    :m_response(NULL)
{
}

AEGetWhiteListResponse::AEGetWhiteListResponse(aesm::message::Response::GetWhiteListResponse& response)
    :m_response(NULL)
{
    m_response = new aesm::message::Response::GetWhiteListResponse(response);
}

AEGetWhiteListResponse::AEGetWhiteListResponse(uint32_t errorCode, uint32_t whiteListLength, const uint8_t* whiteList)
    :m_response(NULL)
{
    m_response = new aesm::message::Response::GetWhiteListResponse();
    m_response->set_errorcode(errorCode);
    if (whiteListLength!= 0 && whiteList != NULL)
        m_response->set_white_list(whiteList, whiteListLength);
}

AEGetWhiteListResponse::AEGetWhiteListResponse(const AEGetWhiteListResponse& other)
    :m_response(NULL)
{
    if (other.m_response != NULL)
        m_response = new aesm::message::Response::GetWhiteListResponse(*other.m_response);
}

AEGetWhiteListResponse::~AEGetWhiteListResponse()
{
    ReleaseMemory();
}

void AEGetWhiteListResponse::ReleaseMemory()
{
   if (m_response != NULL)
    {
        delete m_response;
        m_response = NULL;
    }
}

AEMessage* AEGetWhiteListResponse::serialize()
{
    AEMessage *ae_msg = NULL;
    aesm::message::Response msg;
    if (check())
    {
        aesm::message::Response::GetWhiteListResponse* mutableResp = msg.mutable_getwhitelistres();
        mutableResp->CopyFrom(*m_response);

        if (msg.ByteSize() <= INT_MAX) {
            ae_msg = new AEMessage;
            ae_msg->size = (unsigned int)msg.ByteSize();
            ae_msg->data = new char[ae_msg->size];
            msg.SerializeToArray(ae_msg->data, ae_msg->size);
        }
    }
    return ae_msg;
}

bool AEGetWhiteListResponse::inflateWithMessage(AEMessage* message)
{
    aesm::message::Response msg;
    msg.ParseFromArray(message->data, message->size);
    if (msg.has_getwhitelistres() == false)
        return false;

    ReleaseMemory();
    m_response = new aesm::message::Response::GetWhiteListResponse(msg.getwhitelistres());
    return true;
}

bool AEGetWhiteListResponse::GetValues(uint32_t* errorCode, uint32_t whiteListLength,uint8_t* whiteList) const
{
    if (m_response->has_white_list() && whiteList != NULL)
    {
        if (m_response->white_list().size() <= whiteListLength)
            memcpy(whiteList, m_response->white_list().c_str(), m_response->white_list().size());
        else
            return false;
    }

    *errorCode = m_response->errorcode();
    return true;
}


AEGetWhiteListResponse& AEGetWhiteListResponse::operator=(const AEGetWhiteListResponse& other)
{
    if (this == &other)
        return * this;

    ReleaseMemory();
    if (other.m_response != NULL)
    {
        m_response = new aesm::message::Response::GetWhiteListResponse(*other.m_response);
    }    return *this;
}

bool AEGetWhiteListResponse::check()
{
    if (m_response == NULL)
        return false;
    return m_response->IsInitialized();
}
