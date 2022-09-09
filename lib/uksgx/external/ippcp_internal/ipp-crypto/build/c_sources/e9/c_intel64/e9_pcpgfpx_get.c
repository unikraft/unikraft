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
//        cpGFpxGet()
//
*/

#include "owncp.h"
#include "pcpbnumisc.h"
#include "pcpgfpxstuff.h"
#include "gsscramble.h"

IPP_OWN_DEFN (BNU_CHUNK_T*, cpGFpxGet, (BNU_CHUNK_T* pDataA, int nsA, const BNU_CHUNK_T* pE, gsModEngine* pGFEx))
{
   cpGFpElementPad(pDataA, nsA, 0);

   if( GFP_IS_BASIC(pGFEx) )
      return cpGFpGet(pDataA, nsA, pE, pGFEx);

   else {
      gsModEngine* pBasicGFE = cpGFpBasic(pGFEx);
      int basicElemLen = GFP_FELEN(pBasicGFE);

      BNU_CHUNK_T* pTmp = pDataA;
      int basicDeg = cpGFpBasicDegreeExtension(pGFEx);

      int deg;
      for(deg=0; deg<basicDeg && nsA>0; deg++) {
         int pieceA = IPP_MIN(nsA, basicElemLen);

         cpGFpGet(pTmp, pieceA, pE, pBasicGFE);
         pE   += basicElemLen;
         pTmp += pieceA;
         nsA -= pieceA;
      }

      return pDataA;
   }
}
