/*******************************************************************************
* Copyright 2013-2021 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

/*
//
//  Purpose:
//     Cryptography Primitive.
//     RSASSA-PKCS-v1_5
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpngrsa.h"
#include "pcphash.h"
#include "pcptool.h"

/*
// The DER encoding T of the DigestInfo value is equal to the following (see PKCS-1v2-2):
*/
static const Ipp8u   SHA1_fixPS[] = "\x30\x21\x30\x09\x06\x05\x2b\x0e\x03\x02\x1a\x05\x00\x04\x14";
static const Ipp8u SHA224_fixPS[] = "\x30\x2d\x30\x0d\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x04\x05\x00\x04\x1c";
static const Ipp8u SHA256_fixPS[] = "\x30\x31\x30\x0d\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x01\x05\x00\x04\x20";
static const Ipp8u SHA384_fixPS[] = "\x30\x41\x30\x0d\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x02\x05\x00\x04\x30";
static const Ipp8u SHA512_fixPS[] = "\x30\x51\x30\x0d\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x03\x05\x00\x04\x40";
static const Ipp8u    MD5_fixPS[] = "\x30\x20\x30\x0c\x06\x08\x2a\x86\x48\x86\xf7\x0d\x02\x05\x05\x00\x04\x10";
static const Ipp8u SHA512_224_fixPS[] = "\x30\x2d\x30\x0d\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x05\x05\x00\x04\x1c";
static const Ipp8u SHA512_256_fixPS[] = "\x30\x31\x30\x0d\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x06\x05\x00\x04\x20";

typedef struct {
    const Ipp8u* pSalt;
    int saltLen;
} SaltInfo;

static SaltInfo pksc15_salt[ippHashAlg_MaxNo] = {
    { NULL,         0 },
    { SHA1_fixPS,   sizeof(SHA1_fixPS) - 1 },
    { SHA256_fixPS, sizeof(SHA256_fixPS) - 1 },
    { SHA224_fixPS, sizeof(SHA224_fixPS) - 1 },
    { SHA512_fixPS, sizeof(SHA512_fixPS) - 1 },
    { SHA384_fixPS, sizeof(SHA384_fixPS) - 1 },
    { MD5_fixPS, sizeof(MD5_fixPS) - 1 },
    { NULL,         0 },
    { SHA512_224_fixPS, sizeof(SHA512_224_fixPS) - 1 },
    { SHA512_256_fixPS, sizeof(SHA512_256_fixPS) - 1 },
};
