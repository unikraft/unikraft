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
//        cpMontExpBin_BNU()
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpmontgomery.h"

//tbcd: temporary excluded: #include <assert.h>

/*F*
// Name: cpMontExpBin_BNU
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
//      pModEngine   Montgomery modulus of IppsMontState.
/       dataY        the Montgomery exponentation result.
//
// Notes: IppsBigNumState *r should possess enough memory space as to hold the result
//        of the operation. i.e. both pointers r->d and r->buffer should possess
//        no less than (m->n->length) number of 32-bit words.
*F*/

IPP_OWN_DEFN (cpSize, cpMontExpBin_BNU, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize nsE, gsModEngine* pModEngine))
{
   cpSize nsM = MOD_LEN( pModEngine );

   /*
   // test for special cases:
   //    x^0 = 1
   //    0^e = 0
   */
   if( cpEqu_BNU_CHUNK(dataE, nsE, 0) ) {
      COPY_BNU(dataY, MOD_MNT_R( pModEngine ), nsM);
   }
   else if( cpEqu_BNU_CHUNK(dataX, nsX, 0) ) {
      ZEXPAND_BNU(dataY, 0, nsM);
   }
   
   /* general case */
   else {
      /* Montgomery engine buffers */
      const int usedPoolLen = 1;
      BNU_CHUNK_T* dataT = gsModPoolAlloc(pModEngine, usedPoolLen);
      //tbcd: temporary excluded: assert(NULL!=dataT);

      {
         /* execute most significant part pE */
         BNU_CHUNK_T eValue = dataE[nsE-1];
         int n = cpNLZ_BNU(eValue)+1;

         /* expand base and init result */
         ZEXPAND_COPY_BNU(dataT, nsM, dataX, nsX);
         COPY_BNU(dataY, dataT, nsM);

         eValue <<= n;
         for(; n<BNU_CHUNK_BITS; n++, eValue<<=1) {
            /* squaring R = R*R mod Modulus */
            MOD_METHOD( pModEngine )->sqr(dataY, dataY, pModEngine);

            /* and multiply R = R*X mod Modulus */
            if(eValue & ((BNU_CHUNK_T)1<<(BNU_CHUNK_BITS-1)))
               MOD_METHOD( pModEngine )->mul(dataY, dataY, dataT, pModEngine);
         }

         /* execute rest bits of E */
         for(--nsE; nsE>0; nsE--) {
            eValue = dataE[nsE-1];

            for(n=0; n<BNU_CHUNK_BITS; n++, eValue<<=1) {
               /* squaring: R = R*R mod Modulus */
               MOD_METHOD( pModEngine )->sqr(dataY, dataY, pModEngine);

               if(eValue & ((BNU_CHUNK_T)1<<(BNU_CHUNK_BITS-1)))
                  MOD_METHOD( pModEngine )->mul(dataY, dataY, dataT, pModEngine);
            }
         }
      }

      gsModPoolFree(pModEngine, usedPoolLen);
   }

   return nsM;
}
