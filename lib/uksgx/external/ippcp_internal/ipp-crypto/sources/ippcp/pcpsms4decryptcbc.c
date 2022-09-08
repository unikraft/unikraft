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
//        ippsSMS4DecryptCBC()
//
*/

#include "owncp.h"
#include "pcpsms4.h"
#include "pcptool.h"
#include "pcpsms4_decrypt_cbc.h"

/*F*
//    Name: ippsSMS4DecryptCBC
//
// Purpose: SMS4-CBC decryption.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//                            pSrc == NULL
//                            pDst == NULL
//                            pIV  == NULL
//    ippStsContextMatchErr   !VALID_SMS4_ID()
//    ippStsLengthErr         len <1
//    ippStsUnderRunErr       0!=(len&(MBS_SMS4-1))
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc        pointer to the source data buffer
//    pDst        pointer to the target data buffer
//    len         input/output buffer length (in bytes)
//    pCtx        pointer to the SMS4 context
//    pIV         pointer to the initialization vector
//
*F*/

IPPFUN(IppStatus, ippsSMS4DecryptCBC,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsSMS4Spec* pCtx,
                                      const Ipp8u* pIV))
{
   /* test context */
   IPP_BAD_PTR1_RET(pCtx);
   /* test the context ID */
   IPP_BADARG_RET(!VALID_SMS4_ID(pCtx), ippStsContextMatchErr);

   /* test source, target buffers and initialization pointers */
   IPP_BAD_PTR3_RET(pSrc, pIV, pDst);
   /* test stream length */
   IPP_BADARG_RET((len<1), ippStsLengthErr);
   /* test stream integrity */
   IPP_BADARG_RET((len&(MBS_SMS4-1)), ippStsUnderRunErr);

   /* do encryption */
   cpDecryptSMS4_cbc(pIV, pSrc, pDst, len, pCtx);

   return ippStsNoErr;
}
