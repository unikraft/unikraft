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


#include <AEGetSupportedAttKeyIDNumResponse.h>

#include <stdlib.h>
#include <string.h>

#include <limits.h>
#include <IAEMessage.h>

AEGetSupportedAttKeyIDNumResponse::AEGetSupportedAttKeyIDNumResponse()
    :m_response(NULL)
{
}

AEGetSupportedAttKeyIDNumResponse::AEGetSupportedAttKeyIDNumResponse(aesm::message::Response::GetSupportedAttKeyIDNumResponse& response)
    :m_response(NULL)
{
    m_response = new aesm::message::Response::GetSupportedAttKeyIDNumResponse(response);
}

AEGetSupportedAttKeyIDNumResponse::AEGetSupportedAttKeyIDNumResponse(uint32_t errorCode, uint32_t att_key_id_num)
    :m_response(NULL)
{
    m_response = new aesm::message::Response::GetSupportedAttKeyIDNumResponse();
    m_response->set_errorcode(errorCode);
    m_response->set_att_key_id_num(att_key_id_num);
}

AEGetSupportedAttKeyIDNumResponse::AEGetSupportedAttKeyIDNumResponse(const AEGetSupportedAttKeyIDNumResponse& other)
    :m_response(NULL)
{
    if (other.m_response != NULL)
        m_response = new aesm::message::Response::GetSupportedAttKeyIDNumResponse(*other.m_response);
}

AEGetSupportedAttKeyIDNumResponse::~AEGetSupportedAttKeyIDNumResponse()
{
    ReleaseMemory();
}

void AEGetSupportedAttKeyIDNumResponse::ReleaseMemory()
{
   if (m_response != NULL)
    {
        delete m_response;
        m_response = NULL;
    }
}


AEMessage* AEGetSupportedAttKeyIDNumResponse::serialize()
{
    AEMessage *ae_msg = NULL;
    aesm::message::Response msg;
    if (check())
    {
        aesm::message::Response::GetSupportedAttKeyIDNumResponse* mutableResp = msg.mutable_getsupportedattkeyidnumres();
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

bool AEGetSupportedAttKeyIDNumResponse::inflateWithMessage(AEMessage* message)
{
    aesm::message::Response msg;
    msg.ParseFromArray(message->data, message->size);
    if (msg.has_getsupportedattkeyidnumres() == false)
        return false;

    ReleaseMemory();
    m_response = new aesm::message::Response::GetSupportedAttKeyIDNumResponse(msg.getsupportedattkeyidnumres());
    return true;
}

bool AEGetSupportedAttKeyIDNumResponse::GetValues(uint32_t* errorCode, uint32_t* att_key_id_num) const
{
    *att_key_id_num = m_response->att_key_id_num();
    *errorCode = m_response->errorcode();
    return true;
}

AEGetSupportedAttKeyIDNumResponse & AEGetSupportedAttKeyIDNumResponse::operator=(const AEGetSupportedAttKeyIDNumResponse &other)
{
    if (this == &other)
        return *this;

    ReleaseMemory();
    if (other.m_response != NULL)
    {
        m_response = new aesm::message::Response::GetSupportedAttKeyIDNumResponse(*other.m_response);
    }    return *this;
}

bool AEGetSupportedAttKeyIDNumResponse::check()
{
    if (m_response == NULL)
        return false;
    return m_response->IsInitialized();
}
