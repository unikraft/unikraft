/*******************************************************************************
* Copyright 2016-2021 Intel Corporation
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
//     AES-XTS Internal Definitions
// 
// 
*/

#if !defined(_PCP_AES_XTS_H)
#define _PCP_AES_XTS_H

#include "owncp.h"
#include "pcpaesm.h"

#define _NEW_XTS_API_3
#if defined (_NEW_XTS_API_3)
/*
// AES-XTS State
*/
#if !defined(AES_BLK_SIZE)
#define AES_BLK_SIZE (IPP_AES_BLOCK_BITSIZE/BITSIZE(Ipp8u))
#endif

#define AES_BLKS_PER_BUFFER (32)

struct _cpAES_XTS
{
   Ipp32u    idCtx;
   int       duBitsize;         /* size of data unit (in bits) */
   IppsAESSpec datumAES;        /* datum AES context */
   IppsAESSpec tweakAES;        /* tweak AES context */
};

#define AES_XTS_SET_ID(ctx)     ((ctx)->idCtx = (Ipp32u)idCtxAESXTS ^ (Ipp32u)IPP_UINT_PTR(ctx))
/* valid AES_XTS context ID */
#define VALID_AES_XTS_ID(ctx)   ((((ctx)->idCtx) ^ (Ipp32u)IPP_UINT_PTR((ctx))) == (Ipp32u)idCtxAESXTS)

/* size of AES-XTS context */
__INLINE int cpSizeof_AES_XTS_Ctx(void)
{
   return sizeof(IppsAES_XTSSpec);
}

static int IsLegalGeometry(int startCipherBlkNo, int bitLen, int duBitsize)
{
   int duBlocks = (duBitsize+IPP_AES_BLOCK_BITSIZE-1)/IPP_AES_BLOCK_BITSIZE;
   int legalBlk = (0<=startCipherBlkNo && startCipherBlkNo<duBlocks) && ((startCipherBlkNo*IPP_AES_BLOCK_BITSIZE+bitLen)<=duBitsize);
   int legalLen = 0;
   if(0 == duBitsize%IPP_AES_BLOCK_BITSIZE) {
      legalLen = (0 == bitLen%IPP_AES_BLOCK_BITSIZE);
   }
   else
      if(bitLen%IPP_AES_BLOCK_BITSIZE)
         legalLen = (startCipherBlkNo*IPP_AES_BLOCK_BITSIZE+bitLen == duBitsize);
   return legalBlk && legalLen;
}


#endif /* _NEW_XTS_API_ */

#endif /* _PCP_AES_XTS_H */
