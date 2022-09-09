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
//     EC over Prime Finite Field (initialization)
// 
//  Contents:
//        ippsECCPInit()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpeccp.h"

/*F*
//    Name: ippsECCPInit
//
// Purpose: Init ECC context.
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pEC
//
//    ippStsSizeErr              2>feBitSize
//                               feBitSize>EC_GFP_MAXBITSIZE
//    ippStsNoErr                no errors
//
// Parameters:
//    feBitSize   size of field element (bits)
//    pEC        pointer to the ECC context
//
*F*/
IPPFUN(IppStatus, ippsECCPInit, (int feBitSize, IppsECCPState* pEC))
{
   /* test pEC pointer */
   IPP_BAD_PTR1_RET(pEC);

   /* test size of field element */
   IPP_BADARG_RET((2>feBitSize || feBitSize>EC_GFP_MAXBITSIZE), ippStsSizeErr);

   {
      /* size of GF context */
      //int gfCtxSize = cpGFpGetSize(feBitSize);
      int gfCtxSize = cpGFpGetSize(feBitSize, feBitSize+BITSIZE(BNU_CHUNK_T), GFP_POOL_SIZE);
      /* size of EC context */
      int ecCtxSize = cpGFpECGetSize(1, feBitSize);

      IppsGFpState* pGF = (IppsGFpState*)((Ipp8u*)pEC+ecCtxSize);
      BNU_CHUNK_T* pScratchBuffer = (BNU_CHUNK_T*)IPP_ALIGNED_PTR((Ipp8u*)pGF+gfCtxSize, CACHE_LINE_SIZE);

      /* set up contexts */
      IppStatus sts;
      do {
         sts = cpGFpInitGFp(feBitSize, pGF);
         if(ippStsNoErr!=sts) break;
         sts = ippsGFpECInit(pGF, NULL, NULL, pEC);
      } while (0);

      /* save scratch buffer pointer */
      ECP_SBUFFER(pEC) = pScratchBuffer;

      return sts;
   }
}
