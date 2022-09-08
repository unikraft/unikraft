/*******************************************************************************
* Copyright 2018-2021 Intel Corporation
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
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     GF(p^d) methods
//
*/
#include "owncp.h"

#include "pcpgfpxstuff.h"
#include "pcpgfpxmethod_com.h"


/*
// return common polynomi alarith methods
*/
static gsModMethod* gsPolyArith(void)
{
   static gsModMethod m = {
      cpGFpxEncode_com,
      cpGFpxDecode_com,
      cpGFpxMul_com,
      cpGFpxSqr_com,
      NULL,
      cpGFpxAdd_com,
      cpGFpxSub_com,
      cpGFpxNeg_com,
      cpGFpxDiv2_com,
      cpGFpxMul2_com,
      cpGFpxMul3_com,
      //cpGFpxInv
   };
   return &m;
}

/*F*
// Name: ippsGFpxMethod_binom2
//
// Purpose: Returns a reference to the implementation of arithmetic operations over GF(pd).
//
// Returns:          pointer to a structure containing
//                   an implementation of arithmetic operations over GF(pd)
//                   g(x) = x^d + x(d - 1) a_(d-1) + x^(d - 2)a_(d - 2) + ... + x1a1 + a0, ai from GF(p)
//
//
*F*/

IPPFUN( const IppsGFpMethod*, ippsGFpxMethod_com, (void) )
{
   static IppsGFpMethod method = {
      cpID_Poly,
      0,
      NULL,
      NULL
   };
   method.arith = gsPolyArith();
   return &method;
}
