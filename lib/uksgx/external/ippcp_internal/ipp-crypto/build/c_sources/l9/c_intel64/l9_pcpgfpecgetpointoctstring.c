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
//
//  Purpose:
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     EC over GF(p) Operations
//
//     Context:
//        ippsGFpECGetPointOctString()
//
*/
#include "pcpgfpecessm2.h"
#include "pcpgfpecstuff.h"

/*F*
//    Name: ippsGFpECGetPointOctString
//
// Purpose: Converts a point on EC into x||y octstring
//
// Returns:                   Reason:
//    ippStsNullPtrErr           pPoint == NULL / pEC == NULL / pStr == NULL
//    ippStsContextMatchErr      pEC, pPoint invalid context
//    ippStsNotSupportedModeErr  pGFE->extdegree > 1
//    ippStsSizeErr              strLen is not equal to double GFp element
//    ippStsOutOfRangeErr        the point does not belong to the EC
//    ippStsPointAtInfinity      a point on infinity cannot be converted to a string
//    ippStsNoErr                no errors
//
// Parameters:
//    pStr     pointer to the string to read from
//    strLen   length of the string
//    pPoint   pointer to output point
//    pEC      EC ctx
//
*F*/
IPPFUN(IppStatus, ippsGFpECGetPointOctString, (const IppsGFpECPoint* pPoint,
   Ipp8u* pStr, int strLen, IppsGFpECState* pEC)) {
   IPP_BAD_PTR3_RET(pPoint, pEC, pStr);
   IPP_BADARG_RET(!ECP_POINT_VALID_ID(pPoint), ippStsContextMatchErr);
   IPP_BADARG_RET(!VALID_ECP_ID(pEC), ippStsContextMatchErr);

   {
      gsModEngine* pGFE = pEC->pGF->pGFE;
      IppsGFpInfo gfi;
      ippsGFpGetInfo(&gfi, pEC->pGF);

      {
         int elemLenBits = gfi.basicGFdegree * gfi.basicElmBitSize;
         int elemLenBytes = BITS2WORD8_SIZE(elemLenBits);
         int elemLenChunks = BITS_BNU_CHUNK(elemLenBits);
         IPP_BADARG_RET(strLen != elemLenBytes * 2, ippStsSizeErr);
         IPP_BADARG_RET(pPoint->elementSize != elemLenChunks, ippStsOutOfRangeErr);

         {
            int finitePoint;
            IppsGFpElement ptX, ptY;

            cpGFpElementConstruct(&ptX, cpGFpGetPool(1, pGFE), elemLenChunks);
            cpGFpElementConstruct(&ptY, cpGFpGetPool(1, pGFE), elemLenChunks);
            finitePoint = gfec_GetPoint(ptX.pData, ptY.pData, pPoint, pEC);
            if (finitePoint) {
               ippsGFpGetElementOctString(&ptX, pStr, elemLenBytes, pEC->pGF);
               pStr += elemLenBytes;
               ippsGFpGetElementOctString(&ptY, pStr, elemLenBytes, pEC->pGF);
            }

            cpGFpReleasePool(2, pGFE); /* release ptX and ptY from the pool */

            return finitePoint ? ippStsNoErr : ippStsPointAtInfinity;
         }
      }
   }
}
