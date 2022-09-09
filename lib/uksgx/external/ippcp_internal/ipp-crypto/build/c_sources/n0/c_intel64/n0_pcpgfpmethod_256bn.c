/*******************************************************************************
* Copyright 2016-2021 Intel Corporation
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
//     GF(p) methods
//
*/

#include "pcpgfpmethod_256.h"

/*F*
// Name: ippsGFpMethod_p256bn
//
// Purpose: Returns a reference to an implementation of
//          arithmetic operations over GF(q).
//
// Returns:  Pointer to a structure containing an implementation of arithmetic
//           operations over GF(q). q =
//           0xFFFFFFFFFFFCF0CD46E5F25EEE71A49F0CDC65FB12980A82D3292DDBAED33013
*F*/

IPPFUN( const IppsGFpMethod*, ippsGFpMethod_p256bn, (void) )
{
   static IppsGFpMethod method = {
      cpID_Prime,
      256,
      tpmBN_p256p_p,
      NULL
   };

   #if(_IPP32E >= _IPP32E_M7)
   method.arith = gsArithGF_p256();
   #else
   method.arith = gsArithGFp();
   #endif
   return &method;
}
