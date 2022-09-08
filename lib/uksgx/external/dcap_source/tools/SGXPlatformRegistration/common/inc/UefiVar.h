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
 * File: UefiVar.h
 *  
 * Description: Definitions of the BIOS UEFI Variable data and structures.
 */
#ifndef __UEFI_VAR_H
#define __UEFI_VAR_H

#include <cstdint>

#define EFIVARS_CONFIGURATION_GUID   "-18b3bc81-e210-42b9-9ec8-2c5a7d4d89b6"
#define EFIVARS_SERVER_REQUEST_GUID  "-304e0796-d515-4698-ac6e-e76cb1a71c28"
#define EFIVARS_SERVER_RESPONSE_GUID "-89589c7b-b2d9-4fc9-bcda-463b983b2fb7"
#define EFIVARS_PACKAGE_INFO_GUID    "-ac406deb-ab92-42d6-aff7-0d78e0826c68"
#define EFIVARS_STATUS_GUID          "-f236c5dc-a491-4bbe-bcdd-88885770df45"

#define EFIVARS_EPCBIOS_GUID         "-c60aa7f6-e8d6-4956-8ba1-fe26298f5e87"
#define EFIVARS_EPCSW_GUID           "-d69a279b-58eb-45d1-a148-771bb9eb5251"
#define EFIVARS_SOFTWAREGUARDSTATUS_GUID    "-9cb2e73f-7325-40f4-a484-659bb344c3cd"

#define UEFI_VAR_CONFIGURATION   "SgxRegistrationConfiguration"  EFIVARS_CONFIGURATION_GUID
#define UEFI_VAR_SERVER_REQUEST  "SgxRegistrationServerRequest"  EFIVARS_SERVER_REQUEST_GUID
#define UEFI_VAR_SERVER_RESPONSE "SgxRegistrationServerResponse" EFIVARS_SERVER_RESPONSE_GUID
#define UEFI_VAR_PACKAGE_INFO    "SgxRegistrationPackageInfo"    EFIVARS_PACKAGE_INFO_GUID
#define UEFI_VAR_STATUS          "SgxRegistrationStatus"         EFIVARS_STATUS_GUID
#define UEFI_VAR_EPCBIOS         "EPCBIOS"                       EFIVARS_EPCBIOS_GUID
#define UEFI_VAR_EPCSW	         "EPCSW"                         EFIVARS_EPCSW_GUID
#define UEFI_VAR_SOFTWAREGUARDSTATUS "SOFTWAREGUARDSTATUS"       EFIVARS_SOFTWAREGUARDSTATUS_GUID

#define MAX_URL_SIZE                        256
#define GUID_SIZE                           16
#define MP_BIOS_UEFI_VARIABLE_VERSION_1     1
#define MP_BIOS_UEFI_VARIABLE_VERSION_2     2
#define MP_STRUCTURE_VERSION                1
#define SERVER_INFO_FLAG_RS_NOT_SAVE_KEYS   0x0001

const uint8_t RegistrationServerID_GUID[GUID_SIZE] = { 0x31,0xA1,0x2A,0xFE,0x07,0x20,0x4E,0xBC,0xB6,0x4E,0xC4,0xB3,0xC7,0xF8,0xBC,0x0F };
const uint8_t RegistrationServerInfo_GUID[GUID_SIZE] = { 0x21,0x2F,0xE1,0x83,0x6B,0x1A,0x42,0xA1,0xA7,0xA9,0xDA,0x3A,0xB6,0xB7,0xBD,0x02 };
const uint8_t PlatformInfo_GUID[GUID_SIZE] = { 0x84,0x94,0x7A,0xC6,0x84,0x40,0x41,0x89,0x90,0x2A,0x7E,0x76,0xCD,0x65,0x89,0x26 };
const uint8_t EncryptedPlatformKey_GUID[GUID_SIZE] = { 0xFD,0x8F,0x5C,0x41,0x1B,0x61,0x4B,0x97,0xA7,0x47,0x96,0xF0,0x89,0x26,0x75,0x7B };
const uint8_t PairingReceipt_GUID[GUID_SIZE] = { 0xB4,0x0B,0xC4,0x67,0x1A,0xB5,0x40,0x66,0xB7,0xF9,0x60,0xB6,0x50,0x4B,0xC1,0x8B };
const uint8_t PlatformManifest_GUID[GUID_SIZE] = { 0x17,0x8E,0x87,0x4B,0x49,0xE4,0x4A,0xA5,0x99,0xBB,0x30,0x57,0x17,0x09,0x25,0xB4 };
const uint8_t KeyBlob_GUID[GUID_SIZE] = { 0x2E,0xCF,0x43,0xFD,0x61,0x4E,0x4F,0x94,0x98,0x2C,0xDF,0x36,0x10,0xF4,0x3A,0x9D };
const uint8_t PlatformMemberShip_GUID[GUID_SIZE] = { 0xEB,0xBD,0x00,0xC0,0xEF,0x90,0x49,0x10,0xA4,0x80,0xF7,0x72,0xE7,0x35,0x5D,0xB4 };
const uint8_t AddRequest_GUID[GUID_SIZE] = { 0x69,0x65,0x19,0xca,0x73,0xc1,0x47,0x85,0xa0,0xf6,0x4d,0x28,0x9d,0x37,0xe9,0x95 };


#pragma pack(push, 1)

typedef struct _StructureHeader
{
    uint8_t guid[GUID_SIZE];      /// Structure GUID value
    uint16_t size;               /// Size of complete structure minus the size of this header.
    uint16_t version;            /// Version of complete structure.  Currenlty, must be 1 for all structures. 
    uint8_t reserved[12];  /// Must be Zero
}StructureHeader;

typedef struct _SgxUefiVar {
    uint16_t version;
    uint16_t size;
    StructureHeader header;
} SgxUefiVar;

typedef struct _ConfigurationUEFI {
    uint16_t version;
    uint16_t size;
    uint16_t flags;
    StructureHeader headerInfo;      /// Unique Identifier for this data structure type.
    uint16_t urlSize;            /// Length of the Registration Server URL, rounded up to the nearest multiple of 16
    uint8_t url[MAX_URL_SIZE];   /// Registration server URL in ASCII representation. It does not contain a '\0\ null termination byte.
                                 /// any data beyond url[urlSize-1] is undefined.
    StructureHeader headerId;  /// PlatformInstanceID header
} ConfigurationUEFI;

typedef struct _RegistrationStatusUEFI {
    uint16_t version;
    uint16_t size;
    union {
        uint16_t status;
        struct {
            uint16_t registrationStatus:1;
            uint16_t packageInfoStatus:1;
            uint16_t reservedStatus:14;
        };
    };
    uint8_t errorCode;
} RegistrationStatusUEFI;

typedef struct _EpcBiosConfigUEFI {
	uint32_t supportedPrmBins; ///< supported EPC bins; Bit 0- Support for 1 MB, 1- Support for 2 MB, 2- Support for 4 MB,
	uint32_t maxEpcSize;       ///< Maximum EPC size supported by platform in MB
	uint32_t epcSize;          ///< Allocated EPC size in MB
	uint32_t epcMap[32];      ///< Mapping of PRM size to corresponding EPC size
} EpcBiosConfigUEFI;

typedef struct _EpcOsConfigUEFI {
	uint32_t epcSize;           ///< Requested EPC size in MB
} EpcOsConfigUEFI;

typedef struct _SoftwareGuardStatusUEFI {
	uint8_t sgxStatus;
    uint8_t reserved;
} SoftwareGuardStatusUEFI;

#pragma pack(pop)
#endif  // #ifndef __UEFI_VAR_