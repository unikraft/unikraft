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

#include <AEGetQuoteSizeExRequest.h>
#include <AEGetQuoteSizeExResponse.h>
#include <IAESMLogic.h>
#include <stdlib.h>
#include <limits.h>
#include <IAEMessage.h>

AEGetQuoteSizeExRequest::AEGetQuoteSizeExRequest(const aesm::message::Request::GetQuoteSizeExRequest& request) :
    m_request(NULL)
{
    m_request = new aesm::message::Request::GetQuoteSizeExRequest();
    m_request->CopyFrom(request);
}

AEGetQuoteSizeExRequest::AEGetQuoteSizeExRequest(uint32_t att_key_id_size, uint8_t* att_key_id, uint32_t timeout)
    :m_request(NULL)
{
    m_request = new aesm::message::Request::GetQuoteSizeExRequest();
    if (att_key_id_size != 0 && att_key_id != NULL)
        m_request->set_att_key_id(att_key_id, att_key_id_size);
    m_request->set_timeout(timeout);
}

AEGetQuoteSizeExRequest::AEGetQuoteSizeExRequest(const AEGetQuoteSizeExRequest& other)
    :m_request(NULL)
{
    if (other.m_request != NULL)
        m_request = new aesm::message::Request::GetQuoteSizeExRequest(*other.m_request);
}

AEGetQuoteSizeExRequest::~AEGetQuoteSizeExRequest()
{
    if (m_request != NULL)
        delete m_request;
}

AEMessage* AEGetQuoteSizeExRequest::serialize(){
    AEMessage *ae_msg = NULL;
    aesm::message::Request msg;
    if (check())
    {
        aesm::message::Request::GetQuoteSizeExRequest* mutableReq = msg.mutable_getquotesizeexreq();
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

IAERequest::RequestClass AEGetQuoteSizeExRequest::getRequestClass() {
    return QUOTING_CLASS;
}

AEGetQuoteSizeExRequest& AEGetQuoteSizeExRequest::operator=(const AEGetQuoteSizeExRequest& other)
{
    if (this == &other)
        return *this;
    if (m_request != NULL)
    {
        delete m_request;
        m_request = NULL;
    }
    if (other.m_request != NULL)
        m_request = new aesm::message::Request::GetQuoteSizeExRequest(*other.m_request);
    return *this;
}

bool AEGetQuoteSizeExRequest::check()
{
    if (m_request == NULL)
        return false;
    return m_request->IsInitialized();
}


IAEResponse* AEGetQuoteSizeExRequest::execute(IAESMLogic* aesmLogic)
{
    aesm_error_t result = AESM_UNEXPECTED_ERROR;
    uint32_t quote_size = 0;

    if (check())
    {
        uint32_t att_key_id_size = 0;
        uint8_t* att_key_id = NULL;
        if (m_request->has_att_key_id())
        {
            att_key_id_size = (uint32_t)m_request->att_key_id().size();
            att_key_id= (uint8_t*)const_cast<char *>(m_request->att_key_id().data());
        }
        result = aesmLogic->get_quote_size_ex(att_key_id_size, att_key_id, &quote_size);
    }

    AEGetQuoteSizeExResponse * response = new AEGetQuoteSizeExResponse((uint32_t)result, quote_size);

    return response;
}
