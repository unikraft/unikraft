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
 * File: MultiPackageDefs.h 
 *   
 * Description: Definitions of the high-level data types used in the 
 * multipackage binaries.
 */
#ifndef __MULTI_PACKAGE_DEFS_H
#define __MULTI_PACKAGE_DEFS_H

#include <stdint.h>

#define MAX_PATH_SIZE           256
#define EFIVARS_FILE_SYSTEM     "/sys/firmware/efi/efivars/"

/* Libraries Definitions */

typedef enum {
    MP_SUCCESS = 0,
    MP_NO_PENDING_DATA = 1,
    MP_ALREADY_REGISTERED = 2,
    MP_MEM_ERROR = 3,
    MP_UEFI_INTERNAL_ERROR = 4,
    MP_USER_INSUFFICIENT_MEM = 5,
    MP_INVALID_PARAMETER = 6,
    MP_SGX_NOT_SUPPORTED = 7,
    MP_UNEXPECTED_ERROR = 8,
    MP_REDUNDANT_OPERATION = 9,
    MP_NETWORK_ERROR = 10,
    MP_NOT_INITIALIZED = 11,
    MP_INSUFFICIENT_PRIVILEGES = 12
} MpResult;

typedef enum {
    MP_TASK_IN_PROGRESS = 0,
    MP_TASK_COMPLETED = 1
} MpTaskStatus;

typedef enum {
    MP_REQ_REGISTRATION = 0,
    MP_REQ_ADD_PACKAGE = 1,
    MP_REQ_NONE = 2
} MpRequestType;

/* Registration Server HTTP 400 Response Error codes and agent error codes */
typedef enum _RegistrationErrorCode
{
    /* These are the possible error codes reported by the agent in the SgxRegsitrationStatus UEFI variable in the ErrorCode field */
    MPA_SUCCESS = 0x00,
    MPA_AG_UNEXPECTED_ERROR = 0x80,         /* Unexpected agent internal error. */
    MPA_AG_OUT_OF_MEMORY = 0x81,            /* Out-of-memory error. */
    MPA_AG_NETWORK_ERROR = 0x82,            /* Proxy detection or network communication error */
    MPA_AG_INVALID_PARAMETER = 0x83,        /* Invalid Parameter passed in */
    MPA_AG_INTERNAL_SERVER_ERROR = 0x84,    /* Internal server error occurred. */
    MPA_AG_SERVER_TIMEOUT = 0x85,           /* Server timeout reached */
    MPA_AG_BIOS_PROTOCOL_ERROR = 0x86,      /* BIOS Protocol error */
    MPA_AG_UNAUTHORIZED_ERROR = 0x87,       /* the client is unauthorized to access the registration server */

    /* Registration Server HTTP 400 Response Error details */
    MPA_RS_INVALID_REQUEST_SYNTAX = 0xA0,           /* The request could not be understood by the server due to malformed syntax. */
    MPA_RS_PM_INVALID_REGISTRATION_SERVER = 0XA1,   /* RS rejected request because it is intended for different Registration Server (Registration Server Authentication Key mismatch). */
    MPA_RS_INVALID_OR_REVOKED_PACKAGE = 0xA2,       /* RS rejected request due to invalid or revoked processor package. */
    MPA_RS_PACKAGE_NOT_FOUND = 0xA3,                /* RS rejected request as at least one of the processor packages could not be recognized by the server. */
    MPA_RS_PM_INCOMPATIBLE_PACKAGE = 0xA4,          /* RS rejected request as at least one of the processor packages is incompatible with rest of the packages on the platform. */
    MPA_RS_PM_INVALID_PLATFORM_MANIFEST = 0xA5,     /* RS rejected request due to invalid platform configuration. */
    MPA_RS_AD_PLATFORM_NOT_FOUND = 0xA6,            /* RS rejected request as provided platform instance is not recognized by the server.  */
    MPA_RS_AD_INVALID_ADD_REQUEST = 0xA7,           /* RS rejected request as the Add Package Payload was invalid. */
    MPA_RS_UNKOWN_ERROR = 0xA8,                     /* RS rejected request for unknown reason.  Probably means RS Agent needs to be updated with newly defined RS errors */
} RegistrationErrorCode;

/* These are the possible HTTP status codes return from the registration server */
typedef enum _HttpResponseCode
{
    MPA_RS_PLATFORM_CREATED = 201,          /* 201 Operation successful - a new platform instance has been registered in the back-end. */
    MPA_RS_PACKAGES_BEEN_ADDED = 200,       /* 200 Operation successful - packages have been added to an existing platform instance. */
    MPA_RS_BAD_REQUEST = 400,               /* Invalid payload. The client should not repeat the request without modifications. */
    MPA_RS_FAIL_UNAUTHORIZED = 401,         /* Failed to authenticate or authorize the request. */
    MPA_RS_INTERNAL_SERVER_ERROR = 500,     /* Internal server error occurred. */
    MPA_RS_SERVICE_UNAVAILABLE = 503,       /* Server is currently unable to process the request. The client should try to repeat its request after some time. */
    MPA_RS_UNSUPPORTED_MEDIA_TYPE = 415,    /* MIME type specified in the request is not supported by the server. */
} HttpStatusCode;

typedef struct {
    union {
        uint16_t status;
        struct {
            uint16_t registrationStatus:1;
            uint16_t packageInfoStatus:1;
            uint16_t reservedStatus:14;
        };
    };
    RegistrationErrorCode errorCode;
} MpRegistrationStatus;

typedef enum _ProxyType
{
    MP_REG_PROXY_TYPE_DEFAULT_PROXY = 0,
    MP_REG_PROXY_TYPE_DIRECT_ACCESS = 1,
    MP_REG_PROXY_TYPE_MANUAL_PROXY = 2,
    MP_REG_PROXY_TYPE_MAX_VALUE
} ProxyType;

typedef struct _ProxyConf{
    ProxyType proxy_type;
    char proxy_url[MAX_PATH_SIZE];
} ProxyConf;

typedef enum _MpSgxStatus {
    MP_SGX_ENABLED,
    MP_SGX_DISABLED_REBOOT_REQUIRED,    /* A reboot is required to finish enabling SGX */
    MP_SGX_DISABLED_LEGACY_OS,          /* SGX is disabled and a Software Control Interface is not available to enable it */
    MP_SGX_DISABLED,                    /* SGX is not enabled on this platform. More details are unavailable. */
    MP_SGX_DISABLED_SCI_AVAILABLE,      /* SGX is disabled, but a Software Control Interface is available to enable it */
    MP_SGX_DISABLED_MANUAL_ENABLE,      /* SGX is disabled, but can be enabled manually in the BIOS setup */
    MP_SGX_DISABLED_HYPERV_ENABLED,     /* Detected an unsupported version of Windows* 10 with Hyper-V enabled */
    MP_SGX_DISABLED_UNSUPPORTED_CPU,    /* SGX is not supported by this CPU */
} MpSgxStatus;

typedef enum _LogLevel
{
    MP_REG_LOG_LEVEL_NONE = 0,
    MP_REG_LOG_LEVEL_FUNC = 1,
    MP_REG_LOG_LEVEL_ERROR = 2,
    MP_REG_LOG_LEVEL_INFO = 3,
    MP_REG_LOG_LEVEL_MAX_VALUE
} LogLevel;


#endif  // #ifndef __MULTI_PACKAGE_DEFS_H

