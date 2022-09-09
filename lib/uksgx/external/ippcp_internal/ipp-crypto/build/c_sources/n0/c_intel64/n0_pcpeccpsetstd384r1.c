/*******************************************************************************
* Copyright 2016-2021 Intel Corporation
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
//     EC over Prime Finite Field (setup/retrieve domain parameters)
//
//  Contents:
//        ippsECCPSetStd384r1()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpeccp.h"

/*F*
//    Name: ippsECCPSetStd384r1
//
// Purpose: Set EC384r1 parameters
//
// Returns:                Reason:
//    ippStsNullPtrErr        NULL == pEC
//
//    ippStsContextMatchErr   illegal pEC->idCtx
//
//    ippStsNoErr             no errors
//
// Parameters:
//    pEC     pointer to the ECC context
//
*F*/
IPPFUN(IppStatus, ippsECCPSetStd384r1, (IppsECCPState* pEC))
{
   /* test pEC */
   IPP_BAD_PTR1_RET(pEC);

   /* set domain parameters */
   return ECCPSetDP(ippsGFpMethod_p384r1(),
                        BITS_BNU_CHUNK(384), secp384r1_p,
                        BITS_BNU_CHUNK(384), secp384r1_a,
                        BITS_BNU_CHUNK(384), secp384r1_b,
                        BITS_BNU_CHUNK(384), secp384r1_gx,
                        BITS_BNU_CHUNK(384), secp384r1_gy,
                        BITS_BNU_CHUNK(384), secp384r1_r,
                        secp384r1_h,
                        pEC);
}
