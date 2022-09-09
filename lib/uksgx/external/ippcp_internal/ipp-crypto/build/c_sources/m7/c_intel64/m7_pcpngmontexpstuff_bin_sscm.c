/*******************************************************************************
* Copyright 2013-2021 Intel Corporation
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
//     Modular Exponentiation
*/

#include "owncp.h"
#include "pcpngmontexpstuff.h"
#include "gsscramble.h"
#include "pcpmask_ct.h"

IPP_OWN_DEFN (cpSize, gsMontExpBin_BNU_sscm, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize bitsizeE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
{

    cpSize nsM = MOD_LEN(pMont);
    cpSize nsE = BITS_BNU_CHUNK(bitsizeE);

    /*
    // test for special cases:
    //    x^0 = 1
    //    0^e = 0
    */
    if (cpEqu_BNU_CHUNK(dataE, nsE, 0)) {
        COPY_BNU(dataY, MOD_MNT_R(pMont), nsM);
    }
    else if (cpEqu_BNU_CHUNK(dataX, nsX, 0)) {
        ZEXPAND_BNU(dataY, 0, nsM);
    }

    /* general case */
    else {

      /* allocate buffers */
      BNU_CHUNK_T* dataT = pBuffer;
      BNU_CHUNK_T* sscmB = dataT + nsM;

      /* mont(1) */
      BNU_CHUNK_T* pR = MOD_MNT_R(pMont);

      /* copy and expand base to the modulus length */
       ZEXPAND_COPY_BNU(dataT, nsM, dataX, nsX);
       /* init result */
       COPY_BNU(dataY, MOD_MNT_R(pMont), nsM);

      /* execute bits of E */
      for (; nsE>0; nsE--) {
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
   }

   return nsM;
}

/*
// "safe" binary montgomery exponentiation
//
// - input/output are in Regular Domain
// - possible inplace mode
//
// scratch buffer structure:
//    dataT[nsM]
//     sscm[nsM]
*/
IPP_OWN_DEFN (cpSize, gsModExpBin_BNU_sscm, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize bitsizeE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
{
   cpSize nsM = MOD_LEN(pMont);

   /* copy and expand base to the modulus length */
   ZEXPAND_COPY_BNU(dataY, nsM, dataX, nsX);

   /* convert base to Montgomery domain */
   MOD_METHOD(pMont)->encode(dataY, dataY, pMont);

   /* exponentiation */
   gsMontExpBin_BNU_sscm(dataY, dataY, nsM, dataE, bitsizeE, pMont, pBuffer);

   /* convert result back to regular domain */
   MOD_METHOD(pMont)->decode(dataY, dataY, pMont);

   return nsM;
}
