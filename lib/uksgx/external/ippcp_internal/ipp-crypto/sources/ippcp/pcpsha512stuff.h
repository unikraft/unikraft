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
//     SHA512 message digest
// 
//  Contents:
//     SHA512 stuff
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"

#if !defined(_PCP_SHA512_STUFF_H)
#define _PCP_SHA512_STUFF_H

/* SHA-512, SHA-384, SHA512-224, SHA512 constants */
static const Ipp64u sha512_iv[] = {
   CONST_64(0x6A09E667F3BCC908), CONST_64(0xBB67AE8584CAA73B),
   CONST_64(0x3C6EF372FE94F82B), CONST_64(0xA54FF53A5F1D36F1),
   CONST_64(0x510E527FADE682D1), CONST_64(0x9B05688C2B3E6C1F),
   CONST_64(0x1F83D9ABFB41BD6B), CONST_64(0x5BE0CD19137E2179)};
static const Ipp64u sha512_384_iv[] = {
   CONST_64(0xCBBB9D5DC1059ED8), CONST_64(0x629A292A367CD507),
   CONST_64(0x9159015A3070DD17), CONST_64(0x152FECD8F70E5939),
   CONST_64(0x67332667FFC00B31), CONST_64(0x8EB44A8768581511),
   CONST_64(0xDB0C2E0D64F98FA7), CONST_64(0x47B5481DBEFA4FA4)};
static const Ipp64u sha512_256_iv[] = {
   CONST_64(0x22312194FC2BF72C), CONST_64(0x9F555FA3C84C64C2),
   CONST_64(0x2393B86B6F53B151), CONST_64(0x963877195940EABD),
   CONST_64(0x96283EE2A88EFFE3), CONST_64(0xBE5E1E2553863992),
   CONST_64(0x2B0199FC2C85B8AA), CONST_64(0x0EB72DDC81C52CA2)};
static const Ipp64u sha512_224_iv[] = {
   CONST_64(0x8C3D37C819544DA2), CONST_64(0x73E1996689DCD4D6),
   CONST_64(0x1DFAB7AE32FF9C82), CONST_64(0x679DD514582F9FCF),
   CONST_64(0x0F6D2B697BD44DA8), CONST_64(0x77E36F7304C48942),
   CONST_64(0x3F9D85A86A1D36C8), CONST_64(0x1112E6AD91D692A1)};

static __ALIGN16 const Ipp64u sha512_cnt[] = {
   CONST_64(0x428A2F98D728AE22), CONST_64(0x7137449123EF65CD), CONST_64(0xB5C0FBCFEC4D3B2F), CONST_64(0xE9B5DBA58189DBBC),
   CONST_64(0x3956C25BF348B538), CONST_64(0x59F111F1B605D019), CONST_64(0x923F82A4AF194F9B), CONST_64(0xAB1C5ED5DA6D8118),
   CONST_64(0xD807AA98A3030242), CONST_64(0x12835B0145706FBE), CONST_64(0x243185BE4EE4B28C), CONST_64(0x550C7DC3D5FFB4E2),
   CONST_64(0x72BE5D74F27B896F), CONST_64(0x80DEB1FE3B1696B1), CONST_64(0x9BDC06A725C71235), CONST_64(0xC19BF174CF692694),
   CONST_64(0xE49B69C19EF14AD2), CONST_64(0xEFBE4786384F25E3), CONST_64(0x0FC19DC68B8CD5B5), CONST_64(0x240CA1CC77AC9C65),
   CONST_64(0x2DE92C6F592B0275), CONST_64(0x4A7484AA6EA6E483), CONST_64(0x5CB0A9DCBD41FBD4), CONST_64(0x76F988DA831153B5),
   CONST_64(0x983E5152EE66DFAB), CONST_64(0xA831C66D2DB43210), CONST_64(0xB00327C898FB213F), CONST_64(0xBF597FC7BEEF0EE4),
   CONST_64(0xC6E00BF33DA88FC2), CONST_64(0xD5A79147930AA725), CONST_64(0x06CA6351E003826F), CONST_64(0x142929670A0E6E70),
   CONST_64(0x27B70A8546D22FFC), CONST_64(0x2E1B21385C26C926), CONST_64(0x4D2C6DFC5AC42AED), CONST_64(0x53380D139D95B3DF),
   CONST_64(0x650A73548BAF63DE), CONST_64(0x766A0ABB3C77B2A8), CONST_64(0x81C2C92E47EDAEE6), CONST_64(0x92722C851482353B),
   CONST_64(0xA2BFE8A14CF10364), CONST_64(0xA81A664BBC423001), CONST_64(0xC24B8B70D0F89791), CONST_64(0xC76C51A30654BE30),
   CONST_64(0xD192E819D6EF5218), CONST_64(0xD69906245565A910), CONST_64(0xF40E35855771202A), CONST_64(0x106AA07032BBD1B8),
   CONST_64(0x19A4C116B8D2D0C8), CONST_64(0x1E376C085141AB53), CONST_64(0x2748774CDF8EEB99), CONST_64(0x34B0BCB5E19B48A8),
   CONST_64(0x391C0CB3C5C95A63), CONST_64(0x4ED8AA4AE3418ACB), CONST_64(0x5B9CCA4F7763E373), CONST_64(0x682E6FF3D6B2B8A3),
   CONST_64(0x748F82EE5DEFB2FC), CONST_64(0x78A5636F43172F60), CONST_64(0x84C87814A1F0AB72), CONST_64(0x8CC702081A6439EC),
   CONST_64(0x90BEFFFA23631E28), CONST_64(0xA4506CEBDE82BDE9), CONST_64(0xBEF9A3F7B2C67915), CONST_64(0xC67178F2E372532B),
   CONST_64(0xCA273ECEEA26619C), CONST_64(0xD186B8C721C0C207), CONST_64(0xEADA7DD6CDE0EB1E), CONST_64(0xF57D4F7FEE6ED178),
   CONST_64(0x06F067AA72176FBA), CONST_64(0x0A637DC5A2C898A6), CONST_64(0x113F9804BEF90DAE), CONST_64(0x1B710B35131C471B),
   CONST_64(0x28DB77F523047D84), CONST_64(0x32CAAB7B40C72493), CONST_64(0x3C9EBE0A15C9BEBC), CONST_64(0x431D67C49C100D4C),
   CONST_64(0x4CC5D4BECB3E42B6), CONST_64(0x597F299CFC657E2A), CONST_64(0x5FCB6FAB3AD6FAEC), CONST_64(0x6C44198C4A475817)
};

