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
//     EC over Prime Finite Field (EC Point operations)
// 
//  Contents:
//        ippsECCPCheckPoint()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpeccp.h"

/*F*
//    Name: ippsECCPCheckPoint
//
// Purpose: Check EC point:
//             - is point lie on EC
//             - is point at infinity
//
// Returns:                Reason:
//    ippStsNullPtrErr        NULL == pEC
//                            NULL == pP
//                            NULL == pResult
//
//    ippStsContextMatchErr   illegal pEC->idCtx
//                            illegal pP->idCtx
//
//    ippStsNoErr             no errors
//
// Parameters:
//    pPoint      pointer to the EC Point context
//    pEC        pointer to the ECCP context
//    pResult     pointer to the result:
//                         ippECValid
//                         ippECPointIsNotValid
//                         ippECPointIsAtInfinite
//
*F*/
IPPFUN(IppStatus, ippsECCPCheckPoint,(const IppsECCPPointState* pP,
                                          IppECResult* pResult,
                                          IppsECCPState* pEC))
{
   return ippsGFpECTstPoint(pP, pResult, pEC);
}
