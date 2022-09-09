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
//     Operations over GF(p).
// 
//     Context:
//        cpGFEqnr()
// 
// 
*/
#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcptool.h"

//tbcd: temporary excluded: #include <assert.h>

IPP_OWN_DEFN (void, cpGFEqnr, (gsModEngine* pGFE))
{
   BNU_CHUNK_T* pQnr = GFP_QNR(pGFE);

   int elemLen = GFP_FELEN(pGFE);
   BNU_CHUNK_T* e = cpGFpGetPool(3, pGFE);
   BNU_CHUNK_T* t = e+elemLen;
   BNU_CHUNK_T* p1 = t+elemLen;
   //tbcd: temporary excluded: assert(NULL!=e);

   cpGFpElementCopyPad(p1, elemLen, GFP_MNT_R(pGFE), elemLen);

   /* (modulus-1)/2 */
   cpLSR_BNU(e, GFP_MODULUS(pGFE), elemLen, 1);

   /* find a non-square g, where g^{(modulus-1)/2} = -1 */
   cpGFpElementCopy(pQnr, p1, elemLen);
   do {
      cpGFpAdd(pQnr, pQnr, p1, pGFE);
      cpGFpExp(t, pQnr, e, elemLen, pGFE);
      cpGFpNeg(t, t, pGFE);
   } while( !GFP_EQ(p1, t, elemLen) );

   cpGFpReleasePool(3, pGFE);
}
