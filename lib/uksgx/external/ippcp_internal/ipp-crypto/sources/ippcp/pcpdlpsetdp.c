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
//     DL over Prime Finite Field (setup/retrieve domain parameters)
// 
//  Contents:
//        ippsDLPSetDP()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdlp.h"

/*F*
//    Name: ippsDLPSetDP
//
// Purpose: Set tagged DL Domain Parameter.
//
// Returns:                Reason:
//    ippStsNullPtrErr        NULL == pDP
//                            NULL == pDL
//
//    ippStsContextMatchErr   illegal pDP->idCtx
//                            illegal pDL->idCtx
//
//    ippStsRangeErr          not enough room for pDP
//
//    ippStsBadArgErr         invalid key tag
//
//    errors produced by      ippsMontSet()
//                            ippsMontForm()
//
//    ippStsNoErr             no errors
//
// Parameters:
//    pDP      pointer to the DL domain parameter
//    tag      DLP key component tag
//    pDL      pointer to the DL context
//
*F*/
IPPFUN(IppStatus, ippsDLPSetDP,(const IppsBigNumState* pDP, IppDLPKeyTag tag, IppsDLPState* pDL))
{
   /* test DL context */
   IPP_BAD_PTR1_RET(pDL);
   IPP_BADARG_RET(!DLP_VALID_ID(pDL), ippStsContextMatchErr);

   /* test DL parameter to be set */
   IPP_BAD_PTR1_RET(pDP);
   IPP_BADARG_RET(!BN_VALID_ID(pDP), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_NEGATIVE(pDP), ippStsBadArgErr);

   {
      IppStatus sts = ippStsNoErr;

      cpBN_zero(DLP_X(pDL));
      cpBN_zero(DLP_YENC(pDL));

      switch(tag) {
         case ippDLPkeyP:
            DLP_FLAG(pDL) &=(Ipp32u)~ippDLPkeyP;
            sts = gsModEngineInit(DLP_MONTP0(pDL), (Ipp32u*)BN_NUMBER(pDP), cpBN_bitsize(pDP), DLP_MONT_POOL_LENGTH, gsModArithDLP());
            if(ippStsNoErr==sts) {
               DLP_FLAG(pDL) |= ippDLPkeyP;
            }
            break;
         case ippDLPkeyR:
            DLP_FLAG(pDL) &=(Ipp32u)~ippDLPkeyR;
            sts = gsModEngineInit(DLP_MONTR(pDL), (Ipp32u*)BN_NUMBER(pDP), cpBN_bitsize(pDP), DLP_MONT_POOL_LENGTH, gsModArithDLP());
            if(ippStsNoErr==sts)
               DLP_FLAG(pDL) |= ippDLPkeyR;
            break;
         case ippDLPkeyG:
            DLP_FLAG(pDL) &=(Ipp32u)~ippDLPkeyG;
            if(DLP_FLAG(pDL)&ippDLPkeyP) {
               cpMontEnc_BN(DLP_GENC(pDL), pDP, DLP_MONTP0(pDL));
               DLP_FLAG(pDL) |= ippDLPkeyG;
            }
            else
               sts = ippStsIncompleteContextErr;
            break;
         default:
            sts = ippStsBadArgErr;
      }

      return sts;
   }
}
