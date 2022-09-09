/*******************************************************************************
* Copyright 2017-2021 Intel Corporation
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
//     Cryptography Primitive. Modular Arithmetic Engine. General Functionality
// 
//  Contents:
//        gsMontFactor()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbnumisc.h"
#include "pcpbnuarith.h"
#include "gsmodstuff.h"
#include "pcptool.h"

/*
// montfomery factor k0 = -((modulus^-1 mod B) %B)
*/
IPP_OWN_DEFN (BNU_CHUNK_T, gsMontFactor, (BNU_CHUNK_T m0))
{
   BNU_CHUNK_T y = 1;
   BNU_CHUNK_T x = 2;
   BNU_CHUNK_T mask = 2*x-1;

   int i;
   for(i=2; i<=BNU_CHUNK_BITS; i++, x<<=1) {
      BNU_CHUNK_T rH, rL;
      MUL_AB(rH, rL, m0, y);
      if( x < (rL & mask) ) /* x < ((m0*y) mod (2*x)) */
         y+=x;
      mask += mask + 1;
   }
   return 0-y;
}
