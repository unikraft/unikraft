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
//     Digesting message according to MD5
//     (derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm)
// 
//     Equivalent code is available from RFC 1321.
// 
//  Contents:
//    MD4 methods and constants
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"

#if !defined(_PCP_MD5_STUFF_H)
#define _PCP_MD5_STUFF_H

/* MD5 constants */
static const Ipp32u md5_iv[] = {
   0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476};

static __ALIGN16 const Ipp32u md5_cnt[] = {
   0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE,
   0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501,
   0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE,
   0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821,

   0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA,
   0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8,
   0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED,
   0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A,

   0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C,
   0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
   0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05,
   0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665,

   0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039,
   0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
   0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1,
   0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391
};

IPP_OWN_DEFN (static void, md5_hashInit, (void* pHash))
{
   /* setup initial digest */
   ((Ipp32u*)pHash)[0] = md5_iv[0];
   ((Ipp32u*)pHash)[1] = md5_iv[1];
   ((Ipp32u*)pHash)[2] = md5_iv[2];
   ((Ipp32u*)pHash)[3] = md5_iv[3];
}

IPP_OWN_DEFN (static void, md5_hashUpdate, (void* pHash, const Ipp8u* pMsg, int msgLen))
{
   UpdateMD5(pHash, pMsg, msgLen, md5_cnt);
}

IPP_OWN_DEFN (static void, md5_hashOctString, (Ipp8u* pMD, void* pHashVal))
{
   /* md5 does not need conversion into big endian */
   ((Ipp32u*)pMD)[0] = ((Ipp32u*)pHashVal)[0];
   ((Ipp32u*)pMD)[1] = ((Ipp32u*)pHashVal)[1];
   ((Ipp32u*)pMD)[2] = ((Ipp32u*)pHashVal)[2];
   ((Ipp32u*)pMD)[3] = ((Ipp32u*)pHashVal)[3];
}

IPP_OWN_DEFN (static void, md5_msgRep, (Ipp8u* pDst, Ipp64u lenLo, Ipp64u lenHi))
{
   IPP_UNREFERENCED_PARAMETER(lenHi);
   lenLo <<= 3;
   ((Ipp64u*)(pDst))[0] = lenLo;
}

#define cpFinalizeMD5 OWNAPI(cpFinalizeMD5)
   IPP_OWN_DECL (void, cpFinalizeMD5, (DigestMD5 pHash, const Ipp8u* inpBuffer, int inpLen, Ipp64u processedMsgLen))

#endif /* #if !defined(_PCP_MD5_STUFF_H) */
