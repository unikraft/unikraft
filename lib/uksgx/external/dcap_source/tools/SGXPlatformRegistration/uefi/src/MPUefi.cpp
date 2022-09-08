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
 * File: MPUefi.cpp
 *   
 * Description: Linux specific implementation for the MPUefi class to 
 * communicate with the BIOS UEFI variables.
 */
#include <string.h>
#ifdef _WIN32
#include "WinUefi.h"
#include <regex>
#else
#include "FSUefi.h"
#include <regex.h>
#endif
#include "MPUefi.h"
#include "UefiVar.h"
#include "uefi_logger.h"

#define REGISTRATION_COMPLETE_BIT_MASK 0x0001
#define PACKAGE_INFO_COMPLETE_BIT_MASK 0x0002

#define URL_REGEX "(http://www.|https://www.|http://|https://)?[a-z0-9]+([-.][a-z0-9]+)*.[a-z]{2,5}(:[0-9]{1,5})?(/.*)?"

// defines for values verification
#define MP_VERIFY_UEFI_STRUCT_READ              1
#define MP_VERIFY_UEFI_VERSION_READ             1
//#define MP_VERIFY_INTERNAL_DATA_STRUCT_READ     1
//#define MP_VERIFY_INTERNAL_DATA_STRUCT_WRITE    1

#if MP_VERIFY_INTERNAL_DATA_STRUCT_READ == 1 || MP_VERIFY_INTERNAL_DATA_STRUCT_WRITE == 1
#include "../../test/src/CommonTypes.h"
#endif

MPUefi::MPUefi(const string path, const LogLevel logLevel)
{
    m_logLevel = logLevel;
#ifdef _WIN32
    m_uefi = new WinUefi(m_logLevel);
#else
    string desiredUefiPath = path;
    if (desiredUefiPath.empty()) {
        // deafult value
        desiredUefiPath = string(EFIVARS_FILE_SYSTEM);
    }
    m_uefi = new FSUefi(desiredUefiPath, m_logLevel);
#endif
}

MpResult MPUefi::getRequestType(MpRequestType& type) {
    MpResult res = MP_SUCCESS;
    size_t varDataSize = 0;
    SgxUefiVar *requestUefi = 0;

    do {
        requestUefi = (SgxUefiVar*)m_uefi->readUEFIVar(UEFI_VAR_SERVER_REQUEST, varDataSize);
        if (requestUefi == 0) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequestType: SgxRegistrationServerRequest UEFI variable was not found.\n");
            type = MP_REQ_NONE;
            break;
        }

#ifdef MP_VERIFY_UEFI_VERSION_READ
        // structure version check
        if ((requestUefi->version != MP_BIOS_UEFI_VARIABLE_VERSION_1) &&
            (requestUefi->version != MP_BIOS_UEFI_VARIABLE_VERSION_2)){
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequestType: version check failed. version: %d\n", requestUefi->version);
            res = MP_UEFI_INTERNAL_ERROR;
            break;
        }
#endif

#ifdef MP_VERIFY_UEFI_STRUCT_READ
        if (varDataSize != sizeof(requestUefi->version) + sizeof(requestUefi->size) + requestUefi->size) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequestType: SgxRegistrationServerRequest UEFI size is invalid.\n");
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequestType: actual size: %d, expected size: %d\n", varDataSize,
                sizeof(requestUefi->version) + sizeof(requestUefi->size) + requestUefi->size);
            res = MP_UEFI_INTERNAL_ERROR;
            break;
        }
#endif

        // set request type
        if (0 == memcmp(requestUefi->header.guid, PlatformManifest_GUID, GUID_SIZE)) {
            type = MP_REQ_REGISTRATION;
        }
        else if (0 == memcmp(requestUefi->header.guid, AddRequest_GUID, GUID_SIZE)) {
            type = MP_REQ_ADD_PACKAGE;
        }
        else {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequestType: request GUID doesn't match expected GUID's\n");
            const uint8_t* actual = requestUefi->header.guid;
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequestType: GUID from SgxRegistrationServerRequest UEFI:\n");
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                actual[0], actual[1], actual[2], actual[3], actual[4], actual[5],
                actual[6], actual[7], actual[8], actual[9], actual[10], actual[11],
                actual[12], actual[13], actual[14], actual[15]);
            res = MP_UEFI_INTERNAL_ERROR;
            break;
        }
    } while (0);

    if (requestUefi) {
        delete[](uint8_t*)requestUefi;
    }
    return res;
}

