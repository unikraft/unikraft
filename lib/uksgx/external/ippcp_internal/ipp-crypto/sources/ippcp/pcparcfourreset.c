/*******************************************************************************
* Copyright 2005-2021 Intel Corporation
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
//     RC4 implementation
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcparcfour.h"
#include "pcptool.h"

/*F*
//    Name: ippsARCFourReset
//
// Purpose: Resrt ARCFOUR context:
//          set current state block to the initial one
//          and zeroes counters
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//    ippStsContextMatchErr   pCtx->idCtx != idCtxARCFOUR
//    ippStsNoErr             no errors
//
// Parameters:
//    pCtx        pointer to the ARCFOUR context
//
*F*/
IPPFUN(IppStatus, ippsARCFourReset, (IppsARCFourState* pCtx))
{
   /* test context */
   IPP_BAD_PTR1_RET(pCtx);
   IPP_BADARG_RET(!RC4_VALID_ID(pCtx), ippStsContextMatchErr);

   {
      /* reset Sbox */
      int n;
      for(n=0; n<256; n++)
         RC4_SBOX(pCtx)[n] = RC4_SBOX0(pCtx)[n];

      /* reset counters */
      RC4_CNTX(pCtx) = 0;
      RC4_CNTY(pCtx) = 0;

      return ippStsNoErr;
   }
}
