/*******************************************************************************
* Copyright 2003-2021 Intel Corporation
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
//     EC over Prime Finite Field (EC Key Generation)
// 
//  Contents:
//     ippsECCPValidateKeyPair()
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpeccp.h"


/*F*
//    Name: ippsECCPValidateKeyPair
//
// Purpose: Validate (private,public) Key Pair
//
// Returns:                Reason:
//    ippStsNullPtrErr        NULL == pEC
//                            NULL == pPrivate
//                            NULL == pPublic
//                            NULL == pResult
//
//    ippStsContextMatchErr   illegal pEC->idCtx
//                            illegal pPrivate->idCtx
//                            illegal pPublic->idCtx
//
//    ippStsNoErr             no errors
//
// Parameters:
//    pPrivate    pointer to the private key
//    pPublic     pointer to the public  key
//    pResult     pointer to the result:
//                ippECValid/ippECInvalidPrivateKey/ippECPointIsAtInfinite/ippECInvalidPublicKey
//    pEC        pointer to the ECCP context
//
*F*/
IPPFUN(IppStatus, ippsECCPValidateKeyPair, (const IppsBigNumState* pPrivate,
                                            const IppsECCPPointState* pPublic,
                                            IppECResult* pResult,
                                            IppsECCPState* pEC))
{
   IPP_BAD_PTR1_RET(pEC);
   IPP_BADARG_RET(!VALID_ECP_ID(pEC), ippStsContextMatchErr);

   return ippsGFpECTstKeyPair(pPrivate, pPublic, pResult, pEC, (Ipp8u*)ECP_SBUFFER(pEC));
}
