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
//     Initialization of AES
// 
//  Contents:
//        ippsAESSetKey()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesm.h"
#include "pcprij128safe.h"
#include "pcptool.h"

/*F*
//    Name: ippsAESSetKey
//
// Purpose: Set/reset new secret key for future usage.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//    ippStsLengthErr         keyLen != 16
//                            keyLen != 24
//                            keyLen != 32
//    ippStsContextMatchErr   !VALID_AES_ID()
//    ippStsNoErr             no errors
//
// Parameters:
//    pKey        security key
//    keyLen      length of the secret key (in bytes)
//    pCtx        pointer to AES initialized context
//
// Note:
//    if pKey==NULL, then zero value key being setup
//
*F*/
IPPFUN(IppStatus, ippsAESSetKey,(const Ipp8u* pKey, int keyLen, IppsAESSpec* pCtx))
{
   /* test pointers */
   IPP_BAD_PTR1_RET(pCtx);

   /* test the context ID */
   IPP_BADARG_RET(!VALID_AES_ID(pCtx), ippStsContextMatchErr);

   /* make sure in legal keyLen */
   IPP_BADARG_RET(keyLen!=16 && keyLen!=24 && keyLen!=32, ippStsLengthErr);

   return ippsAESInit(pKey, keyLen, pCtx, sizeof(IppsAESSpec));
}
