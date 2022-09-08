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

#include <AEInitQuoteExRequest.h>
#include <AEInitQuoteExResponse.h>
#include <IAESMLogic.h>

#include <stdlib.h>
#include <limits.h>
#include <IAEMessage.h>


AEInitQuoteExRequest::AEInitQuoteExRequest(const aesm::message::Request::InitQuoteExRequest& request) :
    m_request(NULL)
{
    m_request = new aesm::message::Request::InitQuoteExRequest();
    m_request->CopyFrom(request);
}
AEInitQuoteExRequest::AEInitQuoteExRequest(uint32_t att_key_id_size, const uint8_t *att_key_id,
        bool b_pub_key_id,
        size_t buf_size, uint32_t timeout):
    m_request(NULL)
{
    m_request = new aesm::message::Request::InitQuoteExRequest();
    if (att_key_id_size != 0 && att_key_id != NULL)
        m_request->set_att_key_id(att_key_id, att_key_id_size);
    m_request->set_b_pub_key_id(b_pub_key_id);
    if (buf_size != 0)
        m_request->set_buf_size(buf_size);
    m_request->set_timeout(timeout);
}

AEInitQuoteExRequest::AEInitQuoteExRequest(const AEInitQuoteExRequest& other) :
    IAERequest(other), m_request(NULL)
{
    if (other.m_request != NULL)
        m_request = new aesm::message::Request::InitQuoteExRequest(*other.m_request);
}

AEInitQuoteExRequest::~AEInitQuoteExRequest()
{
    if (m_request != NULL)
        delete m_request;
}


AEMessage* AEInitQuoteExRequest::serialize()
{
    AEMessage *ae_msg = NULL;
    aesm::message::Request msg;
    if (check())
    {
        aesm::message::Request::InitQuoteExRequest* mutableReq = msg.mutable_initquoteexreq();
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

bool AEInitQuoteExRequest::check()
{
    if (m_request == NULL)
        return false;
    return m_request->IsInitialized();
}

IAERequest::RequestClass AEInitQuoteExRequest::getRequestClass() {
    return QUOTING_CLASS;
}

AEInitQuoteExRequest& AEInitQuoteExRequest::operator=(const AEInitQuoteExRequest& other)
{
    if (this == &other)
        return *this;
    if (m_request != NULL)
    {
        delete m_request;
        m_request = NULL;
    }
    if (other.m_request != NULL)
        m_request = new aesm::message::Request::InitQuoteExRequest(*other.m_request);
    return *this;
}


IAEResponse* AEInitQuoteExRequest::execute(IAESMLogic* aesmLogic)
{
    aesm_error_t result = AESM_UNEXPECTED_ERROR;
    uint8_t* target_info = NULL;
    uint32_t target_info_size = 0;
    uint8_t *pub_key_id = NULL;
    size_t pub_key_id_size = 0;
    size_t buf_size = 0;
    if (check())
    {
        uint32_t att_key_id_size = 0;
        uint8_t *att_key_id = NULL;
        if (m_request->has_att_key_id())
        {
            att_key_id_size = (unsigned int)m_request->att_key_id().size();
            att_key_id = (uint8_t*)const_cast<char *>(m_request->att_key_id().data());
        }

        bool b_pub_key_id = m_request->b_pub_key_id();

        if (m_request->has_buf_size())
	{
            pub_key_id_size = (size_t)m_request->buf_size();
            buf_size = pub_key_id_size;
	}

        result= aesmLogic->init_quote_ex(
                att_key_id_size, att_key_id,
                &target_info, &target_info_size,
                b_pub_key_id, &pub_key_id_size, &pub_key_id);
    }
    IAEResponse* response = new AEInitQuoteExResponse(result, target_info_size, target_info, &pub_key_id_size, buf_size, pub_key_id);
    if (target_info)
        delete[] target_info;
    if (pub_key_id)
        delete pub_key_id;
    return response;
}
