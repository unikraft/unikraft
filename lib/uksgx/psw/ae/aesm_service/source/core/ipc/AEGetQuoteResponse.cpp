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

#include <AEGetQuoteResponse.h>

#include <stdlib.h>
#include <string.h>

#include <limits.h>
#include <IAEMessage.h>

AEGetQuoteResponse::AEGetQuoteResponse() :
    m_response(NULL)
{
}

AEGetQuoteResponse::AEGetQuoteResponse(aesm::message::Response::GetQuoteResponse& response) :
    m_response(NULL)
{
    m_response = new aesm::message::Response::GetQuoteResponse(response);
}

AEGetQuoteResponse::AEGetQuoteResponse(uint32_t errorCode, uint32_t quoteLength, const uint8_t* quote,
                                                      uint32_t qeReportLength, const uint8_t* qeReport) :
    m_response(NULL)
{
    m_response = new aesm::message::Response::GetQuoteResponse();
    m_response->set_errorcode(errorCode);
    if (quoteLength!= 0 && quote != NULL)
        m_response->set_quote(quote, quoteLength);
    if (qeReportLength!= 0 && qeReport != NULL)
        m_response->set_qe_report(qeReport, qeReportLength);
}

//copy constructor
AEGetQuoteResponse::AEGetQuoteResponse(const AEGetQuoteResponse& other) :
    m_response(NULL)
{
    if (other.m_response != NULL)
        m_response = new aesm::message::Response::GetQuoteResponse(*other.m_response);
}

AEGetQuoteResponse::~AEGetQuoteResponse()
{
    ReleaseMemory();
}

void AEGetQuoteResponse::ReleaseMemory()
{
   if (m_response != NULL)
    {
        delete m_response;
        m_response = NULL;
    }
}

AEMessage* AEGetQuoteResponse::serialize()
{
    AEMessage *ae_msg = NULL;
    aesm::message::Response msg;
    if (check())
    {
        aesm::message::Response::GetQuoteResponse* mutableResp = msg.mutable_getquoteres();
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

bool AEGetQuoteResponse::inflateWithMessage(AEMessage* message)
{
    aesm::message::Response msg;
    msg.ParseFromArray(message->data, message->size);
    if (msg.has_getquoteres() == false)
        return false;

    ReleaseMemory();
    m_response = new aesm::message::Response::GetQuoteResponse(msg.getquoteres());
    return true;
}

bool AEGetQuoteResponse::GetValues(uint32_t* errorCode, uint32_t quoteLength, uint8_t* quote,
                                                      uint32_t qeReportLength, uint8_t* qeReport) const
{
    if (m_response->has_quote() && quote != NULL)
    {
        if (m_response->quote().size() <= quoteLength)
            memcpy(quote, m_response->quote().c_str(), m_response->quote().size());
        else
            return false;
    }

    if (m_response->has_qe_report() && qeReport != NULL)
    {
        if (m_response->qe_report().size() <= qeReportLength)
            memcpy(qeReport, m_response->qe_report().c_str(), m_response->qe_report().size());
        else
            return false;
    }
    *errorCode = m_response->errorcode();
    return true;
}

AEGetQuoteResponse& AEGetQuoteResponse::operator=(const AEGetQuoteResponse& other)
{
    if (this == &other)
        return *this;

    ReleaseMemory();
    if (other.m_response != NULL)
    {
        m_response = new aesm::message::Response::GetQuoteResponse(*other.m_response);
    }    return *this;
}

bool AEGetQuoteResponse::check()
{
    if (m_response == NULL)
        return false;
    return m_response->IsInitialized();
}
