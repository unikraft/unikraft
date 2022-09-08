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
 * File: RegistrationService.cpp
 *  
 * Description: Implmenation for the high level class for the registration 
 * agent service/daemon.
 */
#include <memory.h>
#include "RegistrationService.h"
#include "UefiVar.h"
#include "PerformBase.h"
#include "PerformPlatformRegistration.h"
#include "PerformPackageAdd.h"
#include "agent_logger.h"
#include "common.h"

#define MP_BIOS_SRC_ERROR 0x80

extern LogLevel glog_level;

RegistrationService::RegistrationService(const MPConfigurations &conf) {
    m_conf = conf;
    m_gLogLevel = conf.log_level;
    glog_level = m_gLogLevel;
    m_uefi = NULL;
    m_network = NULL;
}


bool RegistrationService::isMultiPackageCapable() {
    MpResult res = MP_UNEXPECTED_ERROR;
    bool capable = false;
    string serverUrl = "";
    uint16_t flags = 0;
    uint16_t serverIdSize = 0;

    if (!m_uefi) {
        m_uefi = new MPUefi(string(m_conf.uefi_path), m_gLogLevel);
        if (!m_uefi) {
            agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Allocation error during uefi initiation.\n");
            goto error;
        }
    }

    res = m_uefi->getRegistrationServerInfo(flags, serverUrl, NULL, serverIdSize);
    if (MP_SUCCESS != res) {
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "getRegistrationServerInfo failed, error: %d\n", res);
        goto error;
    }
    capable = true;

error:
    return capable;
}

