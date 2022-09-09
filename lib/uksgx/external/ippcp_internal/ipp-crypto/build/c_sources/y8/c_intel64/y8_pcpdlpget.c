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
//        ippsDLPGet()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdlp.h"

/*F*
//    Name: ippsDLPGet
//
// Purpose: Retrieve DLP Domain Parameter.
//
// Returns:                Reason:
//    ippStsNullPtrErr        NULL == pP
//                            NULL == pR
//                            NULL == pG
//                            NULL == pDL
//
//    ippStsContextMatchErr   illegal pP->idCtx
//                            illegal pR->idCtx
//                            illegal pG->idCtx
//                            illegal pDL->idCtx
//
//    ippStsIncompleteContextErr
//                            incomplete context
//
//    ippStsRangeErr          not enough room for:
//                            pP,
//                            pR,
//                            pG
//
//    ippStsNoErr             no errors
//
// Parameters:
//    pP       pointer to the retrieval P
//    pR       pointer to the retrieval R
//    pG       pointer to the retrieval G
//    pDSA     pointer to the DL context
//
*F*/
IPPFUN(IppStatus, ippsDLPGet,(IppsBigNumState* pP,
                              IppsBigNumState* pR,
                              IppsBigNumState* pG,
                              IppsDLPState* pDL))
{
   /* test DL context */
   IPP_BAD_PTR1_RET(pDL);
   IPP_BADARG_RET(!DLP_VALID_ID(pDL), ippStsContextMatchErr);

   /* test flag */
   IPP_BADARG_RET(!DLP_COMPLETE(pDL), ippStsIncompleteContextErr);

   /* test DL parameters */
   IPP_BAD_PTR3_RET(pP, pR, pG);
   IPP_BADARG_RET(!BN_VALID_ID(pP), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pR), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pG), ippStsContextMatchErr);

   /* test size of retrieval parameters */
   IPP_BADARG_RET(BN_ROOM(pP)<BITS_BNU_CHUNK(DLP_BITSIZEP(pDL)), ippStsRangeErr);
   IPP_BADARG_RET(BN_ROOM(pR)<BITS_BNU_CHUNK(DLP_BITSIZER(pDL)), ippStsRangeErr);
   IPP_BADARG_RET(BN_ROOM(pG)<BITS_BNU_CHUNK(DLP_BITSIZEP(pDL)), ippStsRangeErr);

   /* retrieve DSA parameter */
   ippsSet_BN(ippBigNumPOS, BITS2WORD32_SIZE(DLP_BITSIZEP(pDL)), (Ipp32u*)DLP_P(pDL), pP);
   ippsSet_BN(ippBigNumPOS, BITS2WORD32_SIZE(DLP_BITSIZER(pDL)), (Ipp32u*)DLP_R(pDL), pR);

   cpMontDec_BN(pG, DLP_GENC(pDL), DLP_MONTP0(pDL));

   return ippStsNoErr;
}
