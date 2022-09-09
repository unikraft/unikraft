/*******************************************************************************
* Copyright 2013-2021 Intel Corporation
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
//     Cryptography Primitive.
//     Digesting message according to SM3
// 
//  Contents:
//        ippsHashMethod_SM3()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"
#include "pcpsm3stuff.h"

/*F*
//    Name: ippsHashMethod_SM3
//
// Purpose: Return SM3 method.
//
// Returns:
//          Pointer to SM3 hash-method.
//
*F*/
IPPFUN( const IppsHashMethod*, ippsHashMethod_SM3, (void) )
{
   static IppsHashMethod method = {
      ippHashAlg_SM3,
      IPP_SM3_DIGEST_BITSIZE/8,
      MBS_SM3,
      MLR_SM3,
      0,
      0,
      0,
      0
   };

   method.hashInit   = sm3_hashInit;
   method.hashUpdate = sm3_hashUpdate;
   method.hashOctStr = sm3_hashOctString;
   method.msgLenRep  = sm3_msgRep;

   return &method;
}
