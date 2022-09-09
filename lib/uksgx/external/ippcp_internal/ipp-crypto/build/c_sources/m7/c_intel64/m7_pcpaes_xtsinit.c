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
//     AES-XTS Functions (IEEE P1619)
// 
//  Contents:
//     ippsAES_XTSInit()
//
*/

#include "owncp.h"
#include "pcpaesmxts.h"
#include "pcptool.h"

/*F*
//    Name: ippsAES_XTSInit
//
// Purpose: Init AES_XTS context for future usage.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//    ippStsMemAllocErr       size of buffer is not match fro operation
//    ippStsLengthErr         keyLen != 16*8*2 &&
//                                   != 32*8*2
//    ippStsNoErr             no errors
//
// Parameters:
//    pKey        pointer to the secret key
//    keyLen      length of the secret key in bits
//    pCtx        pointer to the AES-XTS context
//    ctxSize     available size (in bytes) of buffer above
//    duBitSize   length of Data Unit in bits
//
*F*/

IPPFUN(IppStatus, ippsAES_XTSInit,(const Ipp8u* pKey, int keyLen,
                                    int duBitsize,
                                    IppsAES_XTSSpec* pCtx, int ctxSize))
{
   /* test key and keyLenBits */
   IPP_BAD_PTR1_RET(pKey);
   IPP_BADARG_RET(keyLen!=16*BYTESIZE*2 && keyLen!=32*BYTESIZE*2, ippStsLengthErr);

   /* test DU parameters */
   IPP_BADARG_RET(duBitsize<IPP_AES_BLOCK_BITSIZE, ippStsLengthErr);

   /* test context pointer */
   IPP_BAD_PTR1_RET(pCtx);
   /* test context pointer */
   IPP_BADARG_RET((int)sizeof(IppsAES_XTSSpec) > ctxSize, ippStsMemAllocErr);

   {
      IppsAESSpec* pdatAES = &pCtx->datumAES;
      IppsAESSpec* ptwkAES = &pCtx->tweakAES;

      int keySize = keyLen/2/BYTESIZE;
      const Ipp8u* pdatKey = pKey;
      const Ipp8u* ptwkKey = pKey+keySize;

      IppStatus sts = ippStsNoErr;
      sts = ippsAESInit(pdatKey, keySize, pdatAES, sizeof(IppsAESSpec));
      if(ippStsNoErr!=sts) return sts;

      sts = ippsAESInit(ptwkKey, keySize, ptwkAES, sizeof(IppsAESSpec));
      if(ippStsNoErr!=sts) return sts;

      AES_XTS_SET_ID(pCtx);
      pCtx->duBitsize = duBitsize;
      return sts;
   }
}
