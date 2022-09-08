/*******************************************************************************
* Copyright 2003-2021 Intel Corporation
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
//     Internal ECC (prime) basic Definitions & Function Prototypes
// 
// 
*/

#if !defined(_NEW_PCP_ECCP_H)
#define _NEW_PCP_ECCP_H

#include "pcpgfpecstuff.h"


__INLINE IppsBigNumState* cpConstructBN(IppsBigNumState* pBN, cpSize len, BNU_CHUNK_T* pData, BNU_CHUNK_T* pBuffer)
{
   BN_SET_ID(pBN);
   BN_SIGN(pBN) = ippBigNumPOS;
   BN_SIZE(pBN) = len;
   BN_ROOM(pBN) = len;
   BN_NUMBER(pBN) = pData;
   BN_BUFFER(pBN) = pBuffer;
   return pBN;
}

/* set EC parameters */
#define ECCPSetDP OWNAPI(ECCPSetDP)
   IPP_OWN_DECL (IppStatus, ECCPSetDP, (const IppsGFpMethod* method, int pLen, const BNU_CHUNK_T* pP, int aLen, const BNU_CHUNK_T* pA, int bLen, const BNU_CHUNK_T* pB, int xLen, const BNU_CHUNK_T* pX, int yLen, const BNU_CHUNK_T* pY, int rLen, const BNU_CHUNK_T* pR, BNU_CHUNK_T h, IppsGFpECState* pEC))

/*
// Recommended (Standard) Domain Parameters
*/
extern const BNU_CHUNK_T secp112r1_p[]; // (2^128 -3)/76439
extern const BNU_CHUNK_T secp112r1_a[];
extern const BNU_CHUNK_T secp112r1_b[];
extern const BNU_CHUNK_T secp112r1_gx[];
extern const BNU_CHUNK_T secp112r1_gy[];
extern const BNU_CHUNK_T secp112r1_r[];
extern       BNU_CHUNK_T secp112r1_h;

extern const BNU_CHUNK_T secp112r2_p[]; // (2^128 -3)/76439
extern const BNU_CHUNK_T secp112r2_a[];
extern const BNU_CHUNK_T secp112r2_b[];
extern const BNU_CHUNK_T secp112r2_gx[];
extern const BNU_CHUNK_T secp112r2_gy[];
extern const BNU_CHUNK_T secp112r2_r[];
extern       BNU_CHUNK_T secp112r2_h;

extern const BNU_CHUNK_T secp128r1_p[]; // 2^128 -2^97 -1
extern const BNU_CHUNK_T secp128r1_a[];
extern const BNU_CHUNK_T secp128r1_b[];
extern const BNU_CHUNK_T secp128r1_gx[];
extern const BNU_CHUNK_T secp128r1_gy[];
extern const BNU_CHUNK_T secp128r1_r[];
extern       BNU_CHUNK_T secp128r1_h;

extern const BNU_CHUNK_T* secp128_mx[];

extern const BNU_CHUNK_T secp128r2_p[]; // 2^128 -2^97 -1
extern const BNU_CHUNK_T secp128r2_a[];
extern const BNU_CHUNK_T secp128r2_b[];
extern const BNU_CHUNK_T secp128r2_gx[];
extern const BNU_CHUNK_T secp128r2_gy[];
extern const BNU_CHUNK_T secp128r2_r[];
extern       BNU_CHUNK_T secp128r2_h;

extern const BNU_CHUNK_T secp160r1_p[]; // 2^160 -2^31 -1
extern const BNU_CHUNK_T secp160r1_a[];
extern const BNU_CHUNK_T secp160r1_b[];
extern const BNU_CHUNK_T secp160r1_gx[];
extern const BNU_CHUNK_T secp160r1_gy[];
extern const BNU_CHUNK_T secp160r1_r[];
extern       BNU_CHUNK_T secp160r1_h;

extern const BNU_CHUNK_T secp160r2_p[]; // 2^160 -2^32 -2^14 -2^12 -2^9 -2^8 -2^7 -2^2 -1
extern const BNU_CHUNK_T secp160r2_a[];
extern const BNU_CHUNK_T secp160r2_b[];
extern const BNU_CHUNK_T secp160r2_gx[];
extern const BNU_CHUNK_T secp160r2_gy[];
extern const BNU_CHUNK_T secp160r2_r[];
extern       BNU_CHUNK_T secp160r2_h;

extern const BNU_CHUNK_T secp192r1_p[]; // 2^192 -2^64 -1
extern const BNU_CHUNK_T secp192r1_a[];
extern const BNU_CHUNK_T secp192r1_b[];
extern const BNU_CHUNK_T secp192r1_gx[];
extern const BNU_CHUNK_T secp192r1_gy[];
extern const BNU_CHUNK_T secp192r1_r[];
extern       BNU_CHUNK_T secp192r1_h;

