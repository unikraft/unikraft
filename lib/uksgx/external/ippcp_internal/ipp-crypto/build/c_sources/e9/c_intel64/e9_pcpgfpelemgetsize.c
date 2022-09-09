/*******************************************************************************
* Copyright 2018-2021 Intel Corporation
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
//     Operations over GF(p).
//
//     Context:
//        ippsGFpElementGetSize()
//
*/
#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcptool.h"

//tbcd: temporary excluded: #include <assert.h>

/*F*
// Name: ippsGFpElementGetSize
//
// Purpose: Gets the size of the context for an element of the finite field.
//
// Returns:                   Reason:
//    ippStsNullPtrErr          pGFp == NULL
//                              pBufferSize == NULL
//    ippStsContextMatchErr     incorrect pGFp's context id
//    ippStsNoErr              no error
//
// Parameters:
//    nExponents      Number of exponents.
//    ExpBitSize      Maximum bit size of the exponents.
//    pGFp            Pointer to the context of the finite field.
//    pBufferSize     Pointer to the calculated buffer size in bytes.
//
*F*/

IPPFUN(IppStatus, ippsGFpElementGetSize,(const IppsGFpState* pGFp, int* pElementSize))
{
   IPP_BAD_PTR2_RET(pElementSize, pGFp);
   IPP_BADARG_RET( !GFP_VALID_ID(pGFp), ippStsContextMatchErr );

   *pElementSize = (Ipp32s)sizeof(IppsGFpElement)
                  +GFP_FELEN(GFP_PMA(pGFp))*(Ipp32s)sizeof(BNU_CHUNK_T);
   return ippStsNoErr;
}
