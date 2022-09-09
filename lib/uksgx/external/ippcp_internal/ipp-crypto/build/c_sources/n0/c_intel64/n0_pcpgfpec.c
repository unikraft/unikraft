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
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     EC over GF(p^m) definitinons
// 
//     Context:
//        cpGFpECGetSize()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "pcpeccp.h"


IPP_OWN_DEFN (int, cpGFpECGetSize, (int basicDeg, int basicElmBitSize))
{
   int ctxSize = 0;
   int elemLen = basicDeg*BITS_BNU_CHUNK(basicElmBitSize);

   int maxOrderBits = 1+ basicDeg*basicElmBitSize;
   #if defined(_LEGACY_ECCP_SUPPORT_)
   int maxOrderLen = BITS_BNU_CHUNK(maxOrderBits);
   #endif

   int modEngineCtxSize;
   if(ippStsNoErr==gsModEngineGetSize(maxOrderBits, MONT_DEFAULT_POOL_LENGTH, &modEngineCtxSize)) {

      ctxSize = (Ipp32s)sizeof(IppsGFpECState)
               +elemLen*(Ipp32s)sizeof(BNU_CHUNK_T)    /* EC coeff    A */
               +elemLen*(Ipp32s)sizeof(BNU_CHUNK_T)    /* EC coeff    B */
               +elemLen*(Ipp32s)sizeof(BNU_CHUNK_T)    /* generator G.x */
               +elemLen*(Ipp32s)sizeof(BNU_CHUNK_T)    /* generator G.y */
               +elemLen*(Ipp32s)sizeof(BNU_CHUNK_T)    /* generator G.z */
               +modEngineCtxSize               /* mont engine (R) */
               +elemLen*(Ipp32s)sizeof(BNU_CHUNK_T)    /* cofactor */
               #if defined(_LEGACY_ECCP_SUPPORT_)
               +2*elemLen*3*(Ipp32s)sizeof(BNU_CHUNK_T)    /* regular and ephemeral public  keys */
               +2*maxOrderLen*(Ipp32s)sizeof(BNU_CHUNK_T)  /* regular and ephemeral private keys */
               #endif
               +elemLen*(Ipp32s)sizeof(BNU_CHUNK_T)*3*EC_POOL_SIZE;
   }
   return ctxSize;
}
