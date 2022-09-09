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
//     EC over Prime Finite Field (setup/retrieve domain parameters)
// 
//  Contents:
//        ippsECCPGetOrderBitSize()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpeccp.h"

/*F*
//    Name: ippsECCPGetOrderBitSize
//
// Purpose: Retrieve size of Base Point Order (in bits).
//
// Returns:                Reason:
//    ippStsNullPtrErr        NULL == pEC
//                            NULL == pBitSize
//
//    ippStsContextMatchErr   illegal pEC->idCtx
//
//    ippStsNoErr             no errors
//
// Parameters:
//    pBitSize pointer to the size of base point order
//    pEC      pointer to the EC context
//
*F*/
IPPFUN(IppStatus, ippsECCPGetOrderBitSize,(int* pBitSize, IppsECCPState* pEC))
{
   /* test pEC */
   IPP_BAD_PTR1_RET(pEC);
   IPP_BADARG_RET(!VALID_ECP_ID(pEC), ippStsContextMatchErr);

   /* test pBitSize*/
   IPP_BAD_PTR1_RET(pBitSize);

   *pBitSize = ECP_ORDBITSIZE(pEC);
   return ippStsNoErr;
}
