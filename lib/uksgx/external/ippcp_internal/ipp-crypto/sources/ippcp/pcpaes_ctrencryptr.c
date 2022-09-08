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
//     AES encryption/decryption (CTR mode)
// 
//  Contents:
//        ippsAESEncryptCTR()
//
*/
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#pragma warning(disable: 4206) // empty unit
#endif

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesm.h"
#include "pcptool.h"
#include "pcpaes_ctr_process.h"


/*
// Name: ippsAESEncryptCTR
//
// Purpose:
//        AES-CFB encryption.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//                            pSrc == NULL
//                            pDst == NULL
//                            pCtrValue ==NULL
//    ippStsContextMatchErr   !VALID_AES_ID()
//    ippStsLengthErr         len <1
//    ippStsCTRSizeErr        128 < ctrNumBitSize < 1
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc           pointer to the source data buffer
//    pDst           pointer to the target data buffer
//    len            input buffer length (in bytes)
//    pCtx           pointer to rge AES context
//    pCtrValue      pointer to the counter block
//    ctrNumBitSize  counter block size (bits)
//
// Note:
//    counter will updated on return
//
*/

IPPFUN(IppStatus, ippsAESEncryptCTR,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                     const IppsAESSpec* pCtx,
                                     Ipp8u* pCtrValue, int ctrNumBitSize))
{
   /* test context */
   IPP_BAD_PTR1_RET(pCtx);

   #if(_IPP32E>=_IPP32E_Y8)
   if(AES_NI_ENABLED==RIJ_AESNI(pCtx))
      return ctrNumBitSize==128? cpProcessAES_ctr128(pSrc, pDst, len, pCtx, pCtrValue) :
                                 cpProcessAES_ctr(pSrc, pDst, len, pCtx, pCtrValue, ctrNumBitSize);
   else
      return cpProcessAES_ctr(pSrc, pDst, len, pCtx, pCtrValue, ctrNumBitSize);
   #else
   return cpProcessAES_ctr(pSrc, pDst, len, pCtx, pCtrValue, ctrNumBitSize);
   #endif
}
