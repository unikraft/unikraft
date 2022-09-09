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
//        cpPackDLPCtx()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdlp.h"
#include "pcptool.h"

IPP_OWN_DEFN (void, cpPackDLPCtx, (const IppsDLPState* pDLP, Ipp8u* pBuffer))
{
   IppsDLPState* pB = (IppsDLPState*)(pBuffer);

   CopyBlock(pDLP, pB, sizeof(IppsDLPState));
   DLP_MONTP0(pB) = (gsModEngine*)((Ipp8u*)NULL + IPP_UINT_PTR(DLP_MONTP0(pDLP))-IPP_UINT_PTR(pDLP));
   DLP_MONTP1(pB)  = NULL;
   DLP_MONTR(pB)   =   (gsModEngine*)((Ipp8u*)NULL + IPP_UINT_PTR(DLP_MONTR(pDLP))   -IPP_UINT_PTR(pDLP));

   DLP_GENC(pB)    = (IppsBigNumState*)((Ipp8u*)NULL + IPP_UINT_PTR(DLP_GENC(pDLP))    -IPP_UINT_PTR(pDLP));
   DLP_X(pB)       = (IppsBigNumState*)((Ipp8u*)NULL + IPP_UINT_PTR(DLP_X(pDLP))       -IPP_UINT_PTR(pDLP));
   DLP_YENC(pB)    = (IppsBigNumState*)((Ipp8u*)NULL + IPP_UINT_PTR(DLP_YENC(pDLP))    -IPP_UINT_PTR(pDLP));

   DLP_PRIMEGEN(pB)=  (IppsPrimeState*)((Ipp8u*)NULL + IPP_UINT_PTR(DLP_PRIMEGEN(pDLP))-IPP_UINT_PTR(pDLP));

   DLP_METBL(pB)   = (BNU_CHUNK_T*)((Ipp8u*)NULL + IPP_UINT_PTR(DLP_METBL(pDLP)) -IPP_UINT_PTR(pDLP));

   DLP_BNCTX(pB)   =  (BigNumNode*)((Ipp8u*)NULL + IPP_UINT_PTR(DLP_BNCTX(pDLP))   -IPP_UINT_PTR(pDLP));
   #if defined(_USE_WINDOW_EXP_)
   DLP_BNUCTX0(pB) = (WINDOW==DLP_EXPMETHOD(pDLP))?(BNU_CHUNK_T*)((Ipp8u*)NULL + IPP_UINT_PTR(DLP_BNUCTX0(pDLP))-IPP_UINT_PTR(pDLP)) : NULL;
   DLP_BNUCTX1(pB) = NULL;
   #endif

   gsPackModEngineCtx(DLP_MONTP0(pDLP),    (Ipp8u*)pB+IPP_UINT_PTR(DLP_MONTP0(pB)));
   gsPackModEngineCtx(DLP_MONTR(pDLP),     (Ipp8u*)pB+IPP_UINT_PTR(DLP_MONTR(pB)));

   cpPackBigNumCtx(DLP_GENC(pDLP),    (Ipp8u*)pB+IPP_UINT_PTR(DLP_GENC(pB)));
   cpPackBigNumCtx(DLP_X(pDLP),       (Ipp8u*)pB+IPP_UINT_PTR(DLP_X(pB)));
   cpPackBigNumCtx(DLP_YENC(pDLP),    (Ipp8u*)pB+IPP_UINT_PTR(DLP_YENC(pB)));

   cpPackPrimeCtx(DLP_PRIMEGEN(pDLP), (Ipp8u*)pB+IPP_UINT_PTR(DLP_PRIMEGEN(pB)));
}
