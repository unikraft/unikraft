/*******************************************************************************
* Copyright 2005-2021 Intel Corporation
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
//     DL operation results
// 
//  Contents:
//     ippsDLGetResultString()
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"


IPPFUN( const char*, ippsDLGetResultString, (IppDLResult code))
{
   switch(code) {
      case ippDLValid:             return "Validation pass successfully";

      case ippDLBaseIsEven:        return "Base is even";
      case ippDLOrderIsEven:       return "Order is even";
      case ippDLInvalidBaseRange:  return "Invalid Base (P) range";
      case ippDLInvalidOrderRange: return "Invalid Order (R) range";
      case ippDLCompositeBase:     return "Composite Base (P)";
      case ippDLCompositeOrder:    return "Composite Order(R)";
      case ippDLInvalidCofactor:   return "R doesn't divide (P-1)";
      case ippDLInvalidGenerator:  return "1 != G^R (mod P)";

      case ippDLInvalidPrivateKey: return "Invalid Private Key";
      case ippDLInvalidPublicKey:  return "Invalid Public Key";
      case ippDLInvalidKeyPair:    return "Invalid Key Pair";

      case ippDLInvalidSignature:  return "Invalid Signature";

      default:                      return "Unknown DL result";
   }
}
