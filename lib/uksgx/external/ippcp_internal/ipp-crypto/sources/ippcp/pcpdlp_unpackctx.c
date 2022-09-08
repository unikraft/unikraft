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
//     DL over Prime Field (initialization)
// 
//  Contents:
//        cpUnpackDLPCtx()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdlp.h"
#include "pcptool.h"

IPP_OWN_DEFN (void, cpUnpackDLPCtx, (const Ipp8u* pBuffer, IppsDLPState* pDLP))
{
   IppsDLPState* pB = (IppsDLPState*)(pBuffer);

   CopyBlock(pB, pDLP, sizeof(IppsDLPState));
   DLP_MONTP0(pDLP) =   (gsModEngine*)((Ipp8u*)pDLP+ IPP_UINT_PTR(DLP_MONTP0(pB)));
   DLP_MONTP1(pDLP) = NULL;
   DLP_MONTR(pDLP)   =   (gsModEngine*)((Ipp8u*)pDLP+ IPP_UINT_PTR(DLP_MONTR(pB)));

   DLP_GENC(pDLP)    = (IppsBigNumState*)((Ipp8u*)pDLP+ IPP_UINT_PTR(DLP_GENC(pB)));
   DLP_X(pDLP)       = (IppsBigNumState*)((Ipp8u*)pDLP+ IPP_UINT_PTR(DLP_X(pB)));
   DLP_YENC(pDLP)    = (IppsBigNumState*)((Ipp8u*)pDLP+ IPP_UINT_PTR(DLP_YENC(pB)));

   DLP_PRIMEGEN(pDLP)=  (IppsPrimeState*)((Ipp8u*)pDLP+ IPP_UINT_PTR(DLP_PRIMEGEN(pB)));

   DLP_METBL(pDLP)   = (BNU_CHUNK_T*)((Ipp8u*)pDLP+ IPP_UINT_PTR(DLP_METBL(pB)));
   DLP_BNCTX(pDLP)   =      (BigNumNode*)((Ipp8u*)pDLP+ IPP_UINT_PTR(DLP_BNCTX(pB)));
   #if defined(_USE_WINDOW_EXP_)
   DLP_BNUCTX0(pDLP) = (WINDOW==DLP_EXPMETHOD(pDLP))?(BNU_CHUNK_T*)((Ipp8u*)pDLP+ IPP_UINT_PTR(DLP_BNUCTX0(pB))) : NULL;
   DLP_BNUCTX1(pDLP) = NULL;
   #endif

   gsUnpackModEngineCtx((Ipp8u*)pB+IPP_UINT_PTR(DLP_MONTP0(pB)), DLP_MONTP0(pDLP));
   gsUnpackModEngineCtx((Ipp8u*)pB+IPP_UINT_PTR(DLP_MONTR(pB)),    DLP_MONTR(pDLP));
   cpUnpackBigNumCtx((Ipp8u*)pB+IPP_UINT_PTR(DLP_GENC(pB)),   DLP_GENC(pDLP));
   cpUnpackBigNumCtx((Ipp8u*)pB+IPP_UINT_PTR(DLP_X(pB)),      DLP_X(pDLP));
   cpUnpackBigNumCtx((Ipp8u*)pB+IPP_UINT_PTR(DLP_YENC(pB)),   DLP_YENC(pDLP));
   cpUnpackPrimeCtx((Ipp8u*)pB+IPP_UINT_PTR(DLP_PRIMEGEN(pB)),DLP_PRIMEGEN(pDLP));
   cpBigNumListInit(DLP_BITSIZEP(pDLP)+1, BNLISTSIZE, DLP_BNCTX(pDLP));
}
