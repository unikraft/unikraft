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
//        ippsSMS4EncryptOFB()
//
*/

#include "owncp.h"
#include "pcpsms4.h"
#include "pcptool.h"
#include "pcpsms4_process_ofb8.h"

/*F*
//    Name: ippsSMS4EncryptOFB
//
// Purpose: Encrypts byte data stream according to SMS4 in OFB mode.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//                            pSrc == NULL
//                            pDst == NULL
//                            pIV  == NULL
//    ippStsContextMatchErr   !VALID_SMS4_ID()
//    ippStsLengthErr         len <1
//    ippStsOFBSizeErr        (1>ofbBlkSize || ofbBlkSize>MBS_SMS4)
//    ippStsUnderRunErr       (len%ofbBlkSize)
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc        pointer to the source data buffer
//    pDst        pointer to the target data buffer
//    len         input/output buffer length (in bytes)
//    ofbBlkSize  OFB block size (in bytes)
//    pCtx        pointer to the SMS4 context
//    pIV         pointer to the initialization vector
*F*/
IPPFUN(IppStatus, ippsSMS4EncryptOFB,(const Ipp8u* pSrc, Ipp8u* pDst, int len, int ofbBlkSize,
                                      const IppsSMS4Spec* pCtx,
                                      Ipp8u* pIV))
{
   /* test context */
   IPP_BAD_PTR1_RET(pCtx);
   /* test the context ID */
   IPP_BADARG_RET(!VALID_SMS4_ID(pCtx), ippStsContextMatchErr);

   /* test source, target buffers and initialization pointers */
   IPP_BAD_PTR3_RET(pSrc, pIV, pDst);
   /* test stream length */
   IPP_BADARG_RET((len<1), ippStsLengthErr);
   /* test OFB value */
   IPP_BADARG_RET(((1>ofbBlkSize) || (MBS_SMS4<ofbBlkSize)), ippStsOFBSizeErr);
   /* test stream integrity */
   IPP_BADARG_RET((len%ofbBlkSize), ippStsUnderRunErr);

   cpProcessSMS4_ofb8(pSrc, pDst, len, ofbBlkSize, pCtx, pIV);
   return ippStsNoErr;
}
