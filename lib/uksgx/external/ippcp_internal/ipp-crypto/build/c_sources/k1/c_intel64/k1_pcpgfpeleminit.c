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
//        ippsGFpElementInit()
//
*/
#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcptool.h"

//tbcd: temporary excluded: #include <assert.h>

/*F*
// Name: ippsGFpElementInit
//
// Purpose: Initializes the context of an element of the finite field.
//
// Returns:                   Reason:
//    ippStsNullPtrErr          pGFp == NULL
//                              pR == NULL
//                              pA && nsA>0 == NULL
//
//    ippStsContextMatchErr     invalid pGFp->idCtx
//                              invalid pR->idCtx
//
//    ippStsSizeErr             pA && !(0<=lenA && lenA<GFP_FELEN32(GFP_PMA(pGFp)))
//
//    ippStsOutOfRangeErr       GFPE_ROOM(pR)!=GFP_FELEN(GFP_PMA(pGFp)
//                              BNU representation of pA[i]..pA[i+GFP_FELEN32()-1] >= modulus
//
//    ippStsNoErr               no error
//
// Parameters:
//    pA      Pointer to the data array storing the finite field element.
//    lenA    Length of the element.
//    pR      Pointer to the context of the finite field element being initialized.
//    pGFp    Pointer to the context of the finite field.
//
*F*/

IPPFUN(IppStatus, ippsGFpElementInit,(const Ipp32u* pA, int lenA, IppsGFpElement* pR, IppsGFpState* pGFp))
{
   IPP_BAD_PTR2_RET(pR, pGFp);
   IPP_BADARG_RET( !GFP_VALID_ID(pGFp), ippStsContextMatchErr );

   IPP_BADARG_RET(0>lenA, ippStsSizeErr);

   {
      int elemLen = GFP_FELEN(GFP_PMA(pGFp));

      Ipp8u* ptr = (Ipp8u*)pR;
      ptr += sizeof(IppsGFpElement);
      cpGFpElementConstruct(pR, (BNU_CHUNK_T*)ptr, elemLen);
      return ippsGFpSetElement(pA, lenA, pR, pGFp);
   }
}
