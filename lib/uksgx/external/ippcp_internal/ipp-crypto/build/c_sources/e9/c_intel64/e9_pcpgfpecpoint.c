/*******************************************************************************
* Copyright 2010-2021 Intel Corporation
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
//        ippsGFpECPointGetSize()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "pcphash.h"
#include "pcphash_rmf.h"

/*F*
// Name: ippsGFpECPointGetSize
//
// Purpose: Gets the size of the IppsGFpECPoint context
//
// Returns:                   Reason:
//    ippStsNullPtrErr          pEC == NULL
//                              pSize == NULL
//
//    ippStsContextMatchErr     invalid pEC->idCtx
//
//    ippStsNoErr               no error
//
// Parameters:
//    pEC             Pointer to the context of the elliptic curve
//    pSize           Pointer to the buffer size
//
*F*/

IPPFUN(IppStatus, ippsGFpECPointGetSize,(const IppsGFpECState* pEC, int* pSize))
{
   IPP_BAD_PTR2_RET(pEC, pSize);
   IPP_BADARG_RET( !VALID_ECP_ID(pEC), ippStsContextMatchErr );

   {
      int elemLen = GFP_FELEN(GFP_PMA(ECP_GFP(pEC)));
      *pSize = (Ipp32s)sizeof(IppsGFpECPoint)
                     +elemLen*(Ipp32s)sizeof(BNU_CHUNK_T) /* X */
                     +elemLen*(Ipp32s)sizeof(BNU_CHUNK_T) /* Y */
                     +elemLen*(Ipp32s)sizeof(BNU_CHUNK_T);/* Z */
      return ippStsNoErr;
   }
}

/*F*
// Name: ippsGFpECPointInit
//
// Purpose: Initializes the context of a point on an elliptic curve
//
// Returns:                   Reason:
//    ippStsNullPtrErr          pPoint == NULL
//                              pEC == NULL
//
//    ippStsContextMatchErr     invalid pEC->idCtx
//
//    ippStsNoErr               no error
//
// Parameters:
//    pX, pY    Pointers to the X and Y coordinates of a point on the elliptic curve
//    pPoint    Pointer to the IppsGFpECPoint context being initialized
//    pEC       Pointer to the context of the elliptic curve
//
*F*/

IPPFUN(IppStatus, ippsGFpECPointInit,(const IppsGFpElement* pX, const IppsGFpElement* pY,
                                      IppsGFpECPoint* pPoint, IppsGFpECState* pEC))
{
   IPP_BAD_PTR2_RET(pPoint, pEC);
   IPP_BADARG_RET( !VALID_ECP_ID(pEC), ippStsContextMatchErr );

   {
      Ipp8u* ptr = (Ipp8u*)pPoint;
      int elemLen = GFP_FELEN(GFP_PMA(ECP_GFP(pEC)));

      ECP_POINT_SET_ID(pPoint);
      ECP_POINT_FLAGS(pPoint) = 0;
      ECP_POINT_FELEN(pPoint) = elemLen;
      ptr += sizeof(IppsGFpECPoint);
      ECP_POINT_DATA(pPoint) = (BNU_CHUNK_T*)(ptr);

      if(pX && pY)
         return ippsGFpECSetPoint(pX, pY, pPoint, pEC);
      else {
         gfec_SetPointAtInfinity(pPoint);
         return ippStsNoErr;
      }
   }
}
