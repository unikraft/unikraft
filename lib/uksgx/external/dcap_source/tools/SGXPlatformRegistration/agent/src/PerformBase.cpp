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
/**
 * File: PerformBase.cpp
 *  
 * Description: Implmentation of the parent class that the agent uses to perform 
 * the registration flows.  It will implement transport registration 
 * data between the BIOS and the registration server using the UEFI 
 * and network libraries.
 */
#include "PerformBase.h"
#include "agent_logger.h"

bool PerformBase::perform(const uint8_t *request, const uint16_t &requestSize, uint8_t retryTimes) {
    MpResult res = MP_UNEXPECTED_ERROR;
    MpRegistrationStatus status;
    uint8_t response[MAX_RESPONSE_SIZE] = { 0 };
    uint16_t reponseSize = MAX_RESPONSE_SIZE;
    HttpStatusCode statusCode = MPA_RS_INTERNAL_SERVER_ERROR;
    RegistrationErrorCode errorCode = MPA_SUCCESS;


    uint8_t retryCnt = retryTimes;
    res = m_uefi->getRegistrationStatus(status);
    if (MP_SUCCESS != res) {
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "getRegistrationStatus failed, error: %d\n", res);
        status.errorCode = MPA_AG_BIOS_PROTOCOL_ERROR;
        goto error;
    }

    do
    {
        res = sendBinaryRequst(request, requestSize, response, reponseSize, statusCode, errorCode);
        if (MP_SUCCESS != res) {
            agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Failed to send request to server, error: %d\n", res);
            if (MP_NETWORK_ERROR == res) {
                retryCnt--;
                if(0 == retryCnt)
                {
                    /* Set the error code to the SgxRegistrationStatus.  Don't set the Registration Complete flag. */
                    status.errorCode = MPA_AG_NETWORK_ERROR;
                    break;
                } else {
                    continue;
                }
            } else {
                status.errorCode = MPA_AG_NETWORK_ERROR;
                break;
            }
        }
        agent_log_message(MP_REG_LOG_LEVEL_INFO, "Response: HTTP Response Code: %d\n", statusCode);
    
        /* Check the response code for errors */
        if(getSuccessHttpResponseCode() == statusCode)
        {
            /* RS reports an OK response.*/
            agent_log_message(MP_REG_LOG_LEVEL_INFO, "RS reports a %d OK.\n", getSuccessHttpResponseCode());
            res = useResponse(response, reponseSize);
            if (MP_SUCCESS != res) {
                agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Failed to used server response, error: %d\n", res);
                status.errorCode = MPA_AG_BIOS_PROTOCOL_ERROR;
                break;
            }

            /* Set the error code and the Registration Compete status flag */
            status.registrationStatus = MP_TASK_COMPLETED;
            status.errorCode = MPA_SUCCESS;

            retryCnt = 0;
        }
        else if(MPA_RS_BAD_REQUEST == statusCode)
        {
            /* RS reports a bad request.  */
            agent_log_message(MP_REG_LOG_LEVEL_ERROR, "RS reports a '400 Bad Request'.\n");
            
            /* Set the error code to the SgxRegistrationStatus*/
            status.registrationStatus = MP_TASK_COMPLETED;
            status.errorCode = errorCode;
            retryCnt = 0;
        }
        else if(MPA_RS_INTERNAL_SERVER_ERROR == statusCode)//do we need to complete here?
        {
            /* RS reports Internal Server Error.  */
            agent_log_message(MP_REG_LOG_LEVEL_ERROR, "RS reports a '500 Internal Server Error'.\n");
    
            /* Set the error code to the SgxRegistrationStatus*/
            status.registrationStatus = MP_TASK_COMPLETED;
            status.errorCode = MPA_AG_INTERNAL_SERVER_ERROR;
            retryCnt = 0;
        }
        else if(MPA_RS_FAIL_UNAUTHORIZED == statusCode)
        {
            /* RS reports the request is unauthorized.  */
            agent_log_message(MP_REG_LOG_LEVEL_ERROR, "RS reports a '401 Failed to authenticate or authorize the request'.\n");
    
            /* Set the error code to the SgxRegistrationStatus*/
            status.errorCode = MPA_AG_UNAUTHORIZED_ERROR;
            retryCnt = 0;
        }
        else if(MPA_RS_SERVICE_UNAVAILABLE == statusCode)
        {
            /* RS reports:Server is currently unable to process the request. */
            agent_log_message(MP_REG_LOG_LEVEL_ERROR, "RS reports a '503 Service Unavailable'.\n");
            /* Retry until retry count runs out */
            retryCnt--;
            if(0 == retryCnt)
            {
                /* Set the error code to the SgxRegistrationStatus.  Don't set the Registration Complete flag. */
                status.errorCode = MPA_AG_SERVER_TIMEOUT;
            }
        }
        else
        {
            /* RS reports an unknown error. */
            agent_log_message(MP_REG_LOG_LEVEL_ERROR, "RS reports an unknown error'.\n");

            /* Set the error code to the SgxRegistrationStatus. */
            status.registrationStatus = MP_TASK_COMPLETED;
            status.errorCode = MPA_RS_UNKOWN_ERROR;
            retryCnt = 0;
        }
    } while(retryCnt > 0);

error:
    res = m_uefi->setRegistrationStatus(status);
    if (MP_SUCCESS != res) {
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "setRegistrationStatus failed, error: %d\n", res);
        return false;
    }
    
    if ((MPA_SUCCESS == status.errorCode) && (MP_TASK_COMPLETED == status.registrationStatus)) {
        return true;
    } else {
        return false;
    }
}
