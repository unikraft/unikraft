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
//     Internal operations over GF(p) extension.
// 
//     Context:
//        cpGFpxRand()
//
*/

#include "owncp.h"
#include "pcpbnumisc.h"
#include "pcpgfpxstuff.h"
#include "gsscramble.h"


IPP_OWN_DEFN (BNU_CHUNK_T*, cpGFpxRand, (BNU_CHUNK_T* pR, gsModEngine* pGFEx, IppBitSupplier rndFunc, void* pRndParam))
{
   if( GFP_IS_BASIC(pGFEx) )
      return cpGFpRand(pR, pGFEx, rndFunc, pRndParam);

   else {
      gsModEngine* pBasicGFE = cpGFpBasic(pGFEx);
      int basicElemLen = GFP_FELEN(pBasicGFE);
      int basicDeg = cpGFpBasicDegreeExtension(pGFEx);

      BNU_CHUNK_T* pTmp = pR;
      int deg;
      for(deg=0; deg<basicDeg; deg++) {
         if(NULL == cpGFpRand(pTmp, pBasicGFE, rndFunc, pRndParam))
            break;
         pTmp += basicElemLen;
      }
      return deg==basicDeg? pR : NULL;
   }
}
