/*******************************************************************************
* Copyright 2012-2021 Intel Corporation
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
//  Purpose:
//     Intel(R) Integrated Performance Primitives.
//     Internal BNU32 arithmetic
// 
// 
*/

#if !defined(_CP_BNU32_ARITH_H)
#define _CP_BNU32_ARITH_H

#define cpAdd_BNU32 OWNAPI(cpAdd_BNU32)
    IPP_OWN_DECL (Ipp32u, cpAdd_BNU32, (Ipp32u* pR, const Ipp32u* pA, const Ipp32u* pB, int ns))
#define cpSub_BNU32 OWNAPI(cpSub_BNU32)
    IPP_OWN_DECL (Ipp32u, cpSub_BNU32, (Ipp32u* pR, const Ipp32u* pA, const Ipp32u* pB, int ns))
#define cpInc_BNU32 OWNAPI(cpInc_BNU32)
    IPP_OWN_DECL (Ipp32u, cpInc_BNU32, (Ipp32u* pR, const Ipp32u* pA, cpSize ns, Ipp32u val))
#define cpDec_BNU32 OWNAPI(cpDec_BNU32)
    IPP_OWN_DECL (Ipp32u, cpDec_BNU32, (Ipp32u* pR, const Ipp32u* pA, cpSize ns, Ipp32u val))
#define cpMulDgt_BNU32 OWNAPI(cpMulDgt_BNU32)
    IPP_OWN_DECL (Ipp32u, cpMulDgt_BNU32, (Ipp32u* pR, const Ipp32u* pA, int ns, Ipp32u val))
#define cpSubMulDgt_BNU32 OWNAPI(cpSubMulDgt_BNU32)
    IPP_OWN_DECL (Ipp32u, cpSubMulDgt_BNU32, (Ipp32u* pR, const Ipp32u* pA, int nsA, Ipp32u val))

#define cpDiv_BNU32 OWNAPI(cpDiv_BNU32)
    IPP_OWN_DECL (int, cpDiv_BNU32, (Ipp32u* pQ, int* nsQ, Ipp32u* pX, int nsX, Ipp32u* pY, int nsY))
#define cpMod_BNU32(pX,sizeX, pM,sizeM) cpDiv_BNU32(NULL,NULL, (pX),(sizeX), (pM),(sizeM))

#define cpFromOS_BNU32 OWNAPI(cpFromOS_BNU32)
    IPP_OWN_DECL (int, cpFromOS_BNU32, (Ipp32u* pBNU, const Ipp8u* pOctStr, int strLen))
#define cpToOS_BNU32 OWNAPI(cpToOS_BNU32)
    IPP_OWN_DECL (int, cpToOS_BNU32, (Ipp8u* pStr, int strLen, const Ipp32u* pBNU, int bnuSize))

#endif /* _CP_BNU32_ARITH_H */
