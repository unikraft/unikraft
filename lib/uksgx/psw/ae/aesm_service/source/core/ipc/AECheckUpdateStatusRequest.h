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
#ifndef _AE_CHECK_UPDATE_STATUS_REQUEST_H
#define _AE_CHECK_UPDATE_STATUS_REQUEST_H

#include <IAERequest.h>

namespace aesm
{
    namespace message
    {
        class Request_CheckUpdateStatusRequest;
    };
};


class AECheckUpdateStatusRequest : public IAERequest
{
    public:
        AECheckUpdateStatusRequest(const aesm::message::Request_CheckUpdateStatusRequest& request);
        AECheckUpdateStatusRequest(uint32_t platformInfoLength, const uint8_t* platformInfo, uint32_t updateInfoLength, uint32_t config, uint32_t timeout = 0);
        AECheckUpdateStatusRequest(const AECheckUpdateStatusRequest& other);
        ~AECheckUpdateStatusRequest();

        AEMessage* serialize();

        //operators
        AECheckUpdateStatusRequest& operator=(const AECheckUpdateStatusRequest& request);

        //checks
        bool check();

        //hooks
        virtual IAEResponse* execute(IAESMLogic* aesmLogic);

        //used to determine in which queue to be placed
        virtual RequestClass getRequestClass();
    protected:
        void ReleaseMemory();
        aesm::message::Request_CheckUpdateStatusRequest* m_request;
};

#endif
