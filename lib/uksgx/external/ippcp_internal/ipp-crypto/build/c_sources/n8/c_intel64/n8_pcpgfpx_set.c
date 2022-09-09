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
//        cpGFpxSet()
//
*/

#include "owncp.h"
#include "pcpbnumisc.h"
#include "pcpgfpxstuff.h"
#include "gsscramble.h"

IPP_OWN_DEFN (BNU_CHUNK_T*, cpGFpxSet, (BNU_CHUNK_T* pE, const BNU_CHUNK_T* pDataA, int nsA, gsModEngine* pGFEx))
{
   if( GFP_IS_BASIC(pGFEx) )
      return cpGFpSet(pE, pDataA, nsA, pGFEx);

   else {
      gsModEngine* pBasicGFE = cpGFpBasic(pGFEx);
      int basicElemLen = GFP_FELEN(pBasicGFE);

      BNU_CHUNK_T* pTmpE = pE;
      int basicDeg = cpGFpBasicDegreeExtension(pGFEx);

      int deg, error;
      for(deg=0, error=0; deg<basicDeg && !error; deg++) {
         int pieceA = IPP_MIN(nsA, basicElemLen);

         error = NULL == cpGFpSet(pTmpE, pDataA, pieceA, pBasicGFE);
         pTmpE   += basicElemLen;
         pDataA += pieceA;
         nsA -= pieceA;
      }

      return (deg<basicDeg)? NULL : pE;
   }
}
