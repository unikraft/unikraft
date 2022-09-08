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

#include <AECheckUpdateStatusRequest.h>
#include <AECheckUpdateStatusResponse.h>
#include <IAESMLogic.h>

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <IAEMessage.h>

AECheckUpdateStatusRequest::AECheckUpdateStatusRequest(const aesm::message::Request::CheckUpdateStatusRequest& request) :
    m_request(NULL)
{
    m_request = new aesm::message::Request::CheckUpdateStatusRequest();
    m_request->CopyFrom(request);
}

AECheckUpdateStatusRequest::AECheckUpdateStatusRequest(uint32_t platformInfoLength, const uint8_t* platformInfo, uint32_t updateInfoLength, uint32_t config, uint32_t timeout)
    :m_request(NULL)
{
    m_request = new aesm::message::Request::CheckUpdateStatusRequest();

    if (platformInfoLength !=0 && platformInfo != NULL)
    {
        m_request->set_platform_info(platformInfo, platformInfoLength);
    }
    m_request->set_config(config);
    m_request->set_update_info_size(updateInfoLength);
    m_request->set_timeout(timeout);
}

AECheckUpdateStatusRequest::AECheckUpdateStatusRequest(const AECheckUpdateStatusRequest& other)
    : m_request(NULL)
{
    if (other.m_request != NULL)
        m_request = new aesm::message::Request::CheckUpdateStatusRequest(*other.m_request);
}

AECheckUpdateStatusRequest::~AECheckUpdateStatusRequest()
{
    if (m_request != NULL)
        delete m_request;
}

AEMessage* AECheckUpdateStatusRequest::serialize()
{
    AEMessage *ae_msg = NULL;
    aesm::message::Request msg;
    if (check())
    {
        aesm::message::Request::CheckUpdateStatusRequest* mutableReq = msg.mutable_checkupdatestatusreq();
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

AECheckUpdateStatusRequest& AECheckUpdateStatusRequest::operator=(const AECheckUpdateStatusRequest& other)
{
    if (this == &other)
        return *this;
    if (m_request != NULL)
    {
        delete m_request;
        m_request = NULL;
    }
    if (other.m_request != NULL)
        m_request = new aesm::message::Request::CheckUpdateStatusRequest(*other.m_request);
    return *this;
}

bool AECheckUpdateStatusRequest::check()
{
    if (m_request == NULL)
        return false;
    return m_request->IsInitialized();
}

IAERequest::RequestClass AECheckUpdateStatusRequest::getRequestClass()
{
    return QUOTING_CLASS;
}

IAEResponse* AECheckUpdateStatusRequest::execute(IAESMLogic* aesmLogic)
{
    aesm_error_t result = AESM_UNEXPECTED_ERROR;
    uint8_t* update_info = NULL;
    uint32_t update_info_size = 0;
    uint32_t status = 0;
    if (check())
    {

        uint32_t platform_info_length = 0;
        uint8_t* platform_info = NULL;
        if (m_request->has_platform_info())
        {
            platform_info_length = (unsigned int)m_request->platform_info().size();
            platform_info = (uint8_t*)const_cast<char *>(m_request->platform_info().data());
        }
        
        update_info_size = m_request->update_info_size();
        uint32_t config = m_request->config();
        result = aesmLogic->checkUpdateStatus(platform_info, platform_info_length,
            &update_info, update_info_size,
            config, &status);
    }
    IAEResponse* response = new AECheckUpdateStatusResponse(result, update_info_size, update_info, &status);
    if (update_info)
        delete[]update_info;
    return response;
}
