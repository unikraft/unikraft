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
//     EC (prime) Point
// 
//  Contents:
//        ippsECCPPointInit()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpeccp.h"

/*F*
//    Name: ippsECCPPointInit
//
// Purpose: Init EC Point context.
//
// Returns:                Reason:
//    ippStsNullPtrErr        NULL == pPoint
//    ippStsSizeErr           2>feBitSize
//    ippStsNoErr             no errors
//
// Parameters:
//    feBitSize   size of field element (bits)
//    pECC        pointer to ECC context
//
*F*/
IPPFUN(IppStatus, ippsECCPPointInit, (int feBitSize, IppsECCPPointState* pPoint))
{
   /* test pEC pointer */
   IPP_BAD_PTR1_RET(pPoint);

   /* test size of field element */
   IPP_BADARG_RET((2>feBitSize), ippStsSizeErr);

   {
      int elemLen = BITS_BNU_CHUNK(feBitSize);
      Ipp8u* ptr = (Ipp8u*)pPoint;

      ECP_POINT_SET_ID(pPoint);
      ECP_POINT_FLAGS(pPoint) = 0;
      ECP_POINT_FELEN(pPoint) = elemLen;
      ptr += sizeof(IppsGFpECPoint);
      ECP_POINT_DATA(pPoint) = (BNU_CHUNK_T*)(ptr);

      gfec_SetPointAtInfinity(pPoint);
      return ippStsNoErr;
   }
}
