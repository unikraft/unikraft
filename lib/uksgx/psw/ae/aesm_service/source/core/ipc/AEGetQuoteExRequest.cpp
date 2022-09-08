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

#include <AEGetQuoteExRequest.h>
#include <AEGetQuoteExResponse.h>
#include "IAESMLogic.h"

#include <stdlib.h>
#include <limits.h>
#include <IAEMessage.h>


    AEGetQuoteExRequest::AEGetQuoteExRequest(const aesm::message::Request::GetQuoteExRequest& request) :
    m_request(NULL)
{
    m_request = new aesm::message::Request::GetQuoteExRequest();
    m_request->CopyFrom(request);
}

AEGetQuoteExRequest::AEGetQuoteExRequest(uint32_t report_size, const uint8_t *report,
    uint32_t att_key_id_size, const uint8_t *att_key_id,
    uint32_t qe_report_info_size, uint8_t *qe_report_info,
    uint32_t buf_size,
    uint32_t timeout)
        : m_request(NULL)
{
    m_request = new aesm::message::Request::GetQuoteExRequest();
    if (report_size !=0 && report != NULL)
        m_request->set_report(report, report_size);
    if (att_key_id_size != 0 && att_key_id != NULL)
        m_request->set_att_key_id(att_key_id, att_key_id_size);
    if (qe_report_info_size != 0 && qe_report_info != NULL)
        m_request->set_qe_report_info(qe_report_info, qe_report_info_size);
    m_request->set_buf_size(buf_size);
    m_request->set_timeout(timeout);
}

AEGetQuoteExRequest::AEGetQuoteExRequest(const AEGetQuoteExRequest& other)
    : m_request(NULL)
{
    if (other.m_request != NULL)
        m_request = new aesm::message::Request::GetQuoteExRequest(*other.m_request);
}

AEGetQuoteExRequest::~AEGetQuoteExRequest()
{
    if (m_request != NULL)
        delete m_request;}

AEMessage* AEGetQuoteExRequest::serialize()
{
    AEMessage *ae_msg = NULL;
    aesm::message::Request msg;
    if (check())
    {
        aesm::message::Request::GetQuoteExRequest* mutableReq = msg.mutable_getquoteexreq();
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

AEGetQuoteExRequest& AEGetQuoteExRequest::operator=(const AEGetQuoteExRequest& other)
{
    if (this == &other)
        return *this;
    if (m_request != NULL)
    {
        delete m_request;
        m_request = NULL;
    }
    if (other.m_request != NULL)
        m_request = new aesm::message::Request::GetQuoteExRequest(*other.m_request);
    return *this;
}

bool AEGetQuoteExRequest::check()
{
    if (m_request == NULL)
        return false;
    return m_request->IsInitialized();
}

IAERequest::RequestClass AEGetQuoteExRequest::getRequestClass() {
    return QUOTING_CLASS;
}

IAEResponse* AEGetQuoteExRequest::execute(IAESMLogic* aesmLogic) {
    aesm_error_t result = AESM_UNEXPECTED_ERROR;

    uint32_t quote_length = 0;
    uint8_t* quote = NULL;
    uint32_t qe_report_info_size = 0;
    uint8_t *qe_report_info = NULL;

    if (check())
    {
        uint32_t report_length = 0;
        uint8_t* report = NULL;
        uint32_t att_key_id_size = 0;
        uint8_t *att_key_id = NULL;

        if (m_request->has_report())
        {
            report_length = (uint32_t)m_request->report().size();
            report = (uint8_t*)const_cast<char *>(m_request->report().data());
        }
        if (m_request->has_att_key_id())
        {
            att_key_id_size = (uint32_t)m_request->att_key_id().size();
            att_key_id = (uint8_t*)const_cast<char *>(m_request->att_key_id().data());
        }
        if (m_request->has_qe_report_info())
        {
            qe_report_info_size = (unsigned int)m_request->qe_report_info().size();
            qe_report_info = (uint8_t*)const_cast<char *>(m_request->qe_report_info().data());
        }
        quote_length = (uint32_t)m_request->buf_size();

        result = aesmLogic->get_quote_ex(report_length, report,
                att_key_id_size, att_key_id,
                qe_report_info_size, qe_report_info,
                quote_length, &quote);

    }
    AEGetQuoteExResponse* response = new AEGetQuoteExResponse(result, quote_length, quote, qe_report_info_size, qe_report_info);

    //free the buffer before send
    if (quote)
        delete[] quote;
    return response;
}
