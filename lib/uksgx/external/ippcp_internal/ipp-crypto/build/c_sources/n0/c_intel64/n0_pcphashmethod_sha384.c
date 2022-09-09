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
// 
//  Purpose:
//     Cryptography Primitive.
//     SHA512 message digest
// 
//  Contents:
//        ippsHashMethod_SHA384()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"
#include "pcpsha512stuff.h"

/*F*
//    Name: ippsHashMethod_SHA384
//
// Purpose: Return SHA384 method.
//
// Returns:
//          Pointer to SHA384 hash-method.
//
*F*/

IPPFUN( const IppsHashMethod*, ippsHashMethod_SHA384, (void) )
{
   static IppsHashMethod method = {
      ippHashAlg_SHA384,
      IPP_SHA384_DIGEST_BITSIZE/8,
      MBS_SHA512,
      MLR_SHA512,
      0,
      0,
      0,
      0
   };

   method.hashInit   = sha512_384_hashInit;
   method.hashUpdate = sha512_hashUpdate;
   method.hashOctStr = sha512_384_hashOctString;
   method.msgLenRep  = sha512_msgRep;

   return &method;
}
