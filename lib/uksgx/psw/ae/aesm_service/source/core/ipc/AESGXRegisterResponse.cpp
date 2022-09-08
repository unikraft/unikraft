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

#include <AESGXRegisterResponse.h>

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <IAEMessage.h>

    AESGXRegisterResponse::AESGXRegisterResponse()
    :m_response(NULL)
{
}

AESGXRegisterResponse::AESGXRegisterResponse(aesm::message::Response::SGXRegisterResponse& response)
    :m_response(NULL)
{
    m_response = new aesm::message::Response::SGXRegisterResponse(response);
}

AESGXRegisterResponse::AESGXRegisterResponse(uint32_t errorCode)
    :m_response(NULL)
{
    m_response = new aesm::message::Response::SGXRegisterResponse();
    m_response->set_errorcode(errorCode);
}

AESGXRegisterResponse::AESGXRegisterResponse(const AESGXRegisterResponse& other)
    :m_response(NULL)
{
    if (other.m_response != NULL)
        m_response = new aesm::message::Response::SGXRegisterResponse(*other.m_response);
}

AESGXRegisterResponse::~AESGXRegisterResponse()
{
    ReleaseMemory();
}

void AESGXRegisterResponse::ReleaseMemory()
{
    if (m_response != NULL)
    {
        delete m_response;
        m_response = NULL;
    }
}

AEMessage* AESGXRegisterResponse::serialize()
{
    AEMessage *ae_msg = NULL;

    aesm::message::Response msg;
    if (check())
    {
        aesm::message::Response::SGXRegisterResponse* mutableRes = msg.mutable_sgxregisterres();
        mutableRes->CopyFrom(*m_response);

        if (msg.ByteSize() <= INT_MAX) {
            ae_msg = new AEMessage;
            ae_msg->size = (unsigned int)msg.ByteSize();
            ae_msg->data = new char[ae_msg->size];
            msg.SerializeToArray(ae_msg->data, ae_msg->size);
        }
    }
    return ae_msg;
}

bool AESGXRegisterResponse::inflateWithMessage(AEMessage* message)
{
    aesm::message::Response msg;
    msg.ParseFromArray(message->data, message->size);
    if (msg.has_sgxregisterres() == false)
        return false;

    //this is an AESGXRegisterResponse
    ReleaseMemory();
    m_response = new aesm::message::Response::SGXRegisterResponse(msg.sgxregisterres());
    return true;
}

bool AESGXRegisterResponse::GetValues(uint32_t* errorCode) const
{
    *errorCode = m_response->errorcode(); 
    return true;
}

AESGXRegisterResponse& AESGXRegisterResponse::operator=(const AESGXRegisterResponse& other)
{
    if (this == &other)
        return * this;

    ReleaseMemory();
    if (other.m_response != NULL)
    {
        m_response = new aesm::message::Response::SGXRegisterResponse(*other.m_response);
    }
    return *this;
}

bool AESGXRegisterResponse::check()
{
    if (m_response == NULL)
        return false;
    return m_response->IsInitialized();
}
