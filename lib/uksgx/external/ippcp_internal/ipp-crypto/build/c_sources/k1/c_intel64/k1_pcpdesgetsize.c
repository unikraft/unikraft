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
//        ippsDESGetSize()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdes.h"
#include "pcptool.h"

/*F*
//    Name: ippsDESGetSize
//
// Purpose: Returns size of DES spec (bytes).
//
// Returns:                Reason:
//    ippStsNullPtrErr        pSize == NULL
//    ippStsNoErr             no errors
//
// Parameters:
//    pSize       pointer DES spec size
//
*F*/
IPPFUN(IppStatus, ippsDESGetSize, (int* pSize))
{
   /* test size's pointer */
   IPP_BAD_PTR1_RET(pSize);

   *pSize = sizeof(IppsDESSpec);

   return ippStsNoErr;
}
