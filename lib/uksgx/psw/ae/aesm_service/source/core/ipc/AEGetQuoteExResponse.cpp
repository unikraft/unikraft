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

#include <AEGetQuoteExResponse.h>

#include <stdlib.h>
#include <string.h>

#include <limits.h>
#include <IAEMessage.h>

AEGetQuoteExResponse::AEGetQuoteExResponse() :
    m_response(NULL)
{
}

AEGetQuoteExResponse::AEGetQuoteExResponse(aesm::message::Response::GetQuoteExResponse& response) :
    m_response(NULL)
{
    m_response = new aesm::message::Response::GetQuoteExResponse(response);
}

AEGetQuoteExResponse::AEGetQuoteExResponse(uint32_t errorCode, uint32_t quoteLength, const uint8_t* quote,
                                        uint32_t qe_report_info_size, const uint8_t *qe_report_info) :
    m_response(NULL)
{
    m_response = new aesm::message::Response::GetQuoteExResponse();
    m_response->set_errorcode(errorCode);
    if (quoteLength!= 0 && quote != NULL)
        m_response->set_quote(quote, quoteLength);
    if (qe_report_info_size != 0 && qe_report_info != NULL)
        m_response->set_qe_report_info(qe_report_info, qe_report_info_size);
}

//copy constructor
AEGetQuoteExResponse::AEGetQuoteExResponse(const AEGetQuoteExResponse& other) :
    m_response(NULL)
{
    if (other.m_response != NULL)
        m_response = new aesm::message::Response::GetQuoteExResponse(*other.m_response);
}

AEGetQuoteExResponse::~AEGetQuoteExResponse()
{
    ReleaseMemory();
}

void AEGetQuoteExResponse::ReleaseMemory()
{
   if (m_response != NULL)
    {
        delete m_response;
        m_response = NULL;
    }
}

AEMessage* AEGetQuoteExResponse::serialize()
{
    AEMessage *ae_msg = NULL;
    aesm::message::Response msg;
    if (check())
    {
        aesm::message::Response::GetQuoteExResponse* mutableResp = msg.mutable_getquoteexres();
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

bool AEGetQuoteExResponse::inflateWithMessage(AEMessage* message)
{
    aesm::message::Response msg;
    msg.ParseFromArray(message->data, message->size);
    if (msg.has_getquoteexres() == false)
        return false;

    ReleaseMemory();
    m_response = new aesm::message::Response::GetQuoteExResponse(msg.getquoteexres());
    return true;
}

bool AEGetQuoteExResponse::GetValues(uint32_t* errorCode, uint32_t quoteLength, uint8_t* quote,
                                        uint32_t qe_report_info_size, uint8_t *qe_report_info) const
{
    if (m_response->has_quote() && quote != NULL)
    {
        if (m_response->quote().size() <= quoteLength)
            memcpy(quote, m_response->quote().c_str(), m_response->quote().size());
        else
            return false;
    }
    if (m_response->has_qe_report_info() && qe_report_info != NULL)
    {
        if (m_response->qe_report_info().size() <= qe_report_info_size)
            memcpy(qe_report_info, m_response->qe_report_info().c_str(), m_response->qe_report_info().size());
        else
            return false;
    }

    *errorCode = m_response->errorcode();
    return true;
}

AEGetQuoteExResponse& AEGetQuoteExResponse::operator=(const AEGetQuoteExResponse& other)
{
    if (this == &other)
        return *this;

    ReleaseMemory();
    if (other.m_response != NULL)
    {
        m_response = new aesm::message::Response::GetQuoteExResponse(*other.m_response);
    }    return *this;
}

bool AEGetQuoteExResponse::check()
{
    if (m_response == NULL)
        return false;
    return m_response->IsInitialized();
}
