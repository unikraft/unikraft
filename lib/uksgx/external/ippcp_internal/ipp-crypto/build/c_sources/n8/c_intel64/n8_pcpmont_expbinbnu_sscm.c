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
//     Modular Exponentiation (binary version)
// 
//  Contents:
//        cpMontExpBin_BNU_sscm()
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpmontgomery.h"
#include "pcpmask_ct.h"

//tbcd: temporary excluded: #include <assert.h>


#if defined(_USE_IPP_OWN_CBA_MITIGATION_)
/*
// The reason was to mitigate "cache monitoring" attack on RSA
//
// This is improved version of modular exponentiation.
// Current version provide both either mitigation and perrformance.
// This version in comparison with previous (Intel(R) Integrated Performance Primitives (Intel(R) IPP) 4.1.3) one ~30-40% faster,
// i.e the the performance stayed as was for pre-mitigated version
//
*/

/*F*
// Name: cpMontExpBin_BNU_sscm
//
// Purpose: computes the Montgomery exponentiation with exponent
//          BNU_CHUNK_T *dataE to the given big number integer of Montgomery form
//          BNU_CHUNK_T *dataX with respect to the modulus gsModEngine *pModEngine.
//
// Returns:
//      Length of modulus
//
//
// Parameters:
//      dataX        big number integer of Montgomery form within the
//                      range [0,m-1]
//      dataE        big number exponent
//      pMont        Montgomery modulus of IppsMontState.
/       dataY        the Montgomery exponentation result.
//
*F*/
IPP_OWN_DEFN (cpSize, cpMontExpBin_BNU_sscm, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize nsE, gsModEngine* pMont))
{
   cpSize nsM = MOD_LEN(pMont);

   /*
   // test for special cases:
   //    x^0 = 1
   //    0^e = 0
   */
   if( cpIsGFpElemEquChunk_ct(dataE, nsE, 0) ) {
      COPY_BNU(dataY, MOD_MNT_R(pMont), nsM);
   }
   else if( cpIsGFpElemEquChunk_ct(dataX, nsX, 0) ) {
      ZEXPAND_BNU(dataY, 0, nsM);
   }

   /* general case */
   else {
      /* Montgomery engine buffers */
      const int usedPoolLen = 2;
      BNU_CHUNK_T* dataT = gsModPoolAlloc(pMont, usedPoolLen);
      BNU_CHUNK_T* sscmB = dataT + nsM;
      //tbcd: temporary excluded: assert(NULL!=dataT);

      /* mont(1) */
      BNU_CHUNK_T* pR = MOD_MNT_R(pMont);

      /* copy base */
      ZEXPAND_COPY_BNU(dataT, nsM, dataX, nsX);
      /* init result, Y=1 */
      COPY_BNU(dataY, pR, nsM);

      /* execute bits of E */
      for(; nsE>0; nsE--) {
         BNU_CHUNK_T eValue = dataE[nsE-1];

         int n;
         for(n=BNU_CHUNK_BITS; n>0; n--) {
            /* sscmB = ( msb(eValue) )? X : mont(1) */
            BNU_CHUNK_T mask = cpIsMsb_ct(eValue);
            eValue <<= 1;
            cpMaskedCopyBNU_ct(sscmB, mask, dataT, pR, nsM);

            /* squaring Y = Y^2 */
            MOD_METHOD(pMont)->sqr(dataY, dataY, pMont);
            /* and multiplication: Y = Y * sscmB */
            MOD_METHOD(pMont)->mul(dataY, dataY, sscmB, pMont);
         }
      }

      gsModPoolFree(pMont, usedPoolLen);
   }

   return nsM;
}

#endif /* _USE_IPP_OWN_CBA_MITIGATION_ */
