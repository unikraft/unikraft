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
//     Internal operations over prime GF(p).
//
//     Context:
//        cpGFpExp
//
*/
#include "owncp.h"

#include "pcpbn.h"
#include "pcpgfpstuff.h"


IPP_OWN_DEFN (BNU_CHUNK_T*, cpGFpExp, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pE, int nsE, gsModEngine* pGFE))
{
   int elemLen = GFP_FELEN(pGFE);
   cpMontExpBin_BNU(pR, pA,cpFix_BNU(pA, elemLen), pE,cpFix_BNU(pE, nsE), pGFE);
   return pR;
}
