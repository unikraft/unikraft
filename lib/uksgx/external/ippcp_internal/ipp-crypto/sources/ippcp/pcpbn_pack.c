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
//     cpPackBigNumCtx()
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcptool.h"


/*F*
//    Name: cpPackBigNumCtx
//
// Purpose: Serialize bigNum context
//
// Parameters:
//    pBN     BigNum
//    pBuffer buffer
*F*/
IPP_OWN_DEFN (void, cpPackBigNumCtx, (const IppsBigNumState* pBN, Ipp8u* pBuffer))
{
   IppsBigNumState* pB = (IppsBigNumState*)(pBuffer);
   CopyBlock(pBN, pB, sizeof(IppsBigNumState));

   cpSize dataAlignment = (cpSize)(IPP_INT_PTR(BN_NUMBER(pBN)) - IPP_INT_PTR(pBN) - (IPP_INT64)sizeof(IppsBigNumState));

   BN_NUMBER(pB) = (BNU_CHUNK_T*)((Ipp8u*)NULL + IPP_INT_PTR(BN_NUMBER(pBN))-IPP_INT_PTR(pBN) - dataAlignment);
   BN_BUFFER(pB) = (BNU_CHUNK_T*)((Ipp8u*)NULL + IPP_INT_PTR(BN_BUFFER(pBN))-IPP_INT_PTR(pBN) - dataAlignment);

   CopyBlock(BN_NUMBER(pBN), (Ipp8u*)pB+IPP_UINT_PTR(BN_NUMBER(pB)), BN_ROOM(pBN)*(Ipp32s)sizeof(BNU_CHUNK_T));
   CopyBlock(BN_BUFFER(pBN), (Ipp8u*)pB+IPP_UINT_PTR(BN_BUFFER(pB)), BN_ROOM(pBN)*(Ipp32s)sizeof(BNU_CHUNK_T));
}