extern const BNU_CHUNK_T secp224r1_p[]; // 2^224 -2^96 +1
extern const BNU_CHUNK_T secp224r1_a[];
extern const BNU_CHUNK_T secp224r1_b[];
extern const BNU_CHUNK_T secp224r1_gx[];
extern const BNU_CHUNK_T secp224r1_gy[];
extern const BNU_CHUNK_T secp224r1_r[];
extern       BNU_CHUNK_T secp224r1_h;

extern const BNU_CHUNK_T secp256r1_p[]; // 2^256 -2^224 +2^192 +2^96 -1
extern const BNU_CHUNK_T secp256r1_a[];
extern const BNU_CHUNK_T secp256r1_b[];
extern const BNU_CHUNK_T secp256r1_gx[];
extern const BNU_CHUNK_T secp256r1_gy[];
extern const BNU_CHUNK_T secp256r1_r[];
extern       BNU_CHUNK_T secp256r1_h;

extern const BNU_CHUNK_T secp384r1_p[]; // 2^384 -2^128 -2^96 +2^32 -1
extern const BNU_CHUNK_T secp384r1_a[];
extern const BNU_CHUNK_T secp384r1_b[];
extern const BNU_CHUNK_T secp384r1_gx[];
extern const BNU_CHUNK_T secp384r1_gy[];
extern const BNU_CHUNK_T secp384r1_r[];
extern       BNU_CHUNK_T secp384r1_h;

extern const BNU_CHUNK_T secp521r1_p[]; // 2^521 -1
extern const BNU_CHUNK_T secp521r1_a[];
extern const BNU_CHUNK_T secp521r1_b[];
extern const BNU_CHUNK_T secp521r1_gx[];
extern const BNU_CHUNK_T secp521r1_gy[];
extern const BNU_CHUNK_T secp521r1_r[];
extern       BNU_CHUNK_T secp521r1_h;

extern const BNU_CHUNK_T tpmBN_p256p_p[]; // TPM BN_P256
extern const BNU_CHUNK_T tpmBN_p256p_a[];
extern const BNU_CHUNK_T tpmBN_p256p_b[];
extern const BNU_CHUNK_T tpmBN_p256p_gx[];
extern const BNU_CHUNK_T tpmBN_p256p_gy[];
extern const BNU_CHUNK_T tpmBN_p256p_r[];
extern       BNU_CHUNK_T tpmBN_p256p_h;

extern const BNU_CHUNK_T tpmSM2_p256_p[]; // TPM SM2_P256
extern const BNU_CHUNK_T tpmSM2_p256_a[];
extern const BNU_CHUNK_T tpmSM2_p256_b[];
extern const BNU_CHUNK_T tpmSM2_p256_gx[];
extern const BNU_CHUNK_T tpmSM2_p256_gy[];
extern const BNU_CHUNK_T tpmSM2_p256_r[];
extern       BNU_CHUNK_T tpmSM2_p256_h;

extern const BNU_CHUNK_T* tpmSM2_p256_p_mx[];

/* half of some std  modulus */
extern const BNU_CHUNK_T h_secp128r1_p[];
extern const BNU_CHUNK_T h_secp192r1_p[];
extern const BNU_CHUNK_T h_secp224r1_p[];
extern const BNU_CHUNK_T h_secp256r1_p[];
extern const BNU_CHUNK_T h_secp384r1_p[];
extern const BNU_CHUNK_T h_secp521r1_p[];
extern const BNU_CHUNK_T h_tpmSM2_p256_p[];

__INLINE BNU_CHUNK_T* cpModAdd_BNU(BNU_CHUNK_T* pR,
                             const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB,
                             const BNU_CHUNK_T* pM, int ns,
                                   BNU_CHUNK_T* pBuffer)
{
   BNU_CHUNK_T e = cpAdd_BNU(pR, pA, pB, ns);
   e -= cpSub_BNU(pBuffer, pR, pM, ns);
   MASKED_COPY_BNU(pR, e, pR, pBuffer, ns);
   return pR;
}

__INLINE BNU_CHUNK_T* cpModSub_BNU(BNU_CHUNK_T* pR,
                             const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB,
                             const BNU_CHUNK_T* pM, int ns,
                                   BNU_CHUNK_T* pBuffer)
{
   BNU_CHUNK_T e = cpSub_BNU(pR, pA, pB, ns);
   cpAdd_BNU(pBuffer, pR, pM, ns);
   MASKED_COPY_BNU(pR, (0-e), pBuffer, pR, ns);
   return pR;
}

#endif /* _NEW_PCP_ECCP_H */
