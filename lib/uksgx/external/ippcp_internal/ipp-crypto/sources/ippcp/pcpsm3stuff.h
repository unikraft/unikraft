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
//     Digesting message according to SM3
// 
//  Contents:
//     SM3 methods and constants
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"

#if !defined _PCP_SM3_STUFF_H
#define _PCP_SM3_STUFF_H

/* SM3 constants */
static const Ipp32u sm3_iv[] = {
   0x7380166F, 0x4914B2B9, 0x172442D7, 0xDA8A0600,
   0xA96F30BC, 0x163138AA, 0xE38DEE4D, 0xB0FB0E4E};

static __ALIGN16 const Ipp32u sm3_cnt[] = {
   0x79CC4519,0xF3988A32,0xE7311465,0xCE6228CB,0x9CC45197,0x3988A32F,0x7311465E,0xE6228CBC,
   0xCC451979,0x988A32F3,0x311465E7,0x6228CBCE,0xC451979C,0x88A32F39,0x11465E73,0x228CBCE6,
   0x9D8A7A87,0x3B14F50F,0x7629EA1E,0xEC53D43C,0xD8A7A879,0xB14F50F3,0x629EA1E7,0xC53D43CE,
   0x8A7A879D,0x14F50F3B,0x29EA1E76,0x53D43CEC,0xA7A879D8,0x4F50F3B1,0x9EA1E762,0x3D43CEC5,
   0x7A879D8A,0xF50F3B14,0xEA1E7629,0xD43CEC53,0xA879D8A7,0x50F3B14F,0xA1E7629E,0x43CEC53D,
   0x879D8A7A,0x0F3B14F5,0x1E7629EA,0x3CEC53D4,0x79D8A7A8,0xF3B14F50,0xE7629EA1,0xCEC53D43,
   0x9D8A7A87,0x3B14F50F,0x7629EA1E,0xEC53D43C,0xD8A7A879,0xB14F50F3,0x629EA1E7,0xC53D43CE,
   0x8A7A879D,0x14F50F3B,0x29EA1E76,0x53D43CEC,0xA7A879D8,0x4F50F3B1,0x9EA1E762,0x3D43CEC5
};

IPP_OWN_DEFN (static void, sm3_hashInit, (void* pHash))
{
   /* setup initial digest */
   ((Ipp32u*)pHash)[0] = sm3_iv[0];
   ((Ipp32u*)pHash)[1] = sm3_iv[1];
   ((Ipp32u*)pHash)[2] = sm3_iv[2];
   ((Ipp32u*)pHash)[3] = sm3_iv[3];
   ((Ipp32u*)pHash)[4] = sm3_iv[4];
   ((Ipp32u*)pHash)[5] = sm3_iv[5];
   ((Ipp32u*)pHash)[6] = sm3_iv[6];
   ((Ipp32u*)pHash)[7] = sm3_iv[7];
}

IPP_OWN_DEFN (static void, sm3_hashUpdate, (void* pHash, const Ipp8u* pMsg, int msgLen))
{
   UpdateSM3(pHash, pMsg, msgLen, sm3_cnt);
}

IPP_OWN_DEFN (static void, sm3_hashOctString, (Ipp8u* pMD, void* pHashVal))
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

IPP_OWN_DEFN (static void, sm3_msgRep, (Ipp8u* pDst, Ipp64u lenLo, Ipp64u lenHi))
{
   IPP_UNREFERENCED_PARAMETER(lenHi);
   lenLo = ENDIANNESS64(lenLo<<3);
   ((Ipp64u*)(pDst))[0] = lenLo;
}

#define cpFinalizeSM3 OWNAPI(cpFinalizeSM3)
   IPP_OWN_DECL (void, cpFinalizeSM3, (DigestSHA1 pHash, const Ipp8u* inpBuffer, int inpLen, Ipp64u processedMsgLen))

#endif /* #if !defined _PCP_SM3_STUFF_H */
