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
//     cpUnpackBigNumCtx()
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcptool.h"


/*F*
//    Name: cpUnpackBigNumCtx
//
// Purpose: Deserialize bigNum context
//
// Parameters:
//    pBN     BigNum
//    pBuffer buffer
*F*/


IPP_OWN_DEFN (void, cpUnpackBigNumCtx, (const Ipp8u* pBuffer, IppsBigNumState* pBN))
{
   IppsBigNumState* pB = (IppsBigNumState*)(pBuffer);
   CopyBlock(pBuffer, pBN, sizeof(IppsBigNumState));

   Ipp8u* ptr = (Ipp8u*)pBN;
   ptr += sizeof(IppsBigNumState);
   ptr = IPP_ALIGNED_PTR(ptr, BN_ALIGNMENT);
   BN_NUMBER(pBN) = (BNU_CHUNK_T*)(ptr);
   ptr += BN_ROOM(pBN)*(Ipp32s)sizeof(BNU_CHUNK_T);
   BN_BUFFER(pBN) = (BNU_CHUNK_T*)(ptr);

   cpSize bufferOffset = (cpSize)(IPP_INT_PTR(BN_BUFFER(pBN)) - IPP_INT_PTR(pBN));

   CopyBlock((Ipp8u*)pB+sizeof(IppsBigNumState), BN_NUMBER(pBN), BN_ROOM(pBN)*(Ipp32s)sizeof(BNU_CHUNK_T));
   CopyBlock((Ipp8u*)pB+bufferOffset, BN_BUFFER(pBN), BN_ROOM(pBN)*(Ipp32s)sizeof(BNU_CHUNK_T));
}
