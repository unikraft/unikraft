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
//        cpGFpInv
//
*/
#include "owncp.h"

#include "pcpbn.h"
#include "pcpgfpstuff.h"

IPP_OWN_DEFN (BNU_CHUNK_T*, cpGFpInv, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsModEngine* pGFE))
{
   GFP_METHOD(pGFE)->decode(pR, pA, pGFE);
   /* gs_mont_inv(pR, pR, pGFE, alm_mont_inv); */
   gs_mont_inv(pR, pR, pGFE, alm_mont_inv_ct); /* switch on CTE internal function */
   return pR;
}
