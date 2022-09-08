/*******************************************************************************
* Copyright 2007-2021 Intel Corporation
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
//     Ciper-based Message Authentication Code (CMAC) see SP800-38B
//     Internal Definitions and Internal Functions Prototypes
//
*/

#if !defined(_PCP_CMAC_H)
#define _PCP_CMAC_H

#include "pcprij.h"


/*
// Rijndael128 based CMAC context
*/
struct _cpAES_CMAC {
   Ipp32u   idCtx;              /* CMAC  identifier              */
   int      index;              /* internal buffer entry (free)  */
   Ipp8u    k1[MBS_RIJ128];     /* k1 subkey                     */
   Ipp8u    k2[MBS_RIJ128];     /* k2 subkey                     */
   Ipp8u    mBuffer[MBS_RIJ128];/* buffer                        */
   Ipp8u    mMAC[MBS_RIJ128];   /* intermediate digest           */
   IppsRijndael128Spec mCipherCtx;
};

/* alignment */
//#define CMACRIJ_ALIGNMENT (RIJ_ALIGNMENT)
#define AESCMAC_ALIGNMENT  (RIJ_ALIGNMENT)

/*
// Useful macros
*/
#define CMAC_SET_ID(stt)  ((stt)->idCtx = (Ipp32u)idCtxCMAC ^ (Ipp32u)IPP_UINT_PTR(stt))
#define CMAC_INDX(stt)    ((stt)->index)
#define CMAC_K1(stt)      ((stt)->k1)
#define CMAC_K2(stt)      ((stt)->k2)
#define CMAC_BUFF(stt)    ((stt)->mBuffer)
#define CMAC_MAC(stt)     ((stt)->mMAC)
#define CMAC_CIPHER(stt)  ((stt)->mCipherCtx)

/* valid context ID */
#define VALID_AESCMAC_ID(ctx) ((((ctx)->idCtx) ^ (Ipp32u)IPP_UINT_PTR((ctx))) == (Ipp32u)idCtxCMAC)

#define cpAESCMAC_Update_AES_NI OWNAPI(cpAESCMAC_Update_AES_NI)
   IPP_OWN_DECL (void, cpAESCMAC_Update_AES_NI, (Ipp8u* pMac, const Ipp8u* inpBlk, int nBlks, int nr, const Ipp8u* pKeys))

#endif /* _PCP_CMAC_H */