MpResult MPUefi::getRequest(uint8_t *request, uint16_t &requestSize) {
    MpResult res = MP_SUCCESS;
    size_t varDataSize = 0;
    SgxUefiVar *requestUefi = NULL;
#if MP_VERIFY_INTERNAL_DATA_STRUCT_READ == 1
    unsigned int i = 0;
    unsigned int pmCount = 0;
#endif

    do {
        requestUefi = (SgxUefiVar*)m_uefi->readUEFIVar(UEFI_VAR_SERVER_REQUEST, varDataSize);
        if (requestUefi == 0) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: SgxRegistrationServerRequest UEFI variable was not found. error: %d\n", errno);
            res = MP_NO_PENDING_DATA;
            break;
        }

#ifdef MP_VERIFY_UEFI_VERSION_READ
        if ((MP_BIOS_UEFI_VARIABLE_VERSION_1 != requestUefi->version) &&
            (MP_BIOS_UEFI_VARIABLE_VERSION_2 != requestUefi->version)){
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: version check failed, UEFI version: %d, expected UEFI version: %d or %d\n", requestUefi->version, MP_BIOS_UEFI_VARIABLE_VERSION_1, MP_BIOS_UEFI_VARIABLE_VERSION_2);
            res = MP_INVALID_PARAMETER;
            break;
        }
#endif

#ifdef MP_VERIFY_UEFI_STRUCT_READ
        if (varDataSize != sizeof(requestUefi->version) + sizeof(requestUefi->size) + requestUefi->size) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: SgxRegistrationServerRequest UEFI size is invalid.\n");
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: actual size: %d, expected size: %d\n", varDataSize,
                sizeof(requestUefi->version) + sizeof(requestUefi->size) + requestUefi->size);
            res = MP_UEFI_INTERNAL_ERROR;
            break;
        }
#endif

