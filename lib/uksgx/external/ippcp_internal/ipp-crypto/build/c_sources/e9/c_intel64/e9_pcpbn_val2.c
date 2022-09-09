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
//     cpBN_TwoRef()
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcptool.h"


/*F*
//    Name: cpBN_TwoRef
//
// Purpose: BN(2) and reference
//
//  Return:
//      BigNum = 2
*F*/

IPP_OWN_DEFN (IppsBigNumState*, cpBN_TwoRef, (void))
{
   static IppsBigNumStateChunk cpChunk_BN2 = {
      {
         idCtxUnknown,
         ippBigNumPOS,
         1,1,
         &cpChunk_BN2.value,&cpChunk_BN2.temporary
      },
      2,0
   };
   BN_SET_ID(&cpChunk_BN2.bn);
   return &cpChunk_BN2.bn;
}