/* setup init hash value */
__INLINE void hashInit(Ipp64u* pHash, const Ipp64u* iv)
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
IPP_OWN_DEFN (static void, sha512_hashInit, (void* pHash))
{
   hashInit((Ipp64u*)pHash, sha512_iv);
}
IPP_OWN_DEFN (static void, sha512_384_hashInit, (void* pHash))
{
   hashInit((Ipp64u*)pHash, sha512_384_iv);
}
IPP_OWN_DEFN (static void, sha512_256_hashInit, (void* pHash))
{
   hashInit((Ipp64u*)pHash, sha512_256_iv);
}
IPP_OWN_DEFN (static void, sha512_224_hashInit, (void* pHash))
{
   hashInit((Ipp64u*)pHash, sha512_224_iv);
}

IPP_OWN_DEFN (static void, sha512_hashUpdate, (void* pHash, const Ipp8u* pMsg, int msgLen))
{
   UpdateSHA512(pHash, pMsg, msgLen, sha512_cnt);
}

/* convert hash into big endian */
IPP_OWN_DEFN (static void, sha512_hashOctString, (Ipp8u* pMD, void* pHashVal))
{
   ((Ipp64u*)pMD)[0] = ENDIANNESS64(((Ipp64u*)pHashVal)[0]);
   ((Ipp64u*)pMD)[1] = ENDIANNESS64(((Ipp64u*)pHashVal)[1]);
   ((Ipp64u*)pMD)[2] = ENDIANNESS64(((Ipp64u*)pHashVal)[2]);
   ((Ipp64u*)pMD)[3] = ENDIANNESS64(((Ipp64u*)pHashVal)[3]);
   ((Ipp64u*)pMD)[4] = ENDIANNESS64(((Ipp64u*)pHashVal)[4]);
   ((Ipp64u*)pMD)[5] = ENDIANNESS64(((Ipp64u*)pHashVal)[5]);
   ((Ipp64u*)pMD)[6] = ENDIANNESS64(((Ipp64u*)pHashVal)[6]);
   ((Ipp64u*)pMD)[7] = ENDIANNESS64(((Ipp64u*)pHashVal)[7]);
}
IPP_OWN_DEFN (static void, sha512_384_hashOctString, (Ipp8u* pMD, void* pHashVal))
{
   ((Ipp64u*)pMD)[0] = ENDIANNESS64(((Ipp64u*)pHashVal)[0]);
   ((Ipp64u*)pMD)[1] = ENDIANNESS64(((Ipp64u*)pHashVal)[1]);
   ((Ipp64u*)pMD)[2] = ENDIANNESS64(((Ipp64u*)pHashVal)[2]);
   ((Ipp64u*)pMD)[3] = ENDIANNESS64(((Ipp64u*)pHashVal)[3]);
   ((Ipp64u*)pMD)[4] = ENDIANNESS64(((Ipp64u*)pHashVal)[4]);
   ((Ipp64u*)pMD)[5] = ENDIANNESS64(((Ipp64u*)pHashVal)[5]);
}
IPP_OWN_DEFN (static void, sha512_256_hashOctString, (Ipp8u* pMD, void* pHashVal))
{
   ((Ipp64u*)pMD)[0] = ENDIANNESS64(((Ipp64u*)pHashVal)[0]);
   ((Ipp64u*)pMD)[1] = ENDIANNESS64(((Ipp64u*)pHashVal)[1]);
   ((Ipp64u*)pMD)[2] = ENDIANNESS64(((Ipp64u*)pHashVal)[2]);
   ((Ipp64u*)pMD)[3] = ENDIANNESS64(((Ipp64u*)pHashVal)[3]);
}
IPP_OWN_DEFN (static void, sha512_224_hashOctString, (Ipp8u* pMD, void* pHashVal))
{
   ((Ipp64u*)pMD)[0] = ENDIANNESS64(((Ipp64u*)pHashVal)[0]);
   ((Ipp64u*)pMD)[1] = ENDIANNESS64(((Ipp64u*)pHashVal)[1]);
   ((Ipp64u*)pMD)[2] = ENDIANNESS64(((Ipp64u*)pHashVal)[2]);
   ((Ipp32u*)pMD)[6] = ENDIANNESS32(((Ipp32u*)pHashVal)[7]);
}