#if MP_VERIFY_INTERNAL_DATA_STRUCT_READ == 1
        if (0 == memcmp(requestUefi->header.guid, PlatformManifest_GUID, GUID_SIZE)) {    
  	    // structure version check
            if (MP_STRUCTURE_VERSION != requestUefi->header.version) {
                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: PlatformManifest structure version check failed. version number: %d\n", requestUefi->header.version);
                res = MP_INVALID_PARAMETER;
                break;
            }

            if (MP_BIOS_UEFI_VARIABLE_VERSION_1 == requestUefi->version) {
                // check expected request size
                if (sizeof(PlatformManifest) != requestUefi->size) {
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: PlatformManifest structure size is not as expected.\n");
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: actual: %d expected: %d\n", requestUefi->size, sizeof(PlatformManifest));
                    res = MP_NO_PENDING_DATA;
                    break;
                }
                pmCount = 1;
            }

            for(i=0; i<pmCount; i++) {
                // verify platform manifest structure
                PlatformManifest *manifest = (PlatformManifest*)(((uint8_t*)(&requestUefi->header)) + sizeof(PlatformManifest)*i);
                if ((manifest->header.size != (sizeof(*manifest) - sizeof(manifest->header))) ||
                    (MP_STRUCTURE_VERSION != manifest->header.version) ||
                    (0 != memcmp(manifest->header.guid, PlatformManifest_GUID, GUID_SIZE))) {
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: PlatformManifest structure is invalid.\n");
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: manifest->header.size: %d, \
                    sizeof(*manifest): %d, sizeof(manifest->header): %d\n",
                        manifest->header.size, sizeof(*manifest),
                        sizeof(manifest->header));

                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: manifest->header.version: %d\n", manifest->header.version);
    
                    const uint8_t* actual = manifest->header.guid;
                    const uint8_t* expected = PlatformManifest_GUID;
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: actual PlatformManifest_GUID:\n");
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                        actual[0], actual[1], actual[2], actual[3], actual[4], actual[5],
                        actual[6], actual[7], actual[8], actual[9], actual[10], actual[11],
                        actual[12], actual[13], actual[14], actual[15]);
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: expected PlatformManifest_GUID:\n");
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                        expected[0], expected[1], expected[2], expected[3], expected[4], expected[5],
                        expected[6], expected[7], expected[8], expected[9], expected[10], expected[11],
                        expected[12], expected[13], expected[14], expected[15]);
    
                    res = MP_UEFI_INTERNAL_ERROR;
                    break;
                }
    
                if ((manifest->platformInfo.header.size !=
                    (sizeof(manifest->platformInfo) - sizeof(manifest->platformInfo.header))) ||
                    (0 != memcmp(manifest->platformInfo.header.guid, PlatformInfo_GUID, GUID_SIZE))) {
    
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: PlatformInfo structure is invalid.\n");
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: manifest->platformInfo.header.size: %d, \
                    sizeof(manifest->platformInfo): %d, sizeof(manifest->platformInfo.header): %d\n",
                        manifest->platformInfo.header.size, sizeof(manifest->platformInfo),
                        sizeof(manifest->platformInfo.header));
    
                    const uint8_t* actual = manifest->platformInfo.header.guid;
                    const uint8_t* expected = PlatformInfo_GUID;
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: actual PlatformInfo_GUID:\n");
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                        actual[0], actual[1], actual[2], actual[3], actual[4], actual[5],
                        actual[6], actual[7], actual[8], actual[9], actual[10], actual[11],
                        actual[12], actual[13], actual[14], actual[15]);
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: expected PlatformInfo_GUID:\n");
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                        expected[0], expected[1], expected[2], expected[3], expected[4], expected[5],
                        expected[6], expected[7], expected[8], expected[9], expected[10], expected[11],
                        expected[12], expected[13], expected[14], expected[15]);
    
                    res = MP_UEFI_INTERNAL_ERROR;
                    break;
                }
    
                if ((manifest->encryptedPlatformKey.header.size !=
                    (sizeof(manifest->encryptedPlatformKey) - sizeof(manifest->encryptedPlatformKey.header))) ||
                    (0 != memcmp(manifest->encryptedPlatformKey.header.guid, EncryptedPlatformKey_GUID, GUID_SIZE))) {
    
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: EncryptedPlatformKey structure is invalid.\n");
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: manifest->encryptedPlatformKey.header.size: %d, \
                    sizeof(manifest->encryptedPlatformKey): %d, sizeof(manifest->encryptedPlatformKey.header): %d\n",
                        manifest->encryptedPlatformKey.header.size, sizeof(manifest->encryptedPlatformKey),
                        sizeof(manifest->encryptedPlatformKey.header));
    
                    const uint8_t* actual = manifest->encryptedPlatformKey.header.guid;
                    const uint8_t* expected = EncryptedPlatformKey_GUID;
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: actual EncryptedPlatformKey_GUID:\n");
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                        actual[0], actual[1], actual[2], actual[3], actual[4], actual[5],
                        actual[6], actual[7], actual[8], actual[9], actual[10], actual[11],
                        actual[12], actual[13], actual[14], actual[15]);
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: expected EncryptedPlatformKey_GUID:\n");
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                        expected[0], expected[1], expected[2], expected[3], expected[4], expected[5],
                        expected[6], expected[7], expected[8], expected[9], expected[10], expected[11],
                        expected[12], expected[13], expected[14], expected[15]);
                    res = MP_UEFI_INTERNAL_ERROR;
                    break;
                }
    
                for (i = 0; i < manifest->platformInfo.PackNumber; i++) {
                    if ((0 != memcmp(manifest->pairingReceipts[i].header.guid, PairingReceipt_GUID, GUID_SIZE))
                        || (manifest->pairingReceipts[i].header.size !=
                        (sizeof(manifest->pairingReceipts[i]) - sizeof(manifest->pairingReceipts[i].header)))) {
    
                        uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: PairingReceipt structure is invalid.\n");
                        uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: i: %d, manifest->pairingReceipts[i].header.size: %d, \
                            sizeof(manifest->pairingReceipts[i]): %d, sizeof(manifest->pairingReceipts[i].header): %d\n",
                            i, manifest->pairingReceipts[i].header.size, sizeof(manifest->pairingReceipts[i]),
                            sizeof(manifest->pairingReceipts[i].header));
    
                        const uint8_t* actual = manifest->pairingReceipts[i].header.guid;
                        const uint8_t* expected = PairingReceipt_GUID;
                        uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: actual PairingReceipt_GUID:\n");
                        uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                            actual[0], actual[1], actual[2], actual[3], actual[4], actual[5],
                            actual[6], actual[7], actual[8], actual[9], actual[10], actual[11],
                            actual[12], actual[13], actual[14], actual[15]);
                        uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: expected PairingReceipt_GUID:\n");
                        uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                            expected[0], expected[1], expected[2], expected[3], expected[4], expected[5],
                            expected[6], expected[7], expected[8], expected[9], expected[10], expected[11],
                            expected[12], expected[13], expected[14], expected[15]);
    
                        res = MP_UEFI_INTERNAL_ERROR;
                        break;
                    }
                }
            }
        }
        else if (0 == memcmp(requestUefi->header.guid, AddRequest_GUID, GUID_SIZE)) {
            // structure version check
            if (MP_STRUCTURE_VERSION != requestUefi->header.version) {
                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: AddPackage structure version check failed. version number: %d\n", requestUefi->header.version);
                res = MP_INVALID_PARAMETER;
                break;
            }

            // check expected request size
            if (requestUefi->size != sizeof(AddRequest)) {
                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: AddPackage structure size is not as expected.\n");
                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: actual: %d expected: %d\n", requestUefi->size, sizeof(AddRequest));
                res = MP_NO_PENDING_DATA;
                break;
            }

            // verify add package structure
            AddRequest *addRequest = (AddRequest*)&requestUefi->header;
            if ((0 != memcmp(addRequest->header.guid, AddRequest_GUID, GUID_SIZE)) ||
                (addRequest->header.size !=
                (sizeof(*addRequest) - sizeof(addRequest->header)))) {

                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: AddRequest structure is invalid.\n");
                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: addRequest->header.size: %d, sizeof(*addRequest): %d, sizeof(addRequest->header): %d\n",
                    addRequest->header.size, sizeof(*addRequest), sizeof(addRequest->header));

                const uint8_t* actual = addRequest->header.guid;
                const uint8_t* expected = AddRequest_GUID;
                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: actual AddRequest_GUID:\n");
                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                    actual[0], actual[1], actual[2], actual[3], actual[4], actual[5],
                    actual[6], actual[7], actual[8], actual[9], actual[10], actual[11],
                    actual[12], actual[13], actual[14], actual[15]);
                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: expected AddRequest_GUID:\n");
                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                    expected[0], expected[1], expected[2], expected[3], expected[4], expected[5],
                    expected[6], expected[7], expected[8], expected[9], expected[10], expected[11],
                    expected[12], expected[13], expected[14], expected[15]);

                res = MP_UEFI_INTERNAL_ERROR;
                break;
            }
        }
