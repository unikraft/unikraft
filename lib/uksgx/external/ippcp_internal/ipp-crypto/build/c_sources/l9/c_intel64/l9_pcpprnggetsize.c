/*******************************************************************************
* Copyright 2004-2021 Intel Corporation
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
//     PRNG Functions
// 
//  Contents:
//        ippsPRNGGetSize()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpprng.h"
#include "pcphash.h"
#include "pcptool.h"

/*F*
//    Name: ippsPRNGGetSize
//
// Purpose: Returns size of PRNG context (bytes).
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pSize
//
//    ippStsNoErr                no error
//
// Parameters:
//    pSize       pointer to the size of internal context
*F*/
IPPFUN(IppStatus, ippsPRNGGetSize, (int* pSize))
{
   IPP_BAD_PTR1_RET(pSize);

   *pSize = sizeof(IppsPRNGState);

   return ippStsNoErr;
}
