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
//               Intel(R) Integrated Performance Primitives
//                   Cryptographic Primitives (ippcp)
// 
//  Contents:
//     ippsGetSize_BN()
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcptool.h"


/*F*
//    Name: ippsGetSize_BN
//
// Purpose: Returns BigNum room.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pBN == NULL
//                            pSize == NULL
//    ippStsContextMatchErr   !BN_VALID_ID(pBN)
//    ippStsNoErr             no errors
//
// Parameters:
//    pBN      BigNum ctx
//    pSize    max BigNum length (in Ipp32u chunks)
//
*F*/
IPPFUN(IppStatus, ippsGetSize_BN, (const IppsBigNumState* pBN, int* pSize))
{
   IPP_BAD_PTR2_RET(pBN, pSize);

   IPP_BADARG_RET(!BN_VALID_ID(pBN), ippStsContextMatchErr);

   *pSize = BN_ROOM(pBN)*(Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u));

    return ippStsNoErr;
}