#endif
        if (request) {
            if (requestSize < requestUefi->size) {
                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: Request buffer too small for pending request, given size: %d, actual size: %d.\n",
                    requestSize, requestUefi->size);
                res = MP_USER_INSUFFICIENT_MEM;
            }
            else {
                memcpy(request, &(requestUefi->header), requestUefi->size);
            }
        }
        requestSize = requestUefi->size;
    } while (0);


    if (requestUefi) {
        delete[](uint8_t*)requestUefi;
    }
    return res;
}

MpResult MPUefi::setServerResponse(const uint8_t *response, const uint16_t &size) {
    MpResult res = MP_SUCCESS;
    uint8_t responseBuff[MAX_RESPONSE_SIZE + sizeof(SgxUefiVar) - sizeof(StructureHeader)];
    SgxUefiVar *responseUefi = (SgxUefiVar*)responseBuff;

    do {
        if (NULL == response || 0 == size) {
            res = MP_INVALID_PARAMETER;
            break;
        }
        // zero response uefi structure
        memset(responseUefi, 0, sizeof(SgxUefiVar));

        responseUefi->version = MP_BIOS_UEFI_VARIABLE_VERSION_1;
        responseUefi->size = size;

        // copy cets to uefi structure
        memcpy(&(responseUefi->header), response, size);

#if MP_VERIFY_INTERNAL_DATA_STRUCT_WRITE == 1
        // verify PlatformMembershipCertificate response size
        if (0 != size % sizeof(PlatformMembershipCertificate)) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "setServerResponse: response size check failed. response should contain PlatformMembershipCertificates, reponse size: %d, PlatformMembershipCertificate size: %d\n", size, sizeof(PlatformMembershipCertificate));
            res = MP_INVALID_PARAMETER;
            break;
        }

        // verify platform membership structure
        for (size_t i = 0; i < size / sizeof(PlatformMembershipCertificate); i++) {
            const PlatformMembershipCertificate *certs = (const PlatformMembershipCertificate *)response;
            // structure version check
            if (MP_STRUCTURE_VERSION != certs[i].header.version) {
                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "setServerResponse: version check failed on cert %d, version number: %d\n", i, certs[i].header.version);
                res = MP_INVALID_PARAMETER;
                break;
            }

            if ((0 != memcmp(certs[i].header.guid, PlatformMemberShip_GUID, GUID_SIZE)) ||
                (certs[i].header.size != (sizeof(certs[i]) - sizeof(certs[i].header)))) {

                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "setServerResponse: PlatformMemberShip structure is invalid.\n");
                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "setServerResponse: i: %d, certs[i].header.size: %d, sizeof(certs[i]): %d, sizeof(certs[i].header): %d\n",
                    i, certs[i].header.size, sizeof(certs[i]), sizeof(certs[i].header));

                const uint8_t* actual = certs[i].header.guid;
                const uint8_t* expected = PlatformMemberShip_GUID;
                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "setServerResponse: actual PlatformMemberShip_GUID:\n");
                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                    actual[0], actual[1], actual[2], actual[3], actual[4], actual[5],
                    actual[6], actual[7], actual[8], actual[9], actual[10], actual[11],
                    actual[12], actual[13], actual[14], actual[15]);
                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "setServerResponse: expected PlatformMemberShip_GUID:\n");
                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                    expected[0], expected[1], expected[2], expected[3], expected[4], expected[5],
                    expected[6], expected[7], expected[8], expected[9], expected[10], expected[11],
                    expected[12], expected[13], expected[14], expected[15]);

                res = MP_INVALID_PARAMETER;
                break;
            }
        }
#endif
        // write certs to uefi: for the UEFI variable, it has one 4 bytes header: 2 bytes for version, 2 bytes for size
        int numOfBytes = m_uefi->writeUEFIVar(UEFI_VAR_SERVER_RESPONSE, (const uint8_t*)(responseUefi), responseUefi->size + 4, true);
        if (numOfBytes != responseUefi->size + 4) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "setServerResponse: failed to write uefi variable.\n");
            res = MP_UEFI_INTERNAL_ERROR;
            break;
        }
    } while (0);

    return res;
}

MpResult MPUefi::getKeyBlobs(uint8_t *blobs, uint16_t &blobsSize) {
    MpResult res = MP_SUCCESS;
    size_t varDataSize = 0;
    SgxUefiVar *packageInfoUefi = NULL;

    do {
        packageInfoUefi = (SgxUefiVar*)m_uefi->readUEFIVar(UEFI_VAR_PACKAGE_INFO, varDataSize);
        if (packageInfoUefi == 0) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getKeyBlobs: SgxRegistrationPackageInfo UEFI variable was not found. error: %d\n", errno);
            res = MP_NO_PENDING_DATA;
            break;
        }

#ifdef MP_VERIFY_UEFI_VERSION_READ
        // structure version check
        if (MP_BIOS_UEFI_VARIABLE_VERSION_1 != packageInfoUefi->version) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getKeyBlobs: version check failed, UEFI version: %d, expected UEFI version: %d.\n", packageInfoUefi->version, MP_BIOS_UEFI_VARIABLE_VERSION_1);
            res = MP_UEFI_INTERNAL_ERROR;
            break;
        }
#endif

