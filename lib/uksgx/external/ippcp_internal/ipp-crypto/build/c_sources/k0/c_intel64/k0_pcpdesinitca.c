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
//     Set Round Keys for DES/TDES
// 
//  Contents:
//        ippsDESInit()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdes.h"
#include "pcptool.h"

/*F*
//    Name: ippsDESInit
//
// Purpose: Init DES spec for future usage.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//                            pKey == NULL
//    ippStsNoErr             no errors
//
// Parameters:
//    pKey        pointer to security key
//    pCtx        pointer DES spec
//
*F*/
IPPFUN(IppStatus, ippsDESInit,(const Ipp8u* pKey, IppsDESSpec* pCtx))
{
   /* test key's and spec's pointers */
   IPP_BAD_PTR2_RET(pKey, pCtx);

   /* init DES spec */
   DES_SET_ID(pCtx);

   /* set round keys */
   SetKey_DES(pKey, pCtx);

   return ippStsNoErr;
}
