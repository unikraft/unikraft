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

#include <AEGetSupportedAttKeyIDsResponse.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>
#include <IAEMessage.h>

AEGetSupportedAttKeyIDsResponse::AEGetSupportedAttKeyIDsResponse()
    :m_response(NULL)
{
}

AEGetSupportedAttKeyIDsResponse::AEGetSupportedAttKeyIDsResponse(aesm::message::Response::GetSupportedAttKeyIDsResponse& response)
    :m_response(NULL)
{
    m_response = new aesm::message::Response::GetSupportedAttKeyIDsResponse(response);
}

AEGetSupportedAttKeyIDsResponse::AEGetSupportedAttKeyIDsResponse(uint32_t errorCode, uint32_t att_key_ids_size, const uint8_t* att_key_ids)
    :m_response(NULL)
{
    m_response = new aesm::message::Response::GetSupportedAttKeyIDsResponse();
    m_response->set_errorcode(errorCode);
    if (att_key_ids_size!= 0 && att_key_ids != NULL)
        m_response->set_att_key_ids(att_key_ids, att_key_ids_size);
}

AEGetSupportedAttKeyIDsResponse::AEGetSupportedAttKeyIDsResponse(const AEGetSupportedAttKeyIDsResponse& other)
    :m_response(NULL)
{
    if (other.m_response != NULL)
        m_response = new aesm::message::Response::GetSupportedAttKeyIDsResponse(*other.m_response);
}

AEGetSupportedAttKeyIDsResponse::~AEGetSupportedAttKeyIDsResponse()
{
    ReleaseMemory();
}

void AEGetSupportedAttKeyIDsResponse::ReleaseMemory()
{
   if (m_response != NULL)
    {
        delete m_response;
        m_response = NULL;
    }
}

AEMessage* AEGetSupportedAttKeyIDsResponse::serialize()
{
    AEMessage *ae_msg = NULL;
    aesm::message::Response msg;
    if (check())
    {
        aesm::message::Response::GetSupportedAttKeyIDsResponse* mutableResp = msg.mutable_getsupportedattkeyidsres();
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

bool AEGetSupportedAttKeyIDsResponse::inflateWithMessage(AEMessage* message)
{
    aesm::message::Response msg;
    msg.ParseFromArray(message->data, message->size);
    if (msg.has_getsupportedattkeyidsres() == false)
        return false;

    ReleaseMemory();
    m_response = new aesm::message::Response::GetSupportedAttKeyIDsResponse(msg.getsupportedattkeyidsres());
    return true;
}

bool AEGetSupportedAttKeyIDsResponse::GetValues(uint32_t* errorCode, uint32_t att_key_ids_size,uint8_t* att_key_ids) const
{
    if (m_response->has_att_key_ids() && att_key_ids != NULL)
    {
        if (m_response->att_key_ids().size() <= att_key_ids_size)
            memcpy(att_key_ids, m_response->att_key_ids().c_str(), m_response->att_key_ids().size());
        else
            return false;
    }

    *errorCode = m_response->errorcode();
    return true;
}


AEGetSupportedAttKeyIDsResponse& AEGetSupportedAttKeyIDsResponse::operator=(const AEGetSupportedAttKeyIDsResponse& other)
{
    if (this == &other)
        return * this;

    ReleaseMemory();
    if (other.m_response != NULL)
    {
        m_response = new aesm::message::Response::GetSupportedAttKeyIDsResponse(*other.m_response);
    }    return *this;
}

bool AEGetSupportedAttKeyIDsResponse::check()
{
    if (m_response == NULL)
        return false;
    return m_response->IsInitialized();
}