#ifdef MP_VERIFY_UEFI_STRUCT_READ
        // uefi structure size check
        if (varDataSize != sizeof(packageInfoUefi->version) + sizeof(packageInfoUefi->size) + packageInfoUefi->size) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getKeyBlobs: SgxRegistrationPackageInfo UEFI size is invalid.\n");
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getKeyBlobs: actual size: %d, expected size: %d\n", varDataSize,
                sizeof(packageInfoUefi->version) + sizeof(packageInfoUefi->size) + packageInfoUefi->size);
            res = MP_UEFI_INTERNAL_ERROR;
            break;
        }
#endif

#if MP_VERIFY_INTERNAL_DATA_STRUCT_WRITE == 1
        // structure size check
        if (packageInfoUefi->size != sizeof(KeyBlob)*MAX_SOCKETS) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getKeyBlobs: keyBlob structure size is not as expected.\n");
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getKeyBlobs: actual: %d expected: %d\n", packageInfoUefi->size, sizeof(KeyBlob)*MAX_SOCKETS);
            res = MP_UEFI_INTERNAL_ERROR;
            break;
        }

        // verify key blobs structure
        KeyBlob *keyBlobs = (KeyBlob*)&packageInfoUefi->header;
        for (unsigned int i = 0; i < MAX_SOCKETS; i++) {
            if (keyBlobs[i].header.size != 0) {
                if (MP_STRUCTURE_VERSION != keyBlobs[i].header.version) {
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getKeyBlobs: version check failed on keyblob %d, version number: %d\n", i, keyBlobs[i].header.version);
                    res = MP_INVALID_PARAMETER;
                    break;
                }

                if ((0 != memcmp(keyBlobs[i].header.guid, KeyBlob_GUID, GUID_SIZE)) ||
                    (keyBlobs[i].header.size != (sizeof(keyBlobs[i]) - sizeof(keyBlobs[i].header)))) {

                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getKeyBlobs: KeyBlob structure is invalid.\n");
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getKeyBlobs: i: %d, keyBlobs[i].header.size: %d, sizeof(keyBlobs[i]): %d, sizeof(keyBlobs[i].header): %d\n",
                        i, keyBlobs[i].header.size, sizeof(keyBlobs[i]), sizeof(keyBlobs[i].header));

                    const uint8_t* actual = keyBlobs[i].header.guid;
                    const uint8_t* expected = KeyBlob_GUID;
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getKeyBlobs: actual KeyBlob_GUID:\n");
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                        actual[0], actual[1], actual[2], actual[3], actual[4], actual[5],
                        actual[6], actual[7], actual[8], actual[9], actual[10], actual[11],
                        actual[12], actual[13], actual[14], actual[15]);
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getKeyBlobs: expected KeyBlob_GUID:\n");
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                        expected[0], expected[1], expected[2], expected[3], expected[4], expected[5],
                        expected[6], expected[7], expected[8], expected[9], expected[10], expected[11],
                        expected[12], expected[13], expected[14], expected[15]);

                    res = MP_UEFI_INTERNAL_ERROR;
                    break;
                }
            }
        }
#endif

        if (blobs) {
            if (blobsSize < packageInfoUefi->size) {
                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRequest: Request buffer too small for pending request, given size: %d, actual size: %d.\n",
                    blobsSize, packageInfoUefi->size);
                res = MP_USER_INSUFFICIENT_MEM;
            }
            else {
                memcpy(blobs, &(packageInfoUefi->header), packageInfoUefi->size);
            }
        }
        blobsSize = packageInfoUefi->size;
    } while (0);


    if (packageInfoUefi) {
        delete[](uint8_t*)packageInfoUefi;
    }
    return res;
}

MpResult MPUefi::getRegistrationStatus(MpRegistrationStatus& status) {
    MpResult res = MP_SUCCESS;
    size_t varDataSize = 0;
    RegistrationStatusUEFI *statusUefi = 0;

    do {
        statusUefi = (RegistrationStatusUEFI*)m_uefi->readUEFIVar(UEFI_VAR_STATUS, varDataSize);
        if (statusUefi == 0 || varDataSize != sizeof(RegistrationStatusUEFI)) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRegistrationStatus: SgxRegistrationStatus UEFI variable was not found or size not as expected.\n");
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRegistrationStatus: SgxRegistrationStatus acutal size: %d, expected size: %d\n", varDataSize, sizeof(RegistrationStatusUEFI));
            res = MP_UEFI_INTERNAL_ERROR;
            break;
        }

#ifdef  MP_VERIFY_UEFI_VERSION_READ
        // structure version check
        if (statusUefi->version != MP_BIOS_UEFI_VARIABLE_VERSION_1) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRegistrationStatus: version check failed.\n");
            res = MP_UEFI_INTERNAL_ERROR;
            break;
        }
#endif

#ifdef MP_VERIFY_UEFI_STRUCT_READ
        // uefi structure size check
        if (statusUefi->size != sizeof(statusUefi->status) + sizeof(statusUefi->errorCode)) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRegistrationStatus: SgxRegistrationStatus structure size not as expected.\n");
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRegistrationStatus: statusUefi->size: %d, sizeof(statusUefi->status): %d, sizeof(statusUefi->errorCode): %d\n",
                statusUefi->size, sizeof(statusUefi->status), sizeof(statusUefi->errorCode));
            res = MP_UEFI_INTERNAL_ERROR;
            break;
        }
