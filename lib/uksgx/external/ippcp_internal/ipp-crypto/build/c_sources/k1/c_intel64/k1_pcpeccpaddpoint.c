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
//        ippsECCPAddPoint()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpeccp.h"

/*F*
//    Name: ippsECCPAddPoint
//
// Purpose: Perforn EC point operation: R = P+Q
//
// Returns:                Reason:
//    ippStsNullPtrErr        NULL == pEC
//                            NULL == pP
//                            NULL == pQ
//                            NULL == pR
//
//    ippStsContextMatchErr   illegal pEC->idCtx
//                            illegal pP->idCtx
//                            illegal pQ->idCtx
//                            illegal pR->idCtx
//
//    ippStsNoErr             no errors
//
// Parameters:
//    pP          pointer to the source EC Point context
//    pQ          pointer to the source EC Point context
//    pR          pointer to the resultant EC Point context
//    pEC        pointer to the ECCP context
//
*F*/
IPPFUN(IppStatus, ippsECCPAddPoint,(const IppsECCPPointState* pP,
                                        const IppsECCPPointState* pQ,
                                        IppsECCPPointState* pR,
                                        IppsECCPState* pEC))
{
   return ippsGFpECAddPoint(pP, pQ, pR, pEC);
}
