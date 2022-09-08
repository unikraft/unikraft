/*******************************************************************************
* Copyright 2002-2021 Intel Corporation
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
//     Digesting message according to SHA256
// 
//  Contents:
//     ippsSHA256GetSize()
//     ippsSHA256Init()
//     ippsSHA256Pack()
//     ippsSHA256Unpack()
//     ippsSHA256Duplicate()
//     ippsSHA256Update()
//     ippsSHA256GetTag()
//     ippsSHA256Final()
//     ippsSHA256MessageDigest()
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"

#if !defined(_PCP_SHA256_STUFF_H)
#define _PCP_SHA256_STUFF_H

/* SHA-256, SHA-224 constants */
static const Ipp32u sha256_iv[] = {
   0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
   0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19};
static const Ipp32u sha224_iv[] = {
   0xC1059ED8, 0x367CD507, 0x3070DD17, 0xF70E5939,
   0xFFC00B31, 0x68581511, 0x64F98FA7, 0xBEFA4FA4};

static __ALIGN16 const Ipp32u sha256_cnt[] = {
   0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
   0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
   0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
   0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
   0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
   0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
   0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
   0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
   0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
   0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
   0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
   0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
   0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
   0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
   0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
   0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
};


/* setup init hash value */
__INLINE void hashInit(Ipp32u* pHash, const Ipp32u* iv)
{
   pHash[0] = iv[0];
   pHash[1] = iv[1];
   pHash[2] = iv[2];
   pHash[3] = iv[3];
   pHash[4] = iv[4];
   pHash[5] = iv[5];
   pHash[6] = iv[6];
   pHash[7] = iv[7];
}
IPP_OWN_DEFN (static void, sha256_hashInit, (void* pHash))
{
   hashInit((Ipp32u*)pHash, sha256_iv);
}
IPP_OWN_DEFN (static void, sha224_hashInit, (void* pHash))
{
   hashInit((Ipp32u*)pHash, sha224_iv);
}

IPP_OWN_DEFN (static void, sha256_hashUpdate, (void* pHash, const Ipp8u* pMsg, int msgLen))
{
   UpdateSHA256(pHash, pMsg, msgLen, sha256_cnt);
}
#if (_SHA_NI_ENABLING_==_FEATURE_TICKTOCK_ || _SHA_NI_ENABLING_==_FEATURE_ON_)
IPP_OWN_DEFN (static void, sha256_ni_hashUpdate, (void* pHash, const Ipp8u* pMsg, int msgLen))
{
   UpdateSHA256ni(pHash, pMsg, msgLen, sha256_cnt);
}
#endif

/* convert hash into big endian */
IPP_OWN_DEFN (static void, sha256_hashOctString, (Ipp8u* pMD, void* pHashVal))
{
   /* convert hash into big endian */
   ((Ipp32u*)pMD)[0] = ENDIANNESS32(((Ipp32u*)pHashVal)[0]);
   ((Ipp32u*)pMD)[1] = ENDIANNESS32(((Ipp32u*)pHashVal)[1]);
   ((Ipp32u*)pMD)[2] = ENDIANNESS32(((Ipp32u*)pHashVal)[2]);
   ((Ipp32u*)pMD)[3] = ENDIANNESS32(((Ipp32u*)pHashVal)[3]);
   ((Ipp32u*)pMD)[4] = ENDIANNESS32(((Ipp32u*)pHashVal)[4]);
   ((Ipp32u*)pMD)[5] = ENDIANNESS32(((Ipp32u*)pHashVal)[5]);
   ((Ipp32u*)pMD)[6] = ENDIANNESS32(((Ipp32u*)pHashVal)[6]);
   ((Ipp32u*)pMD)[7] = ENDIANNESS32(((Ipp32u*)pHashVal)[7]);
}
IPP_OWN_DEFN (static void, sha224_hashOctString, (Ipp8u* pMD, void* pHashVal))
{
   /* convert hash into big endian */
   ((Ipp32u*)pMD)[0] = ENDIANNESS32(((Ipp32u*)pHashVal)[0]);
   ((Ipp32u*)pMD)[1] = ENDIANNESS32(((Ipp32u*)pHashVal)[1]);
   ((Ipp32u*)pMD)[2] = ENDIANNESS32(((Ipp32u*)pHashVal)[2]);
   ((Ipp32u*)pMD)[3] = ENDIANNESS32(((Ipp32u*)pHashVal)[3]);
   ((Ipp32u*)pMD)[4] = ENDIANNESS32(((Ipp32u*)pHashVal)[4]);
   ((Ipp32u*)pMD)[5] = ENDIANNESS32(((Ipp32u*)pHashVal)[5]);
   ((Ipp32u*)pMD)[6] = ENDIANNESS32(((Ipp32u*)pHashVal)[6]);
}

IPP_OWN_DEFN (static void, sha256_msgRep, (Ipp8u* pDst, Ipp64u lenLo, Ipp64u lenHi))
{
   IPP_UNREFERENCED_PARAMETER(lenHi);
   lenLo = ENDIANNESS64(lenLo<<3);
   ((Ipp64u*)(pDst))[0] = lenLo;
}

/*
// SHA256 init context
*/
IPP_OWN_DEFN (static IppStatus, GetSizeSHA256, (int* pSize))
{
   IPP_BAD_PTR1_RET(pSize);
   *pSize = sizeof(IppsSHA256State);
   return ippStsNoErr;
}

IPP_OWN_DEFN (static IppStatus, InitSHA256, (IppsSHA256State* pState, const DigestSHA256 IV))
{
   /* test state pointer */
   IPP_BAD_PTR1_RET(pState);

   HASH_SET_ID(pState, idCtxSHA256);
   HASH_LENLO(pState) = 0;
   HAHS_BUFFIDX(pState) = 0;

   /* setup initial digest */
   HASH_VALUE(pState)[0] = IV[0];
   HASH_VALUE(pState)[1] = IV[1];
   HASH_VALUE(pState)[2] = IV[2];
   HASH_VALUE(pState)[3] = IV[3];
   HASH_VALUE(pState)[4] = IV[4];
   HASH_VALUE(pState)[5] = IV[5];
   HASH_VALUE(pState)[6] = IV[6];
   HASH_VALUE(pState)[7] = IV[7];

   return ippStsNoErr;
}

#define cpSHA256MessageDigest OWNAPI(cpSHA256MessageDigest)
   IPP_OWN_DECL (IppStatus, cpSHA256MessageDigest, (DigestSHA256 hash, const Ipp8u* pMsg, int msgLen, const DigestSHA256 IV))
#define cpFinalizeSHA256 OWNAPI(cpFinalizeSHA256)
   IPP_OWN_DECL (void, cpFinalizeSHA256, (DigestSHA256 pHash, const Ipp8u* inpBuffer, int inpLen, Ipp64u processedMsgLen))

#endif /* #if !defined(_PCP_SHA256_STUFF_H) */