#endif
        memset(&status, 0, sizeof(status));

        if (statusUefi->status & REGISTRATION_COMPLETE_BIT_MASK) {
            status.registrationStatus = MP_TASK_COMPLETED;
        }
        if (statusUefi->status & PACKAGE_INFO_COMPLETE_BIT_MASK) {
            status.packageInfoStatus = MP_TASK_COMPLETED;
        }
        status.errorCode = (RegistrationErrorCode)statusUefi->errorCode;
    } while (0);

    if (statusUefi) {
        delete[](uint8_t*)statusUefi;
    }

    return res;
}

MpResult MPUefi::setRegistrationStatus(const MpRegistrationStatus& status) {
    MpResult res = MP_SUCCESS;
    RegistrationStatusUEFI statusUefi;

    // zero all response uefi structure
    memset(&(statusUefi), 0, sizeof(statusUefi));

    do {
        statusUefi.version = MP_BIOS_UEFI_VARIABLE_VERSION_1;
        statusUefi.size = (uint16_t)(sizeof(statusUefi.status) + sizeof(statusUefi.errorCode));

        if (status.registrationStatus == MP_TASK_COMPLETED) {
            statusUefi.status |= REGISTRATION_COMPLETE_BIT_MASK;
        }
        if (status.packageInfoStatus == MP_TASK_COMPLETED) {
            statusUefi.status |= PACKAGE_INFO_COMPLETE_BIT_MASK;
        }
        uefi_log_message(MP_REG_LOG_LEVEL_INFO, "setRegistrationStatus: status.status = 0x%02x, statusUefi.status = 0x%02x.\n", status.status, statusUefi.status);

        statusUefi.errorCode = status.errorCode;

        // write registration status to uefi
        int numOfBytes = m_uefi->writeUEFIVar(UEFI_VAR_STATUS, (const uint8_t*)(&statusUefi), sizeof(statusUefi), false);
        if (numOfBytes != sizeof(statusUefi)) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "setRegistrationStatus: failed to write uefi variable.\n");
            res = MP_UEFI_INTERNAL_ERROR;
            break;
        }
    } while (0);

    return res;
}

MpResult MPUefi::getRegistrationServerInfo(uint16_t &flags, string &serverAddress, uint8_t *serverId, uint16_t &serverIdSize) {
    MpResult res = MP_SUCCESS;
    size_t varDataSize = 0;
    uint16_t requiredSize = 0;
    ConfigurationUEFI *configurationUefi = 0;
#if MP_VERIFY_INTERNAL_DATA_STRUCT_READ == 1
    RegistrationServerID *registrationServerID;
#endif

    do {
        configurationUefi = (ConfigurationUEFI *)m_uefi->readUEFIVar(UEFI_VAR_CONFIGURATION, varDataSize);
        if (configurationUefi == 0) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRegistrationServerInfo: SgxRegistrationConfiguration UEFI variable was not found.\n");
            res = MP_UEFI_INTERNAL_ERROR;
            break;
        }

#ifdef  MP_VERIFY_UEFI_VERSION_READ
        // structure version check
        if (configurationUefi->version != MP_BIOS_UEFI_VARIABLE_VERSION_1) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRegistrationServerInfo: version check failed.\n");
            res = MP_UEFI_INTERNAL_ERROR;
            break;
        }
#endif


#ifdef MP_VERIFY_UEFI_STRUCT_READ
        if (varDataSize != configurationUefi->size + sizeof(configurationUefi->version) + sizeof(configurationUefi->size)) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRegistrationServerInfo: RegistrationServerInfo UEFI size is invalid.\n");
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRegistrationServerInfo: actual size: %d, expected size: %d\n", varDataSize,
                configurationUefi->size + sizeof(configurationUefi->version) + sizeof(configurationUefi->size));
            res = MP_UEFI_INTERNAL_ERROR;
            break;
        }
#endif
        // we assume that REGISTRATION_SERVER_INFO is part of the UEFI defenition so we need to verify it always
        if (MP_STRUCTURE_VERSION != configurationUefi->headerInfo.version) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRegistrationServerInfo: version check failed ServerInfo, version number: %d\n", configurationUefi->headerInfo.version);
            res = MP_INVALID_PARAMETER;
            break;
        }

