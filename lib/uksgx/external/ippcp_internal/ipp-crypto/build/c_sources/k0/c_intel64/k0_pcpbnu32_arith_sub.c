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
//  Purpose:
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     Internal BNU32 arithmetic.
// 
//  Contents:
//     cpSub_BNU32()
// 
*/

#include "owncp.h"
#include "pcpbnumisc.h"
#include "pcpbnu32misc.h"
#include "pcpbnu32arith.h"


/*F*
//    Name: cpSub_BNU32
//
// Purpose: substract BNU32.
//
// Returns:
//    borrow
//
// Parameters:
//    pA    source
//    pB    source
//    ns    size
//    pR    result
//
*F*/
IPP_OWN_DEFN (Ipp32u, cpSub_BNU32, (Ipp32u* pR, const Ipp32u* pA, const Ipp32u* pB, cpSize ns))
{
   Ipp32u borrow = 0;
   cpSize i;
   for(i=0; i<ns; i++) {
      Ipp64u t = (Ipp64u)(pA[i]) - pB[i] - borrow;
      pR[i] = IPP_LODWORD(t);
      borrow = 0-IPP_HIDWORD(t);
   }
   return borrow;
}
