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
#ifndef _EPID_RL_H_
#define _EPID_RL_H_

#include <stdint.h>
#pragma pack(1)

typedef struct _EPID_SIG_RL
{
    uint8_t     sver[2];        // sigma 1.1 -- 00 01 sigma 2.0 -- 00 02
    uint8_t     blob_id[2];     // 00 0E
    uint8_t     group_id[4];    // big-endian
    uint8_t     rl_version[4];  // big-endian
    uint8_t     entries[4];     // big-endian
    uint8_t     entry[0];       // each entry is 128 bytes (EPID_SIG_RL_ENTRY_SIZE)
//    uint8_t     ECDSA1[32];   // big-endian (follows entry array)
//    uint8_t     ECDSA2[32];   // big-endian
}EPID_SIG_RL;

typedef struct _EPID_PRIV_RL
{
    uint8_t   sver[2];          // sigma 1.1 -- 00 01 sigma 2.0 -- 00 02
    uint8_t   blob_id[2];       // 00 0D
    uint8_t   group_id[4];      // big-endian
    uint8_t   rl_version[4];    // big-endian
    uint8_t   entries[4];       // big-endian
    uint8_t   entry[0];         // each entry is 32 bytes (EPID_PRIV_RL_ENTRY_SIZE)
//  uint8_t   ECDSA1[32];       // big-endian (follows entry array)
//  uint8_t   ECDSA2[32];       // big-endian
}EPID_PRIV_RL;

#pragma pack()

#pragma pack()
// EPID 1.1 and EPID 2.0 share same entry and signature size
#define EPID_SIG_RL_ENTRY_SIZE       128
#define EPID_SIG_RL_SIGNATURE_SIZE    64
#define EPID_PRIV_RL_ENTRY_SIZE       32
#define EPID_PRIV_RL_SIGNATURE_SIZE   64
#endif
