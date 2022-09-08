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

#include <AEInitQuoteExResponse.h>

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <IAEMessage.h>

AEInitQuoteExResponse::AEInitQuoteExResponse() :
    m_response(NULL)
{
}

AEInitQuoteExResponse::AEInitQuoteExResponse(aesm::message::Response::InitQuoteExResponse& response) :
    m_response(NULL)
{
    m_response = new aesm::message::Response::InitQuoteExResponse(response);
}

AEInitQuoteExResponse::AEInitQuoteExResponse(uint32_t errorCode, uint32_t target_info_size, uint8_t *target_info, 
            uint64_t *pub_key_id_size, uint64_t buf_size, const uint8_t *pub_key_id) :
    m_response(NULL)
{

    m_response = new aesm::message::Response::InitQuoteExResponse();
    m_response->set_errorcode(errorCode);
    if (target_info_size!= 0 && target_info != NULL)
        m_response->set_target_info(target_info, target_info_size);
    if (pub_key_id_size!= NULL )
        m_response->set_pub_key_id_size(*pub_key_id_size);
    if (buf_size!= 0 && pub_key_id != NULL)
        m_response->set_pub_key_id(pub_key_id, buf_size);
}

AEInitQuoteExResponse::AEInitQuoteExResponse(const AEInitQuoteExResponse& other) :
    m_response(NULL)
{
    if (other.m_response != NULL)
        m_response = new aesm::message::Response::InitQuoteExResponse(*other.m_response);
}
AEInitQuoteExResponse::~AEInitQuoteExResponse()
{
    ReleaseMemory();
}

void AEInitQuoteExResponse::ReleaseMemory()
{
    if (m_response != NULL)
    {
        delete m_response;
        m_response = NULL;
    }
}


AEMessage* AEInitQuoteExResponse::serialize()
{
    AEMessage *ae_msg = NULL;

    aesm::message::Response msg;
    if (check())
    {
        aesm::message::Response::InitQuoteExResponse* mutableRes = msg.mutable_initquoteexres();
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

bool AEInitQuoteExResponse::inflateWithMessage(AEMessage* message)
{
    aesm::message::Response msg;
    msg.ParseFromArray(message->data, message->size);
    if (msg.has_initquoteexres() == false)
        return false;

    //this is an AEGetLaunchTokenResponse
    ReleaseMemory();
    m_response = new aesm::message::Response::InitQuoteExResponse(msg.initquoteexres());
    return true;
}

bool AEInitQuoteExResponse::GetValues(uint32_t* errorCode, uint32_t target_info_size, uint8_t *target_info, 
            uint64_t* pub_key_id_size, uint64_t buf_size, uint8_t *pub_key_id) const
{
    if (m_response->has_target_info() && target_info != NULL)
    {
        if (m_response->target_info().size() <= target_info_size)
            memcpy(target_info, m_response->target_info().c_str(), m_response->target_info().size());
        else
            return false;
    }
    if (m_response->has_pub_key_id_size())
        *pub_key_id_size = m_response->pub_key_id_size(); 
    if (m_response->has_pub_key_id() && pub_key_id_size != NULL &&  m_response->pub_key_id().size() == *pub_key_id_size )
    {
        if (m_response->pub_key_id().size() <= buf_size)
            memcpy(pub_key_id, m_response->pub_key_id().c_str(), m_response->pub_key_id().size());
        else
            return false;
    }
    *errorCode = m_response->errorcode(); 
    return true;
}

AEInitQuoteExResponse & AEInitQuoteExResponse::operator=(const AEInitQuoteExResponse &other)
{
    if (this == &other)
        return *this;

    ReleaseMemory();
    if (other.m_response != NULL)
    {
        m_response = new aesm::message::Response::InitQuoteExResponse(*other.m_response);
    }
    return *this;
}

bool AEInitQuoteExResponse::check()
{
    if (m_response == NULL)
        return false;
    return m_response->IsInitialized();
}
