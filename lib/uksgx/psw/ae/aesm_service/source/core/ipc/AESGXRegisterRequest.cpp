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

#include <AESGXRegisterRequest.h>
#include <AESGXRegisterResponse.h>
#include <IAESMLogic.h>

#include <stdlib.h>
#include <limits.h>
#include <IAEMessage.h>

AESGXRegisterRequest::AESGXRegisterRequest(const aesm::message::Request::SGXRegisterRequest& request) :
    m_request(NULL)
{
    m_request = new aesm::message::Request::SGXRegisterRequest();
    m_request->CopyFrom(request);
}

AESGXRegisterRequest::AESGXRegisterRequest(uint32_t BufLength, const uint8_t* Buf, uint32_t DataType, uint32_t timeout)
    :m_request(NULL)
{
    m_request = new aesm::message::Request::SGXRegisterRequest();

    if (BufLength !=0 && Buf != NULL)
        m_request->set_buf(Buf, BufLength);
    m_request->set_data_type(DataType);
    m_request->set_timeout(timeout);
}

AESGXRegisterRequest::AESGXRegisterRequest(const AESGXRegisterRequest& other)
    : m_request(NULL)
{
    if (other.m_request != NULL)
        m_request = new aesm::message::Request::SGXRegisterRequest(*other.m_request);
}

AESGXRegisterRequest::~AESGXRegisterRequest()
{
    if (m_request != NULL)
        delete m_request;
}

AEMessage* AESGXRegisterRequest::serialize()
{
    AEMessage *ae_msg = NULL;
    aesm::message::Request msg;
    if (check())
    {
        aesm::message::Request::SGXRegisterRequest* mutableReq = msg.mutable_sgxregisterreq();
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


AESGXRegisterRequest& AESGXRegisterRequest::operator=(const AESGXRegisterRequest& other)
{
    if (this == &other)
        return *this;
    if (m_request != NULL)
    {
        delete m_request;
        m_request = NULL;
    }
    if (other.m_request != NULL)
        m_request = new aesm::message::Request::SGXRegisterRequest(*other.m_request);
    return *this;
}

bool AESGXRegisterRequest::check()
{
    if (m_request == NULL)
        return false;
    return m_request->IsInitialized();
}

IAERequest::RequestClass AESGXRegisterRequest::getRequestClass() {
    return LAUNCH_CLASS;
}

IAEResponse* AESGXRegisterRequest::execute(IAESMLogic* aesmLogic) {
    aesm_error_t ret = AESM_UNEXPECTED_ERROR;

    if (check())
    {

        uint32_t buf_length = 0;
        uint8_t* buf = NULL;
        uint32_t data_type = 0;

        if (m_request->has_buf())
        {
            buf_length = (unsigned int)m_request->buf().size();
            buf = (uint8_t*)const_cast<char *>(m_request->buf().data());
        }

        data_type = m_request->data_type();
        ret = aesmLogic->sgxRegister(buf, buf_length, data_type);
    }

    IAEResponse* ae_res = new AESGXRegisterResponse(ret);

    return ae_res;
}
