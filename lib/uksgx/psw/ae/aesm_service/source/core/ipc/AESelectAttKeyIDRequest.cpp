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

#include <AESelectAttKeyIDRequest.h>
#include <AESelectAttKeyIDResponse.h>
#include <IAESMLogic.h>

#include <stdlib.h>
#include <limits.h>
#include <IAEMessage.h>

AESelectAttKeyIDRequest::AESelectAttKeyIDRequest(const aesm::message::Request::SelectAttKeyIDRequest &request) : m_request(NULL)
{
    m_request = new aesm::message::Request::SelectAttKeyIDRequest();
    m_request->CopyFrom(request);
}
AESelectAttKeyIDRequest::AESelectAttKeyIDRequest(uint32_t att_key_id_list_size, const uint8_t *att_key_id_list, uint32_t timeout) : m_request(NULL)
{
    m_request = new aesm::message::Request::SelectAttKeyIDRequest();
    if (att_key_id_list_size != 0 && att_key_id_list != NULL)
        m_request->set_att_key_id_list(att_key_id_list, att_key_id_list_size);
    m_request->set_timeout(timeout);
}

AESelectAttKeyIDRequest::AESelectAttKeyIDRequest(const AESelectAttKeyIDRequest &other) : IAERequest(other), m_request(NULL)
{
    if (other.m_request != NULL)
        m_request = new aesm::message::Request::SelectAttKeyIDRequest(*other.m_request);
}

AESelectAttKeyIDRequest::~AESelectAttKeyIDRequest()
{
    if (m_request != NULL)
        delete m_request;
}

AEMessage *AESelectAttKeyIDRequest::serialize()
{
    AEMessage *ae_msg = NULL;
    aesm::message::Request msg;
    if (check())
    {
        aesm::message::Request::SelectAttKeyIDRequest *mutableReq = msg.mutable_selectattkeyidreq();
        mutableReq->CopyFrom(*m_request);

        if (msg.ByteSize() <= INT_MAX)
        {
            ae_msg = new AEMessage;
            ae_msg->size = (unsigned int)msg.ByteSize();
            ae_msg->data = new char[ae_msg->size];
            msg.SerializeToArray(ae_msg->data, ae_msg->size);
        }
    }
    return ae_msg;
}

bool AESelectAttKeyIDRequest::check()
{
    if (m_request == NULL)
        return false;
    return m_request->IsInitialized();
}

IAERequest::RequestClass AESelectAttKeyIDRequest::getRequestClass()
{
    return QUOTING_CLASS;
}

AESelectAttKeyIDRequest &AESelectAttKeyIDRequest::operator=(const AESelectAttKeyIDRequest &other)
{
    if (this == &other)
        return *this;
    if (m_request != NULL)
    {
        delete m_request;
        m_request = NULL;
    }
    if (other.m_request != NULL)
        m_request = new aesm::message::Request::SelectAttKeyIDRequest(*other.m_request);
    return *this;
}

IAEResponse *AESelectAttKeyIDRequest::execute(IAESMLogic *aesmLogic)
{
    aesm_error_t result = AESM_UNEXPECTED_ERROR;
    uint8_t *select_att_key_id = NULL;
    uint32_t select_att_key_id_size = 0;

    if (check())
    {
        uint8_t *att_key_id_list = NULL;
        uint32_t att_key_id_list_size = 0;

        if (m_request->has_att_key_id_list())
        {
            att_key_id_list_size = (unsigned int)m_request->att_key_id_list().size();
            att_key_id_list = (uint8_t *)const_cast<char *>(m_request->att_key_id_list().data());
        }

        result = aesmLogic->select_att_key_id(att_key_id_list_size, att_key_id_list,
                                              &select_att_key_id_size, &select_att_key_id);
    }
    IAEResponse *response = new AESelectAttKeyIDResponse(result, select_att_key_id_size, select_att_key_id);
    if (select_att_key_id)
        delete[] select_att_key_id;
    return response;
}
