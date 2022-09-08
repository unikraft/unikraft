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

#ifndef _REF_LE_H_
#define _REF_LE_H_

#include "aeerror.h"
#include "arch.h"
#include "sgx_tcrypto.h"


// calculate the white list size
#define REF_LE_WL_SIZE(entries)         ((uint32_t)(sizeof(ref_le_white_list_t) + entries * sizeof(ref_le_white_list_entry_t)))

/* All fields are big endian */

#pragma pack(push, 1)
typedef struct _ref_le_white_list_entry_t
{
    uint8_t                     provision_key;
    uint8_t                     match_mr_enclave;
    uint8_t                     reserved[6]; // align the measurment on 64 bits
    sgx_measurement_t           mr_signer;
    sgx_measurement_t           mr_enclave;
} ref_le_white_list_entry_t;

typedef struct _ref_le_white_list_t
{
    uint16_t                    version;
    uint32_t                    wl_version;
    uint16_t                    entries_count;
    sgx_rsa3072_public_key_t    signer_pubkey;
    ref_le_white_list_entry_t   wl_entries[];
} ref_le_white_list_t;
#pragma pack(pop)

#endif
