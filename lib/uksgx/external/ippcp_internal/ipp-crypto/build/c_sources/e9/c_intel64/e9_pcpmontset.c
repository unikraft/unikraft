/*******************************************************************************
* Copyright 2002-2021 Intel Corporation
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
//               Intel(R) Integrated Performance Primitives
//                   Cryptographic Primitives (ippcp)
// 
//  Contents:
//        ippsMontSet()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpmontgomery.h"
#include "pcptool.h"

/*F*
// Name: ippsMontSet
//
// Purpose: Setup modulus value
//
// Returns:                   Reason:
//    ippStsNullPtrErr           pCtx == NULL
//                               pModulus == NULL
//    ippStsContextMatchErr      !MNT_VALID_ID(pCtx)
//    ippStsBadModulusErr        (pModulus[0] & 1) == 0
//    ippStsOutOfRangeErr        ((Ipp32u)MNT_ROOM(pCtx) < INTERNAL_BNU_LENGTH(size))
//    ippStsLengthErr            size<1
//    ippStsNoErr                no errors
//
// Parameters:
//    pModulus    pointer to the modulus buffer
//    size        length of the  modulus (in Ipp32u chunks).
//    pCtx        pointer to the context
*F*/

IPPFUN(IppStatus, ippsMontSet,(const Ipp32u* pModulus, int size, IppsMontState* pCtx))
{
   IPP_BAD_PTR2_RET(pModulus, pCtx);
   IPP_BADARG_RET(!MNT_VALID_ID(pCtx), ippStsContextMatchErr);

   IPP_BADARG_RET(size<1, ippStsLengthErr);

   /* modulus is not an odd number */
   IPP_BADARG_RET((pModulus[0] & 1) == 0, ippStsBadModulusErr);
   IPP_BADARG_RET(((Ipp32u)MNT_ROOM(pCtx) < (Ipp32u)INTERNAL_BNU_LENGTH(size)), ippStsOutOfRangeErr);

   {
      return cpMontSet(pModulus, size, pCtx);
   }
}
