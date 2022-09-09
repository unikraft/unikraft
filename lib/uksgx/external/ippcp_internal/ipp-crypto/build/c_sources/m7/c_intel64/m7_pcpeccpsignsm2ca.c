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
//     EC over Prime Finite Field (Sign, SM2 version)
// 
//  Contents:
//     ippsECCPSignSM2()
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpeccp.h"


/*F*
//    Name: ippsECCPSignSM2
//
// Purpose: Signing of message representative.
//          (SM2 version).
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pEC
//                               NULL == pMsgDigest
//                               NULL == pRegPrivate
//                               NULL == pEphPrivate
//                               NULL == pSignR
//                               NULL == pSignS
//
//    ippStsContextMatchErr      illegal pEC->idCtx
//                               illegal pMsgDigest->idCtx
//                               illegal pRegPrivate->idCtx
//                               illegal pEphPrivate->idCtx
//                               illegal pSignR->idCtx
//                               illegal pSignS->idCtx
//
//    ippStsIvalidPrivateKey     (1 + regPrivate) >= order
//
//    ippStsMessageErr           MsgDigest >= order
//                               MsgDigest <  0
//
//    ippStsRangeErr             not enough room for:
//                               signR
//                               signS
//
//    ippStsEphemeralKeyErr      (signR + ephPrivate) == order
//                               (0==signR) || (0==signS)
//
//    ippStsNoErr                no errors
//
// Parameters:
//    pMsgDigest     pointer to the message representative to be signed
//    pRegPrivate    pointer to the regular private key
//    pEphPrivate    pointer to the ephemeral private key
//    pSignR,pSignS  pointer to the signature
//    pEC           pointer to the ECCP context
//
// Note:
//    ephemeral key computes inside ippsECCPSignSM2 in contrast with ippsECCPSignDSA,
//    where ephemeral key pair computed before call and setup in context
//
*F*/
IPPFUN(IppStatus, ippsECCPSignSM2,(const IppsBigNumState* pMsgDigest,
                                   const IppsBigNumState* pRegPrivate,
                                   IppsBigNumState* pEphPrivate,
                                   IppsBigNumState* pSignR, IppsBigNumState* pSignS,
                                   IppsECCPState* pEC))
{
   IPP_BAD_PTR1_RET(pEC);
   IPP_BADARG_RET(!VALID_ECP_ID(pEC), ippStsContextMatchErr);

   return ippsGFpECSignSM2(pMsgDigest,
                           pRegPrivate, pEphPrivate,
                           pSignR, pSignS,
                           pEC, (Ipp8u*)ECP_SBUFFER(pEC));
}
