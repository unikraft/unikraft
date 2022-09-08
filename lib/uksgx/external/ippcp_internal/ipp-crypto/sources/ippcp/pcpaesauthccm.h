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
//     Message Authentication Algorithm
//     Internal Definitions and Internal Functions Prototypes
// 
// 
*/

#if !defined(_CP_AES_CCM_H)
#define _CP_AES_CCM_H

#include "pcprij.h"
#include "pcpaesm.h"

struct _cpAES_CCM {
   Ipp32u   idCtx;            /* CCM ID               */

   Ipp64u   msgLen;           /* length of message to be processed */
   Ipp64u   lenProcessed;     /* message length has been processed */
   Ipp32u   tagLen;           /* length of authentication tag      */
   Ipp32u   counterVal;       /* currnt couter value */
   Ipp8u   ctr0[MBS_RIJ128];  /* counter value */
   Ipp8u     s0[MBS_RIJ128];  /* S0 = ENC(CTR0) content */
   Ipp8u     si[MBS_RIJ128];  /* Si = ENC(CTRi) content */
   Ipp8u    blk[MBS_RIJ128];  /* temporary data container */
   Ipp8u    mac[MBS_RIJ128];  /* current MAC value */

   Ipp8u    cipher[sizeof(IppsAESSpec)];
};

/* alignment */
#define AESCCM_ALIGNMENT   ((int)(sizeof(void*)))

/*
// access macros
*/
#define AESCCM_SET_ID(stt)    ((stt)->idCtx = (Ipp32u)idCtxAESCCM ^ (Ipp32u)IPP_UINT_PTR(stt))
#define AESCCM_MSGLEN(stt)    ((stt)->msgLen)
#define AESCCM_LENPRO(stt)    ((stt)->lenProcessed)
#define AESCCM_TAGLEN(stt)    ((stt)->tagLen)
#define AESCCM_COUNTER(stt)   ((stt)->counterVal)
#define AESCCM_CTR0(stt)      ((stt)->ctr0)
#define AESCCM_S0(stt)        ((stt)->s0)
#define AESCCM_Si(stt)        ((stt)->si)
#define AESCCM_BLK(stt)       ((stt)->blk)
#define AESCCM_MAC(stt)       ((stt)->mac)
#define AESCCM_CIPHER(stt)    (IppsAESSpec*)(&((stt)->cipher))

/* valid context ID */
#define VALID_AESCCM_ID(ctx)  ((((ctx)->idCtx) ^ (Ipp32u)IPP_UINT_PTR((ctx))) == (Ipp32u)idCtxAESCCM)

/*
// Internal functions
*/
#if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)
#define AuthEncrypt_RIJ128_AES_NI OWNAPI(AuthEncrypt_RIJ128_AES_NI)
   IPP_OWN_DECL (void, AuthEncrypt_RIJ128_AES_NI, (const Ipp8u* inpBlk, Ipp8u* outBlk, int nr, const void* pRKey, Ipp32u len, void* pLocalCtx))
#define DecryptAuth_RIJ128_AES_NI OWNAPI(DecryptAuth_RIJ128_AES_NI)
   IPP_OWN_DECL (void, DecryptAuth_RIJ128_AES_NI, (const Ipp8u* inpBlk, Ipp8u* outBlk, int nr, const void* pRKey, Ipp32u len, void* pLocalCtx))
#endif

/* Counter block formatter */
static Ipp8u* CounterEnc(Ipp32u* pBuffer, int fmt, Ipp64u counter)
{
   #if (IPP_ENDIAN == IPP_LITTLE_ENDIAN)
   pBuffer[0] = ENDIANNESS(IPP_HIDWORD(counter));
   pBuffer[1] = ENDIANNESS(IPP_LODWORD(counter));
   #else
   pBuffer[0] = IPP_HIDWORD(counter);
   pBuffer[1] = IPP_LODWORD(counter);
   #endif
   return (Ipp8u*)pBuffer + 8 - fmt;
}

static int cpSizeofCtx_AESCCM(void)
{
   return sizeof(IppsAES_CCMState);
}

#endif /* _CP_AES_CCM_H*/
