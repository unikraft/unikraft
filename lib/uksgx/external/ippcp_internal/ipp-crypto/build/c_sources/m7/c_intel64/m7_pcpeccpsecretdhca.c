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
//     EC over Prime Finite Field (EC Key Ecxhange, Diffie-Hellman version)
// 
//  Contents:
//     ippsECCPSharedSecretDH()
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpeccp.h"


/*F*
//    Name: ippsECCPSharedSecretDH
//
// Purpose: Shared Secret Value Derivation
//          (Diffie-Hellman version).
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pEC
//                               NULL == pPrivateA
//                               NULL == pPublicB
//                               NULL == pShare
//
//    ippStsContextMatchErr      illegal pEC->idCtx
//                               illegal pPrivateA->idCtx
//                               illegal pPublicB->idCtx
//                               illegal pShare->idCtx
//
//    ippStsRangeErr             not enough room for share key
//
//    ippStsShareKeyErr          (infinity) => z
//
//    ippStsNoErr                no errors
//
// Parameters:
//    pPrivateA   pointer to own   private key
//    pPublicB    pointer to alien public  key
//    pShare      pointer to the shareds secret value
//    pEC        pointer to the ECCP context
//
*F*/
IPPFUN(IppStatus, ippsECCPSharedSecretDH,(const IppsBigNumState* pPrivateA,
                                          const IppsECCPPointState* pPublicB,
                                          IppsBigNumState* pShare,
                                          IppsECCPState* pEC))
{
   IPP_BAD_PTR1_RET(pEC);
   IPP_BADARG_RET(!VALID_ECP_ID(pEC), ippStsContextMatchErr);

   return ippsGFpECSharedSecretDH(pPrivateA, pPublicB, pShare, pEC, (Ipp8u*)ECP_SBUFFER(pEC));
}
