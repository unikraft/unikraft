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
//        cpGFpSqrt
//
*/
#include "owncp.h"

#include "pcpbn.h"
#include "pcpgfpstuff.h"

static int factor2(BNU_CHUNK_T* pA, int nsA)
{
   int factor = 0;
   int bits;

   int i;
   for(i=0; i<nsA; i++) {
      int ntz = cpNTZ_BNU(pA[i]);
      factor += ntz;
      if(ntz<BITSIZE(BNU_CHUNK_T))
         break;
   }

   bits = factor;
   if(bits >= BITSIZE(BNU_CHUNK_T)) {
      int nchunk = bits/BITSIZE(BNU_CHUNK_T);
      cpGFpElementCopyPad(pA, nsA, pA+nchunk, nsA-nchunk);
      bits %= BITSIZE(BNU_CHUNK_T);
   }
   if(bits)
      cpLSR_BNU(pA, pA, nsA, bits);

   return factor;
}

static BNU_CHUNK_T* cpGFpExp2(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, int e, gsModEngine* pGFE)
{
   cpGFpElementCopy(pR, pA, GFP_FELEN(pGFE));
   while(e--) {
      GFP_METHOD(pGFE)->sqr(pR, pR, pGFE);
   }
   return pR;
}


/* returns:
   0, if a - qnr
   1, if sqrt is found
*/
IPP_OWN_DEFN (int, cpGFpSqrt, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsModEngine* pGFE))
{
   int elemLen = GFP_FELEN(pGFE);
   int poolelementLen = GFP_PELEN(pGFE);
   int resultFlag = 1;

   /* case A==0 */
   if( GFP_IS_ZERO(pA, elemLen) )
      cpGFpElementPad(pR, elemLen, 0);

   /* general case */
   else {
      BNU_CHUNK_T* q = cpGFpGetPool(4, pGFE);
      BNU_CHUNK_T* x = q + poolelementLen;
      BNU_CHUNK_T* y = x + poolelementLen;
      BNU_CHUNK_T* z = y + poolelementLen;

      int s;

      //tbcd: temporary excluded: assert(q!=NULL);

      /* z=1 */
      GFP_ONE(z, elemLen);

      /* (modulus-1) = 2^s*q */
      cpSub_BNU(q, GFP_MODULUS(pGFE), z, elemLen);
      s = factor2(q, elemLen);

      /*
      // initialization
      */

      /* y = qnr^q */
      cpGFpExp(y, GFP_QNR(pGFE), q,elemLen, pGFE);
      /* x = a^((q-1)/2) */
      cpSub_BNU(q, q, z, elemLen);
      cpLSR_BNU(q, q, elemLen, 1);
      cpGFpExp(x, pA, q, elemLen, pGFE);
      /* z = a*x^2 */
      GFP_METHOD(pGFE)->mul(z, x, x, pGFE);
      GFP_METHOD(pGFE)->mul(z, pA, z, pGFE);
      /* R = a*x */
      GFP_METHOD(pGFE)->mul(pR, pA, x, pGFE);

      while( !GFP_EQ(z, MOD_MNT_R(pGFE), elemLen) ) {
         int m = 0;
         cpGFpElementCopy(q, z, elemLen);

         for(m=1; m<s; m++) {
            GFP_METHOD(pGFE)->mul(q, q, q, pGFE);
            if( GFP_EQ(q, MOD_MNT_R(pGFE), elemLen) )
               break;
         }

         if(m==s) {
            /* A is quadratic non-residue */
            resultFlag = 0;
            break;
         }
         else {
            /* exponent reduction */
            cpGFpExp2(q, y, (s-m-1), pGFE);           /* q = y^(2^(s-m-1)) */
            GFP_METHOD(pGFE)->mul(y, q, q, pGFE);     /* y = q^2 */
            GFP_METHOD(pGFE)->mul(pR, q, pR, pGFE);   /* R = q*R */
            GFP_METHOD(pGFE)->mul(z, y, z, pGFE);     /* z = z*y */
            s = m;
         }
      }

      /* choose smallest between R and (modulus-R) */
      GFP_METHOD(pGFE)->decode(q, pR, pGFE);
      if(GFP_GT(q, GFP_HMODULUS(pGFE), elemLen))
         GFP_METHOD(pGFE)->neg(pR, pR, pGFE);

      cpGFpReleasePool(4, pGFE);
   }

   return resultFlag;
}
