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
//        ippsGFpGetElementOctString()
// 
*/
#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcptool.h"


/*F*
// Name: ippsGFpGetElementOctString
//
// Purpose: Get GF Element to the octet string
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pGFp
//                               NULL == pStr
//                               NULL == pA
//
//    ippStsContextMatchErr      invalid pGFp->idCtx
//                               invalid pA->idCtx
//                               invalid pR->idCtx
//
//    ippStsOutOfRangeErr        GFPE_ROOM() != GFP_FELEN()
//
//    ippStsSizeErr              !(0<lenA && lenA>=GFP_FELEN32(pGFE))
//
//    ippStsNoErr                no error
//
// Parameters:
//    pA       Pointer to the context of the finite field element.
//    pStr     Pointer to the octet string.
//    strSize  Size of the octet string buffer in bytes.
//    pGFp     Pointer to the context of the finite field.
//
*F*/

IPPFUN(IppStatus, ippsGFpGetElementOctString,(const IppsGFpElement* pA, Ipp8u* pStr, int strSize, IppsGFpState* pGFp))
{
   IPP_BAD_PTR3_RET(pStr, pA, pGFp);
   IPP_BADARG_RET( !GFP_VALID_ID(pGFp), ippStsContextMatchErr );
   IPP_BADARG_RET( !GFPE_VALID_ID(pA), ippStsContextMatchErr );
   IPP_BADARG_RET( 0>=strSize, ippStsSizeErr );
   {
      gsModEngine* pGFE = GFP_PMA(pGFp);
      IPP_BADARG_RET( GFPE_ROOM(pA)!=GFP_FELEN(pGFE), ippStsOutOfRangeErr);
      {
         gsModEngine* pBasicGFE = cpGFpBasic(pGFE);
         int basicDeg = cpGFpBasicDegreeExtension(pGFE);
         int basicElemLen = GFP_FELEN(pBasicGFE);
         int basicSize = BITS2WORD8_SIZE(BITSIZE_BNU(GFP_MODULUS(pBasicGFE),GFP_FELEN(pBasicGFE)));

         BNU_CHUNK_T* pDataElm = GFPE_DATA(pA);
         int deg, error;
         for(deg=0, error=0; deg<basicDeg && !error; deg++) {
            int size = IPP_MIN(strSize, basicSize);
            error = (NULL == cpGFpGetOctString(pStr, size, pDataElm, pBasicGFE));

            pDataElm += basicElemLen;
            pStr += size;
            strSize -= size;
         }

         return error ? ippStsSizeErr : ippStsNoErr;
      }
   }
}