#if MP_VERIFY_INTERNAL_DATA_STRUCT_READ == 1
        // verify server info structure
        if (0 != memcmp(configurationUefi->headerInfo.guid, RegistrationServerInfo_GUID, GUID_SIZE)) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRegistrationServerInfo: RegistrationServerInfo structure is invalid.\n");
            const uint8_t* actual = configurationUefi->headerInfo.guid;
            const uint8_t* expected = RegistrationServerInfo_GUID;
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "actual RegistrationServerInfo_GUID:\n");
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                actual[0], actual[1], actual[2], actual[3], actual[4], actual[5],
                actual[6], actual[7], actual[8], actual[9], actual[10], actual[11],
                actual[12], actual[13], actual[14], actual[15]);
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "expect RegistrationServerInfo_GUID:\n");
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                expected[0], expected[1], expected[2], expected[3], expected[4], expected[5],
                expected[6], expected[7], expected[8], expected[9], expected[10], expected[11],
                expected[12], expected[13], expected[14], expected[15]);

            res = MP_UEFI_INTERNAL_ERROR;
            break;
        }

        registrationServerID = (RegistrationServerID*)&configurationUefi->headerId;
        if (configurationUefi->headerInfo.size != (sizeof(RegistrationServerInfo) - sizeof(configurationUefi->headerInfo))) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRegistrationServerInfo: RegistrationServerInfo structure is invalid.\n");
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRegistrationServerInfo: configurationUefi->headerInfo.size: %d, \
                expected size: %d\n", configurationUefi->headerInfo.size, sizeof(RegistrationServerInfo) - sizeof(configurationUefi->headerInfo));

        }

        if ((0 != memcmp(registrationServerID->header.guid, RegistrationServerID_GUID, GUID_SIZE)) ||
            (registrationServerID->header.size !=
            (sizeof(*registrationServerID) - sizeof(registrationServerID->header)))) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRegistrationServerInfo: registrationServerID structure is invalid.\n");
            const uint8_t* actual = registrationServerID->header.guid;
            const uint8_t* expected = RegistrationServerID_GUID;
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "actual RegistrationServerID_GUID:\n");
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                actual[0], actual[1], actual[2], actual[3], actual[4], actual[5],
                actual[6], actual[7], actual[8], actual[9], actual[10], actual[11],
                actual[12], actual[13], actual[14], actual[15]);
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "expect RegistrationServerID_GUID:\n");
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                expected[0], expected[1], expected[2], expected[3], expected[4], expected[5],
                expected[6], expected[7], expected[8], expected[9], expected[10], expected[11],
                expected[12], expected[13], expected[14], expected[15]);

            res = MP_UEFI_INTERNAL_ERROR;
            break;
        }
#endif

        // copy flags and server address
        flags = configurationUefi->flags;
        serverAddress = string((const char*)configurationUefi->url, (size_t)configurationUefi->urlSize);

        requiredSize = (uint16_t)(configurationUefi->headerId.size + (uint16_t)sizeof(configurationUefi->headerId));
        if (serverId) {
            if (serverIdSize < requiredSize) {
                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "getRegistrationServerInfo: Request buffer too small for pending request, given size: %d, actual size: %d.\n",
                    serverIdSize, requiredSize);
                res = MP_USER_INSUFFICIENT_MEM;
            }
            else {
                memcpy(serverId, &(configurationUefi->headerId), requiredSize);
            }
        }
        serverIdSize = requiredSize;
    } while (0);

    if (configurationUefi) {
        delete[](uint8_t*)configurationUefi;
    }

    return res;
}

MpResult MPUefi::setRegistrationServerInfo(const uint16_t &flags, const string &serverAddress, const uint8_t *serverId, const uint16_t &serverIdSize) {
    MpResult res = MP_SUCCESS;
    int ret = 0;
    uint8_t *buff = NULL;
    ConfigurationUEFI *configurationUefi = NULL;
#ifndef _WIN32
    regex_t regex;
#else
    std::cmatch match;
    std::regex regex(URL_REGEX);
    std::regex_constants::match_flag_type flag =
        std::regex_constants::match_default;
#endif
#if MP_VERIFY_INTERNAL_DATA_STRUCT_WRITE == 1
    RegistrationServerID *registrationServerID;
#endif

    do {
        if (NULL == serverId || 0 == serverIdSize) {
            res = MP_INVALID_PARAMETER;
            break;
        }

        if (MAX_URL_SIZE < serverAddress.length()) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "setRegistrationServerInfo: URL length is too long. \n", res);
            res = MP_INVALID_PARAMETER;
            break;
        }

#ifndef _WIN32
        /* create regular expression */
        ret = regcomp(&regex, URL_REGEX, REG_EXTENDED);
        if (ret) {
            res = MP_UNEXPECTED_ERROR;
            break;
        }

        /* use regular expression */
        ret = regexec(&regex, serverAddress.c_str(), 0, NULL, 0);
        if (ret) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "Server info contains invalid URL: %s\n", serverAddress.c_str());
            res = MP_INVALID_PARAMETER;
            regfree(&regex);
            break;
        }

        /* free compiled regular expression */
        regfree(&regex);
#else
        /* use regular expression */
        if (!regex_match(serverAddress, regex, flag)) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "Server info contains invalid URL: %s\n", serverAddress.c_str());
            res = MP_INVALID_PARAMETER;
            break;
        }
#endif

        // Allocate buff
        buff = new uint8_t[sizeof(ConfigurationUEFI) + serverIdSize - sizeof(configurationUefi->headerId)];
        if (NULL == buff) {
            res = MP_MEM_ERROR;
            break;
        }
        configurationUefi = (ConfigurationUEFI*)buff;

        // zero all response uefi structure
        memset(configurationUefi, 0, sizeof(ConfigurationUEFI) + serverIdSize - sizeof(configurationUefi->headerId));

        configurationUefi->version = MP_BIOS_UEFI_VARIABLE_VERSION_1;
        configurationUefi->size = (uint16_t)(sizeof(configurationUefi->flags) + sizeof(configurationUefi->headerInfo) +
            +sizeof(configurationUefi->urlSize) + sizeof(configurationUefi->url) + serverIdSize);
        configurationUefi->flags = flags;
        memcpy(&(configurationUefi->headerInfo.guid), RegistrationServerInfo_GUID, GUID_SIZE);
        configurationUefi->headerInfo.version = MP_STRUCTURE_VERSION;
        configurationUefi->headerInfo.size = (uint16_t)((uint16_t)sizeof(configurationUefi->urlSize) +
            (uint16_t)sizeof(configurationUefi->url) + serverIdSize);
        configurationUefi->urlSize = (uint16_t)serverAddress.length();
        memcpy(&(configurationUefi->url), serverAddress.c_str(), configurationUefi->urlSize);

        // copy server ID to uefi structure
        memcpy(&(configurationUefi->headerId), serverId, serverIdSize);

