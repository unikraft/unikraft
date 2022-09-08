/*******************************************************************************
* Copyright 2017-2021 Intel Corporation
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
//     Fixed EC primes
// 
// 
*/

#if !defined(_PCP_ECPRIME_H)
#define _PCP_ECPRIME_H

#include "owndefs.h"
#include "pcpbnuimpl.h"


/*
// Recommended (NIST's) underlying EC Primes
*/
extern const BNU_CHUNK_T secp112r1_p[]; // (2^128 -3)/76439
extern const BNU_CHUNK_T secp112r2_p[]; // (2^128 -3)/76439
extern const BNU_CHUNK_T secp128r1_p[]; // 2^128 -2^97 -1
extern const BNU_CHUNK_T secp128r2_p[]; // 2^128 -2^97 -1
extern const BNU_CHUNK_T secp160r1_p[]; // 2^160 -2^31 -1
extern const BNU_CHUNK_T secp160r2_p[]; // 2^160 -2^32 -2^14 -2^12 -2^9 -2^8 -2^7 -2^2 -1
extern const BNU_CHUNK_T secp192r1_p[]; // 2^192 -2^64 -1
extern const BNU_CHUNK_T secp224r1_p[]; // 2^224 -2^96 +1
extern const BNU_CHUNK_T secp256r1_p[]; // 2^256 -2^224 +2^192 +2^96 -1
extern const BNU_CHUNK_T secp384r1_p[]; // 2^384 -2^128 -2^96 +2^32 -1
extern const BNU_CHUNK_T secp521r1_p[]; // 2^521 -1

extern const BNU_CHUNK_T tpmBN_p256p_p[]; // TPM BN_P256

/*
// Recommended (SM2) underlying EC Prime
*/
extern const BNU_CHUNK_T tpmSM2_p256_p[]; // TPM SM2_P256

#endif /* _PCP_ECPRIME_H */
