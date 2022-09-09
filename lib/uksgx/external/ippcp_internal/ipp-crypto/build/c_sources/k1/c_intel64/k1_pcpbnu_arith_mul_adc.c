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
//     cpMulAdc_BNU_school()
// 
*/

#include "owncp.h"
#include "pcpbnuarith.h"
#include "pcpbnumisc.h"


/*F*
//    Name: cpMulAdc_BNU_school
//
// Purpose: Multiply 2 BigNums.
//
// Returns:
//    extention of result of multiply 2 BigNums
// Parameters:
//    pA    source BigNum
//    pB    source BigNum
//    nsA   size of A
//    nsB   size of B
//    pR    resultant BigNum
//
*F*/
#if !((_IPP==_IPP_V8) || \
      (_IPP==_IPP_P8) || \
      (_IPP>=_IPP_G9) || \
      (_IPP==_IPP_S8) || \
      (_IPP32E==_IPP32E_M7) || \
      (_IPP32E==_IPP32E_U8) || \
      (_IPP32E==_IPP32E_Y8) || \
      (_IPP32E>=_IPP32E_E9) || \
      (_IPP32E==_IPP32E_N8)) || \
      defined(_USE_C_cpMulAdc_BNU_school_)
IPP_OWN_DEFN (BNU_CHUNK_T, cpMulAdc_BNU_school, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, cpSize nsA, const BNU_CHUNK_T* pB, cpSize nsB))
{
   const BNU_CHUNK_T* pa = (BNU_CHUNK_T*)pA;
   const BNU_CHUNK_T* pb = (BNU_CHUNK_T*)pB;
   BNU_CHUNK_T* pr = (BNU_CHUNK_T*)pR;

   BNU_CHUNK_T extension = 0;
   cpSize i, j;

   ZEXPAND_BNU(pr, 0, nsA+nsB);

   for(i=0; i<nsB; i++ ) {
      BNU_CHUNK_T b = pb[i];

      for(j=0, extension=0; j<nsA; j++ ) {
         BNU_CHUNK_T rH, rL;

         MUL_AB(rH, rL, pa[j], b);
         ADD_ABC(extension, pr[i+j], pr[i+j], rL, extension);
         extension += rH;
      }
      pr[i+j] = extension;
   }
   return extension;
}
#endif
