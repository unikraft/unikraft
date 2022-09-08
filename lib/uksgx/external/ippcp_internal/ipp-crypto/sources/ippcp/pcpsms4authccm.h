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
//     Message Authentication Algorithm
//     Internal Definitions and Internal Functions Prototypes
// 
// 
*/

#if !defined(_CP_SMS4_CCM_H)
#define _CP_SMS4_CCM_H

#include "pcpsms4.h"

struct _cpSMS4_CCM {
   Ipp32u   idCtx;            /* CCM ID               */

   Ipp64u   msgLen;           /* length of message to be processed */
   Ipp64u   lenProcessed;     /* message length has been processed */
   Ipp32u   tagLen;           /* length of authentication tag      */
   Ipp32u   counterVal;       /* currnt couter value */
   Ipp8u   ctr0[MBS_SMS4];    /* counter value */
   Ipp8u     s0[MBS_SMS4];    /* S0 = ENC(CTR0) content */
   Ipp8u     si[MBS_SMS4];    /* Si = ENC(CTRi) content */
   Ipp8u    blk[MBS_SMS4];    /* temporary data container */
   Ipp8u    mac[MBS_SMS4];    /* current MAC value */

   Ipp8u    cipher[sizeof(IppsSMS4Spec)];
};

/* alignment */
#define SMS4CCM_ALIGNMENT   ((int)(sizeof(void*)))

/*
// access macros
*/
#define SMS4CCM_SET_ID(stt)    ((stt)->idCtx = (Ipp32u)idCtxAESCCM ^ (Ipp32u)IPP_UINT_PTR(stt))
#define SMS4CCM_MSGLEN(stt)    ((stt)->msgLen)
#define SMS4CCM_LENPRO(stt)    ((stt)->lenProcessed)
#define SMS4CCM_TAGLEN(stt)    ((stt)->tagLen)
#define SMS4CCM_COUNTER(stt)   ((stt)->counterVal)
#define SMS4CCM_CTR0(stt)      ((stt)->ctr0)
#define SMS4CCM_S0(stt)        ((stt)->s0)
#define SMS4CCM_Si(stt)        ((stt)->si)
#define SMS4CCM_BLK(stt)       ((stt)->blk)
#define SMS4CCM_MAC(stt)       ((stt)->mac)
#define SMS4CCM_CIPHER(stt)    (IppsSMS4Spec*)(&((stt)->cipher))

/* valid context ID */
#define VALID_SMS4CCM_ID(ctx)  ((((ctx)->idCtx) ^ (Ipp32u)IPP_UINT_PTR((ctx))) == (Ipp32u)idCtxAESCCM)

static int cpSizeofCtx_SMS4CCM(void)
{
   return sizeof(IppsSMS4_CCMState);
}

/* Counter block formatter */
static
Ipp8u* CounterEnc(Ipp32u* pBuffer, int fmt, Ipp64u counter)
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

#endif /* _CP_SMS4_CCM_H*/