IPP_OWN_DEFN (static void, sha512_msgRep, (Ipp8u* pDst, Ipp64u lenLo, Ipp64u lenHi))
{
   lenHi = LSL64(lenHi,3) | LSR64(lenLo,63-3);
   lenLo = LSL64(lenLo,3);
   ((Ipp64u*)(pDst))[0] = ENDIANNESS64(lenHi);
   ((Ipp64u*)(pDst))[1] = ENDIANNESS64(lenLo);
}

IPP_OWN_DEFN (static IppStatus, GetSizeSHA512, (int* pSize))
{
   /* test pointer */
   IPP_BAD_PTR1_RET(pSize);
   *pSize = sizeof(IppsSHA512State);
   return ippStsNoErr;
}

/* #define cpFinalizeSHA512 OWNAPI(cpFinalizeSHA512) */
   /* IPP_OWN_DECL (void, cpFinalizeSHA512, (DigestSHA512 pHash, const Ipp8u* inpBuffer, int inpLen, Ipp64u lenLo, Ipp64u lenHi)) */
#define cpSHA512MessageDigest OWNAPI(cpSHA512MessageDigest)
   IPP_OWN_DECL (IppStatus, cpSHA512MessageDigest, (DigestSHA512 hash, const Ipp8u* pMsg, int msgLen, const DigestSHA512 IV))
#define InitSHA512 OWNAPI(InitSHA512)
   IPP_OWN_DECL (IppStatus, InitSHA512, (IppsSHA512State* pState, const DigestSHA512 IV))

IPP_OWN_DEFN (static void, cpFinalizeSHA512, (DigestSHA512 pHash, const Ipp8u* inpBuffer, int inpLen, Ipp64u lenLo, Ipp64u lenHi))
{
   /* local buffer and it length */
   Ipp8u buffer[MBS_SHA512*2];
   int bufferLen = inpLen < (MBS_SHA512-(int)MLR_SHA512)? MBS_SHA512 : MBS_SHA512*2; 

   /* copy rest of message into internal buffer */
   CopyBlock(inpBuffer, buffer, inpLen);

   /* padd message */
   buffer[inpLen++] = 0x80;
   PadBlock(0, buffer+inpLen, (cpSize)(bufferLen-inpLen-(int)MLR_SHA512));

   /* message length representation */
   lenHi = LSL64(lenHi,3) | LSR64(lenLo,63-3);
   lenLo = LSL64(lenLo,3);
   ((Ipp64u*)(buffer+bufferLen))[-2] = ENDIANNESS64(lenHi);
   ((Ipp64u*)(buffer+bufferLen))[-1] = ENDIANNESS64(lenLo);

   /* copmplete hash computation */
   UpdateSHA512(pHash, buffer, bufferLen, sha512_cnt);
}

#endif /* #if !defined(_PCP_SHA512_STUFF_H) */
