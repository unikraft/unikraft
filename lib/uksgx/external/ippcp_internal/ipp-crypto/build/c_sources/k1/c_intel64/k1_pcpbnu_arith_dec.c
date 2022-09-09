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
//     Internal Unsigned arithmetic
// 
//  Contents:
//     cpDec_BNU()
// 
*/

#include "owncp.h"
#include "pcpbnuarith.h"
#include "pcpbnumisc.h"

/*F*
//    Name: cpDec_BNU
//
// Purpose: decrement BigNum.
//
// Returns:
//    borrow of result of dec BigNum.
//
// Parameters:
//    pA    source BigNum A
//    pR    resultant BigNum
//    ns    size of BigNum
//    val   carry
*F*/

#if !((_IPP32E==_IPP32E_M7) || \
      (_IPP32E==_IPP32E_U8) || \
      (_IPP32E==_IPP32E_Y8) || \
      (_IPP32E>=_IPP32E_E9) || \
      (_IPP32E==_IPP32E_N8))

IPP_OWN_DEFN (BNU_CHUNK_T, cpDec_BNU, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, cpSize ns, BNU_CHUNK_T val))
{
   cpSize i;
   for(i=0; i<ns && val; i++) {
      BNU_CHUNK_T borrow;
      SUB_AB(borrow, pR[i], pA[i], val);
      val = borrow;
   }
   if(pR!=pA)
      for(; i<ns; i++)
         pR[i] = pA[i];
   return val;
}
#endif