#if MP_VERIFY_INTERNAL_DATA_STRUCT_WRITE == 1
        registrationServerID = (RegistrationServerID*)&configurationUefi->headerId;
        // structure version check
        if (configurationUefi->headerInfo.version != MP_STRUCTURE_VERSION) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "setRegistrationServerInfo: version check failed\n");
            res = MP_UEFI_INTERNAL_ERROR;
            break;
        }

        // verify registration server info structure
        if ((configurationUefi->headerInfo.size != (sizeof(RegistrationServerInfo) - sizeof(configurationUefi->headerInfo))) ||
            (0 != memcmp(configurationUefi->headerInfo.guid, RegistrationServerInfo_GUID, GUID_SIZE))) {

            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "setRegistrationServerInfo: registration server info structure is invalid.\n");
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "setRegistrationServerInfo: configurationUefi->headerInfo.size: %d, \
                expected size: %d\n", configurationUefi->headerInfo.size, sizeof(RegistrationServerInfo) - sizeof(configurationUefi->headerInfo));

            const uint8_t* actual = configurationUefi->headerInfo.guid;
            const uint8_t* expected = RegistrationServerInfo_GUID;
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "actual RegistrationServerInfo_GUID:\n");
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                actual[0], actual[1], actual[2], actual[3], actual[4], actual[5],
                actual[6], actual[7], actual[8], actual[9], actual[10], actual[11],
                actual[12], actual[13], actual[14], actual[15]);
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "expect RegistrationServerInfo_GUID:\n");
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                expected[0], expected[1], expected[2], expected[3], expected[4], expected[5],
                expected[6], expected[7], expected[8], expected[9], expected[10], expected[11],
                expected[12], expected[13], expected[14], expected[15]);

            res = MP_UEFI_INTERNAL_ERROR;
            break;
        }
 
        // structure version check
        if (registrationServerID->header.version != MP_STRUCTURE_VERSION) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "setRegistrationServerInfo: registrationServerID version check failed\n");
            res = MP_UEFI_INTERNAL_ERROR;
            break;
        }

        if ((registrationServerID->header.size != (sizeof(*registrationServerID) - sizeof(registrationServerID->header))) ||
            (0 != memcmp(registrationServerID->header.guid, RegistrationServerID_GUID, GUID_SIZE))) {

            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "setRegistrationServerInfo: registration server ID structure is invalid.\n");
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "setRegistrationServerInfo: registrationServerID->header.size: %d, sizeof(registrationServerID->): %d, sizeof(registrationServerID->header): %d\n",
                registrationServerID->header.size, sizeof(*registrationServerID), sizeof(registrationServerID->header));

            const uint8_t* actual = registrationServerID->header.guid;
            const uint8_t* expected = RegistrationServerID_GUID;
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "actual RegistrationServerID_GUID:\n");
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                actual[0], actual[1], actual[2], actual[3], actual[4], actual[5],
                actual[6], actual[7], actual[8], actual[9], actual[10], actual[11],
                actual[12], actual[13], actual[14], actual[15]);
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "expect RegistrationServerID_GUID:\n");
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX\n",
                expected[0], expected[1], expected[2], expected[3], expected[4], expected[5],
                expected[6], expected[7], expected[8], expected[9], expected[10], expected[11],
                expected[12], expected[13], expected[14], expected[15]);

            res = MP_UEFI_INTERNAL_ERROR;
            break;
        }
#endif

        // write registration configuration to uefi
        int numOfBytes = m_uefi->writeUEFIVar(UEFI_VAR_CONFIGURATION, (const uint8_t*)configurationUefi, sizeof(ConfigurationUEFI) + serverIdSize - 
            sizeof(configurationUefi->headerId), false);
        if (numOfBytes != (int)(sizeof(ConfigurationUEFI) + serverIdSize - sizeof(configurationUefi->headerId))) {
	    if(numOfBytes == -1) {
                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "setRegistrationServerInfo: Can't write Registration Configuration UEFI variable, please check whether the SGX has been disabled.\n");
                res = MP_INSUFFICIENT_PRIVILEGES;
	    } 
            else {
                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "setRegistrationServerInfo: failed to write uefi variable.\n");
                res = MP_UNEXPECTED_ERROR;
            }	    
            break;
        }
    } while (0);

    if (buff) {
        delete[] buff;
    }

    return res;
}


MPUefi::~MPUefi() {
    if (NULL != m_uefi) {
        delete m_uefi;
        m_uefi = NULL;
    }
}
