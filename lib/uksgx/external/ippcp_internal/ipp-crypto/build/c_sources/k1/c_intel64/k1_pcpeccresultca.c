/*******************************************************************************
* Copyright 2003-2021 Intel Corporation
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
//     ECC operation results
// 
//  Contents:
//     ippsECCGetResultString()
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"

/*F*
//    Name: ippsECCGetResultString
//
// Purpose: Returns the string corresponding to code 
//          that represents the result of validation
//
// Parameters:
//    code       The code of the validation result
//
*F*/

IPPFUN( const char*, ippsECCGetResultString, (IppECResult code))
{
   switch(code) {
   case ippECValid:           return "Validation pass successfully";
   case ippECCompositeBase:   return "Finite Field produced by Composite";
   case ippECComplicatedBase: return "Too much non-zero terms in the polynomial";
   case ippECIsZeroDiscriminant: return "Zero discriminamt";
   case ippECCompositeOrder:     return "Composite Base Point order";
   case ippECInvalidOrder:       return "Invalid Base Point order";
   case ippECIsWeakMOV:          return "EC cover by MOV Reduction Test";
   case ippECIsWeakSSSA:         return "EC cover by SS-SA Reduction Test";
   case ippECIsSupersingular:    return "EC is supersingular curve";
   case ippECInvalidPrivateKey:  return "Invalid Private Key";
   case ippECInvalidPublicKey:   return "Invalid Public Key";
   case ippECInvalidKeyPair:     return "Invalid Key Pair";
   case ippECPointOutOfGroup:    return "Point is out of group";
   case ippECPointIsAtInfinite:  return "Point at Infinity";
   case ippECPointIsNotValid:    return "Invalid EC Point";
   case ippECPointIsEqual:       return "Points are equal";
   case ippECPointIsNotEqual:    return "Points are different";
   case ippECInvalidSignature:   return "Invalid Signature";
   default:                      return "Unknown ECC result";
   }
}
