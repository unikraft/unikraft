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
//        ippsMontExp()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpmontgomery.h"
#include "pcptool.h"

/*F*
// Name: ippsMontExp
//
// Purpose: computes the Montgomery exponentiation with exponent
//          IppsBigNumState *pE to the given big number integer of Montgomery form
//          IppsBigNumState *pA with respect to the modulus IppsMontState *pCtx.
//
// Returns:                Reason:
//      ippStsNoErr         Returns no error.
//      ippStsNullPtrErr    Returns an error when pointers are null.
//      ippStsBadArgErr     Returns an error when a or b is a negative integer.
//      ippStsScaleRangeErr Returns an error when a or b is more than m.
//      ippStsOutOfRangeErr Returns an error when IppsBigNumState *r is larger than
//                          IppsMontState *m.
//      ippStsContextMatchErr Returns an error when the context parameter does
//                          not match the operation.
//
//
// Parameters:
//      pA      big number integer of Montgomery form within the
//                      range [0,m-1]
//      pE      big number exponent
//      pCtx    Montgomery modulus of IppsMontState
/       pR      the Montgomery exponentation result.
//
// Notes: IppsBigNumState *r should possess enough memory space as to hold the result
//        of the operation. i.e. both pointers r->d and r->buffer should possess
//        no less than (m->n->length) number of 32-bit words.
*F*/

IPPFUN(IppStatus, ippsMontExp, (const IppsBigNumState* pA, const IppsBigNumState* pE, IppsMontState* pCtx, IppsBigNumState* pR))
{
   IPP_BAD_PTR4_RET(pA, pE, pCtx, pR);

   IPP_BADARG_RET(!MNT_VALID_ID(pCtx), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pA), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pE), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pR), ippStsContextMatchErr);

   IPP_BADARG_RET(BN_ROOM(pR) <  MOD_LEN( MNT_ENGINE(pCtx) ), ippStsOutOfRangeErr);
   /* check a */
   IPP_BADARG_RET(BN_NEGATIVE(pA), ippStsBadArgErr);
   IPP_BADARG_RET(cpCmp_BNU(BN_NUMBER(pA), BN_SIZE(pA), MOD_MODULUS( MNT_ENGINE(pCtx) ), MOD_LEN( MNT_ENGINE(pCtx) )) >= 0, ippStsScaleRangeErr);
   /* check e */
   IPP_BADARG_RET(BN_NEGATIVE(pE), ippStsBadArgErr);

   cpMontExpBin_BN(pR, pA, pE, MNT_ENGINE( pCtx) );

   return ippStsNoErr;
}
