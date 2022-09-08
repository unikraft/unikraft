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

#include <AEGetSupportedAttKeyIDsRequest.h>
#include <AEGetSupportedAttKeyIDsResponse.h>
#include <IAESMLogic.h>

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <IAEMessage.h>

AEGetSupportedAttKeyIDsRequest::AEGetSupportedAttKeyIDsRequest(const aesm::message::Request::GetSupportedAttKeyIDsRequest& request) :
    m_request(NULL)
{
    m_request = new aesm::message::Request::GetSupportedAttKeyIDsRequest();
    m_request->CopyFrom(request);
}

AEGetSupportedAttKeyIDsRequest::AEGetSupportedAttKeyIDsRequest(uint32_t buf_size, uint32_t timeout)
    :m_request(NULL)

{
    m_request = new aesm::message::Request::GetSupportedAttKeyIDsRequest();
    m_request->set_buf_size(buf_size);
    m_request->set_timeout(timeout);
}

AEGetSupportedAttKeyIDsRequest::AEGetSupportedAttKeyIDsRequest(const AEGetSupportedAttKeyIDsRequest& other)
    :m_request(NULL)
{
    if (other.m_request != NULL)
        m_request = new aesm::message::Request::GetSupportedAttKeyIDsRequest(*other.m_request);
}

AEGetSupportedAttKeyIDsRequest::~AEGetSupportedAttKeyIDsRequest()
{
    if (m_request != NULL)
        delete m_request;
}

AEMessage* AEGetSupportedAttKeyIDsRequest::serialize()
{
    AEMessage *ae_msg = NULL;
    aesm::message::Request msg;
    if (check())
    {
        aesm::message::Request::GetSupportedAttKeyIDsRequest* mutableReq = msg.mutable_getsupportedattkeyidsreq();
        mutableReq->CopyFrom(*m_request);

        if (msg.ByteSize() <= INT_MAX) {
            ae_msg = new AEMessage;
            ae_msg->size = (unsigned int)msg.ByteSize();
            ae_msg->data = new char[ae_msg->size];
            msg.SerializeToArray(ae_msg->data, ae_msg->size);
        }
    }
    return ae_msg;
}

AEGetSupportedAttKeyIDsRequest& AEGetSupportedAttKeyIDsRequest::operator=(const AEGetSupportedAttKeyIDsRequest& other)
{
    if (this == &other)
        return *this;
    if (m_request != NULL)
    {
        delete m_request;
        m_request = NULL;
    }
    if (other.m_request != NULL)
        m_request = new aesm::message::Request::GetSupportedAttKeyIDsRequest(*other.m_request);
    return *this;
}

bool AEGetSupportedAttKeyIDsRequest::check()
{
    if (m_request == NULL)
        return false;
    return m_request->IsInitialized();
}

IAERequest::RequestClass AEGetSupportedAttKeyIDsRequest::getRequestClass()
{
    return LAUNCH_CLASS;
}

IAEResponse* AEGetSupportedAttKeyIDsRequest::execute(IAESMLogic* aesmLogic)
{
    aesm_error_t result = AESM_UNEXPECTED_ERROR;
    uint8_t* att_key_ids = 0;
    uint32_t buf_size = 0;

    if (check())
    {
        buf_size =  m_request->buf_size();
        result = aesmLogic->get_supported_att_key_ids(&att_key_ids,buf_size);
    }

    AEGetSupportedAttKeyIDsResponse* response = new AEGetSupportedAttKeyIDsResponse(result, buf_size, att_key_ids);

    if (att_key_ids)
        delete [] att_key_ids;

    return response;
}
