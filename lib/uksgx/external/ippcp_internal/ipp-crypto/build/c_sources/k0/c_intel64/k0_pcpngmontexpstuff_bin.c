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

IPP_OWN_DEFN (cpSize, gsMontExpBinBuffer, (int modulusBits))
{
   cpSize nsM = BITS_BNU_CHUNK(modulusBits);
   cpSize bufferNum = nsM;
   return bufferNum;
}

IPP_OWN_DEFN (cpSize, gsMontExpBin_BNU, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize bitsizeE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer) )
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

        /* copy and expand base to the modulus length */
        ZEXPAND_COPY_BNU(dataT, nsM, dataX, nsX);
        /* copy */
        COPY_BNU(dataY, dataT, nsM);

        FIX_BNU(dataE, nsE);

        /* execute most significant part pE */
        {
            BNU_CHUNK_T eValue = dataE[nsE - 1];
            int n = cpNLZ_BNU(eValue) + 1;

            eValue <<= n;
            for (; n<BNU_CHUNK_BITS; n++, eValue <<= 1) {
                /* squaring R = R*R mod Modulus */
                MOD_METHOD(pMont)->sqr(dataY, dataY, pMont);
                /* and multiply R = R*X mod Modulus */
                if (eValue & ((BNU_CHUNK_T)1 << (BNU_CHUNK_BITS - 1)))
                    MOD_METHOD(pMont)->mul(dataY, dataY, dataT, pMont);
            }

            /* execute rest bits of E */
            for (--nsE; nsE>0; nsE--) {
                eValue = dataE[nsE - 1];

                for (n = 0; n<BNU_CHUNK_BITS; n++, eValue <<= 1) {
                    /* squaring: R = R*R mod Modulus */
                    MOD_METHOD(pMont)->sqr(dataY, dataY, pMont);

                    if (eValue & ((BNU_CHUNK_T)1 << (BNU_CHUNK_BITS - 1)))
                        MOD_METHOD(pMont)->mul(dataY, dataY, dataT, pMont);
                }
            }
        }
    }

    return nsM;
}

/*
// "fast" binary montgomery exponentiation
//
// - input/output are in Regular Domain
// - possible inplace mode
//
// scratch buffer structure:
//    dataT[nsM]     copy of base (in case of inplace operation)
*/
IPP_OWN_DEFN (cpSize, gsModExpBin_BNU, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize bitsizeE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
{
   cpSize nsM = MOD_LEN(pMont);
   
   /* copy and expand base to the modulus length */
   ZEXPAND_COPY_BNU(dataY, nsM, dataX, nsX);
   /* convert base to Montgomery domain */
   MOD_METHOD(pMont)->encode(dataY, dataY, pMont);

   /* exponentiation */
   gsMontExpBin_BNU(dataY, dataY, nsM, dataE, bitsizeE, pMont, pBuffer);

   /* convert result back to regular domain */
   MOD_METHOD(pMont)->decode(dataY, dataY, pMont);

   return nsM;
}
