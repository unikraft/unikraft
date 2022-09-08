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
//     SMS4 encryption/decryption
// 
//  Contents:
//        ippsSMS4EncryptCTR()
//
*/

#include "owncp.h"
#include "pcpsms4.h"
#include "pcptool.h"

/*F*
//    Name: ippsSMS4EncryptCTR
//
// Purpose: SMS4-CTR encryption.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//                            pSrc == NULL
//                            pDst == NULL
//                            pCtrValue ==NULL
//    ippStsContextMatchErr   !VALID_SMS4_ID()
//    ippStsLengthErr         len <1
//    ippStsCTRSizeErr        128 < ctrNumBitSize < 1
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc           pointer to the source data buffer
//    pDst           pointer to the target data buffer
//    len        input/output buffer length (in bytes)
//    pCtx           pointer to rge SMS4 context
//    pCtrValue      pointer to the counter block
//    ctrNumBitSize  counter block size (bits)
//
// Note:
//    counter will updated on return
//
*F*/

IPPFUN(IppStatus, ippsSMS4EncryptCTR,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsSMS4Spec* pCtx,
                                      Ipp8u* pCtrValue, int ctrNumBitSize))
{
   return cpProcessSMS4_ctr(pSrc, pDst, len, pCtx, pCtrValue, ctrNumBitSize);
}