void RegistrationService::registerPlatformIfNeeded() {
    MpResult res = MP_UNEXPECTED_ERROR;
    RegistrationErrorCode errorCode = MPA_AG_UNEXPECTED_ERROR;
    bool registered = false;
    PerformBase* pb = NULL;
    uint16_t requestSize = 0;

    if (!m_uefi) {
        m_uefi = new MPUefi(string(m_conf.uefi_path), m_gLogLevel);
        if (!m_uefi) {
            agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Allocation error during uefi initiation.\n");
            return;
        }
    }

    MpRegistrationStatus status;
    res = m_uefi->getRegistrationStatus(status);
    if (MP_SUCCESS != res) {
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Registration Flow - getRegistrationStatus failed, error: %d\n", res);
        errorCode = MPA_AG_BIOS_PROTOCOL_ERROR;
        goto error;
    }

    // Check status.errorCode: 
    //          when BIOS/MCHECK reported error, the registration will be aborted
    //          when Agent itself reported error, the registration will continue.
    if ((status.errorCode != 0)  && !(MP_BIOS_SRC_ERROR & status.errorCode)) {
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Registration Flow - BIOS/MCHECK failed, reported error: 0x%02X\n", status.errorCode);
        // since it is one BIOS/MCHECK error, the Multi-package Agent server can't do anything, so just log the message and return
        return;
    }

    // Check that the RegistrationStatus.RegistrationComplete flag
    if (MP_TASK_IN_PROGRESS == status.registrationStatus) {
        MpRequestType reqType;
        res = m_uefi->getRequestType(reqType);
        if (MP_SUCCESS != res) {
            agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Registration Flow - getRequestType failed, error: %d\n", res);
            errorCode = MPA_AG_BIOS_PROTOCOL_ERROR;
            goto error;
        }

        if (MP_REQ_NONE != reqType) // There is a request pending
        {
            string serverUrl = "";
            uint16_t flags = 0;
            uint16_t serverIdSize = 0;
            res = m_uefi->getRegistrationServerInfo(flags, serverUrl, NULL, serverIdSize);
            if (MP_SUCCESS != res) {
                agent_log_message(MP_REG_LOG_LEVEL_ERROR, "getRegistrationServerInfo failed, error: %d\n", res);
                errorCode = MPA_AG_BIOS_PROTOCOL_ERROR;
                goto error;
            }
            agent_log_message(MP_REG_LOG_LEVEL_INFO, "Server URL: %s\n", serverUrl.c_str());

            if (!m_network) {
                m_network = new MPNetwork(serverUrl, 
                    string(m_conf.server_add_package_subscription_key), m_conf.proxy, m_gLogLevel);
                if (!m_network) {
                    agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Allocation error during network initiation.\n");
                    errorCode = MPA_AG_OUT_OF_MEMORY;
                    goto error;
                }
            }
            
            switch(reqType)
            {
                case MP_REQ_REGISTRATION:
                    agent_log_message(MP_REG_LOG_LEVEL_INFO, "Registration Flow - PLATFORM_ESTABLISHMENT or TCB_RECOVERY.\n");
                    
                    if (flags & SERVER_INFO_FLAG_RS_NOT_SAVE_KEYS) {
                        agent_log_message(MP_REG_LOG_LEVEL_FUNC, "SGX MP Server configuration flag indicates that Registration Server won't save encrypted platform keys.\n");
                        agent_log_message(MP_REG_LOG_LEVEL_FUNC, "Platform registration request (PLATFORM_MANIFEST) won't be send to Registration Server.\n");
                        agent_log_message(MP_REG_LOG_LEVEL_FUNC, "Please use management tool or PCKCertIDRetrievalTool to read PLATFORM_MANIFEST.\n");
                        break;
                    }

                    uint8_t manifest[MAX_REQUEST_SIZE];
                    requestSize = sizeof(manifest);
                    res = m_uefi->getRequest((uint8_t*)&manifest, requestSize);
                    if (MP_SUCCESS != res) {
                        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Registration Flow - getRegistrationRequest failed, error: %d\n", res);
                        errorCode = MPA_AG_BIOS_PROTOCOL_ERROR;
                        goto error;
                    }

                    pb = new (std::nothrow) PerformPlatformRegistration(m_network, m_uefi);
                    if(NULL != pb) {
                        registered = pb->perform((uint8_t*)&manifest, requestSize, REGISTRATION_RETRY_TIMES);
                        delete pb;
                    } else {
                        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Registration Flow - Memory has been used up.\n");
                        errorCode = MPA_AG_OUT_OF_MEMORY;
                        goto error;
                    }			    

                    if (registered) {
                        agent_log_message(MP_REG_LOG_LEVEL_FUNC, "Registration Flow - PLATFORM_ESTABLISHMENT or TCB_RECOVERY passed successfully.\n");
                    } else {
                        agent_log_message(MP_REG_LOG_LEVEL_FUNC, "Registration Flow - PLATFORM_ESTABLISHMENT or TCB_RECOVERY failed.\n");
                    }
                    break;
                case MP_REQ_ADD_PACKAGE:
                    agent_log_message(MP_REG_LOG_LEVEL_INFO, "Registration Flow - ADD_REQUEST.\n");

                    uint8_t addPackage[MAX_REQUEST_SIZE];
                    requestSize = sizeof(addPackage);
                    res = m_uefi->getRequest((uint8_t*)&addPackage, requestSize);
                    if (MP_SUCCESS != res) {
                        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Registration Flow - getAddPackageRequest failed, error: %d\n", res);
                        errorCode = MPA_AG_BIOS_PROTOCOL_ERROR;
                        goto error;
                    }

                    pb = new PerformPackageAdd(m_network, m_uefi);
                    registered = pb->perform((uint8_t*)&addPackage, requestSize, REGISTRATION_RETRY_TIMES);
                    delete pb;

                    if (registered) {
                        agent_log_message(MP_REG_LOG_LEVEL_FUNC, "Registration Flow - ADD_REQUEST passed successfully.\n");
                    } else {
                        agent_log_message(MP_REG_LOG_LEVEL_FUNC, "Registration Flow - ADD_REQUEST failed.\n");
                    }
                    break;
                default:
                    agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Registration Flow - Unknown_request - noting to do\n");
                    errorCode = MPA_AG_BIOS_PROTOCOL_ERROR;
                break;
            }
        }
        else
        {
            agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Registration Flow - Registration not complete but no RequestVariable available?\n");
        }
    }
    else
    {
        if(status.errorCode == MPA_SUCCESS) {
            agent_log_message(MP_REG_LOG_LEVEL_FUNC, "Registration Flow - Registration status indicates registration is completed successfully. MPA has nothing to do.\n");
        }
        else
        {
            agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Registration Flow - Registration status indicates registration is completed unsuccessfully, and the error code is %d. \n", status.errorCode);
        }
    }
    return;
    
error:
    // Error occurred before performing the required registration flow
    status.errorCode = errorCode;
    res = m_uefi->setRegistrationStatus(status);
    if (MP_SUCCESS != res) {
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "setRegistrationStatus failed, error: %d\n", res);
        return;
    }
}

RegistrationService::~RegistrationService() {
    delete m_uefi;
    delete m_network;
}
