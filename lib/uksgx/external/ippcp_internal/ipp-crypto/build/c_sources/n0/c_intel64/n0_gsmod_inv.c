/*******************************************************************************
* Copyright 2017-2021 Intel Corporation
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
//     Cryptography Primitive. Modular Arithmetic Engine. General Functionality
// 
//  Contents:
//        gs_inv()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbnumisc.h"
#include "pcpbnuarith.h"
#include "gsmodstuff.h"

//tbcd: temporary excluded: #include <assert.h>

/*
// returns r =1/a
//    a in desidue domain
//    r in desidue domain
*/
IPP_OWN_DEFN (BNU_CHUNK_T*, gs_inv, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, gsModEngine* pME, alm_inv alm_inversion))
{
   int k = alm_inversion(pr, pa, pME);

   if(0==k)
      return NULL;

   {
      int mLen = MOD_LEN(pME);
      int m = mLen*BNU_CHUNK_BITS;
      mod_mul mon_mul = MOD_METHOD(pME)->mul;

      BNU_CHUNK_T* t = gsModPoolAlloc(pME, 1);
      //tbcd: temporary excluded: assert(NULL!=t);

      if(k>m) {
         ZEXPAND_BNU(t, 0, mLen);
         t[0] = 1;
         mon_mul(pr, pr, t, pME);
         k -= m;
      }
      ZEXPAND_BNU(t, 0, mLen);
      SET_BIT(t, m-k); /* t = 2^(m-k) */
      mon_mul(pr, pr, t, pME);

      gsModPoolFree(pME, 1);

      return pr;
   }
}
