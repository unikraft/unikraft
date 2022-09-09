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
//     GF(p^d) methods
//
*/
#include "owncp.h"

#include "pcpgfpxstuff.h"
#include "pcpgfpxmethod_com.h"


IPP_OWN_DEFN (BNU_CHUNK_T*, cpGFpxAdd_com, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, gsEngine* pGFEx))
{
   gsEngine* pBasicGFE = cpGFpBasic(pGFEx);
   mod_add addF = GFP_METHOD(pBasicGFE)->add;
   int basicElemLen = GFP_FELEN(pBasicGFE);
   int basicDeg = cpGFpBasicDegreeExtension(pGFEx);

   BNU_CHUNK_T* pTmp = pR;
   int deg;
   for(deg=0; deg<basicDeg; deg++) {
      addF(pTmp, pA, pB, pBasicGFE);
      pTmp += basicElemLen;
      pA += basicElemLen;
      pB += basicElemLen;
   }
   return pR;
}
