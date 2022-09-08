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
//     EC over Prime Finite Field (initialization)
// 
//  Contents:
//        ippsECCPInitStd256r1()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpeccp.h"

/*F*
//    Name: ippsECCPInitStd256r1
//
// Purpose: Init ECC context for secp256r1 [SEC2].
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pECC
//
//    ippStsNoErr                no errors
//
// Parameters:
//    pECC        pointer to the ECC context
//
*F*/
IPPFUN(IppStatus, ippsECCPInitStd256r1, (IppsECCPState* pEC))
{
   return ippsECCPInit(256, pEC);
}
