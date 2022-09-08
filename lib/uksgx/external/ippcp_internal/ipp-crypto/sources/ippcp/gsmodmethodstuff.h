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

#if !defined(_GS_MOD_METHOD_STUFF_H)
#define _GS_MOD_METHOD_STUFF_H

#include "owndefs.h"
#include "owncp.h"
#include "pcpbnumisc.h"
#include "pcpbnuarith.h"

#include "gsmodstuff.h"
#include "gsmodmethod.h"
#include "pcpmontred.h"

//tbcd: temporary excluded: #include <assert.h>

/* r = (a+m) mod m */
/*
 * Requirements:
 *   Length of pr data buffer:   modLen
 *   Length of pa data buffer:   modLen
 *   Length of pb data buffer:   modLen
 *   Memory size from the pool:  modLen * sizeof(BNU_CHUNK_T)
 */
IPP_OWN_DEFN (static BNU_CHUNK_T*, gs_mont_add, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, const BNU_CHUNK_T* pb, gsModEngine* pME))
{
   const BNU_CHUNK_T* pm = MOD_MODULUS(pME);
   int mLen = MOD_LEN(pME);

   const int polLength  = 1;
   BNU_CHUNK_T* pBuffer = gsModPoolAlloc(pME, polLength);
   //tbcd: temporary excluded: assert(NULL!=pBuffer);
   {
      BNU_CHUNK_T extension = cpAdd_BNU(pr, pa, pb, mLen);
      extension -= cpSub_BNU(pBuffer, pr, pm, mLen);
      cpMaskMove_gs(pr, pBuffer, mLen, cpIsZero(extension));
   }
   gsModPoolFree(pME, polLength);
   return pr;
}

/* r = (a-b) mod m */
/*
 * Requirements:
 *   Length of pr data buffer:   modLen
 *   Length of pa data buffer:   modLen
 *   Length of pb data buffer:   modLen
 *   Memory size from the pool:  modLen * sizeof(BNU_CHUNK_T)
 */
IPP_OWN_DEFN (static BNU_CHUNK_T*, gs_mont_sub, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, const BNU_CHUNK_T* pb, gsModEngine* pME))
{
   const BNU_CHUNK_T* pm = MOD_MODULUS(pME);
   int mLen = MOD_LEN(pME);

   const int polLength  = 1;
   BNU_CHUNK_T* pBuffer = gsModPoolAlloc(pME, polLength);
   //tbcd: temporary excluded: assert(NULL!=pBuffer);
   {
      BNU_CHUNK_T extension = cpSub_BNU(pr, pa, pb, mLen);
      cpAdd_BNU(pBuffer, pr, pm, mLen);
      cpMaskMove_gs(pr, pBuffer, mLen, cpIsNonZero(extension));
   }
   gsModPoolFree(pME, polLength);
   return pr;
}

/* r = (m-a) mod m */
/*
 * Requirements:
 *   Length of pr data buffer:   modLen
 *   Length of pa data buffer:   modLen
 *   Memory size from the pool:  modLen * sizeof(BNU_CHUNK_T)
 */
IPP_OWN_DEFN (static BNU_CHUNK_T*, gs_mont_neg, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, gsModEngine* pME))
{
   const BNU_CHUNK_T* pm = MOD_MODULUS(pME);
   int mLen = MOD_LEN(pME);

   const int polLength  = 1;
   BNU_CHUNK_T* pBuffer = gsModPoolAlloc(pME, polLength);
   //tbcd: temporary excluded: assert(NULL!=pBuffer);
   {
      BNU_CHUNK_T extension = cpSub_BNU(pr, pm, pa, mLen);
      extension -= cpSub_BNU(pBuffer, pr, pm, mLen);
      cpMaskMove_gs(pr, pBuffer, mLen, cpIsZero(extension));
   }
   gsModPoolFree(pME, polLength);
   return pr;
}

/* r = (a/2) mod m */
/*
 * Requirements:
 *   Length of pr data buffer:   modLen
 *   Length of pa data buffer:   modLen
 *   Memory size from the pool:  modLen * sizeof(BNU_CHUNK_T)
 */
IPP_OWN_DEFN (static BNU_CHUNK_T*, gs_mont_div2, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, gsModEngine* pME))
{
   const BNU_CHUNK_T* pm = MOD_MODULUS(pME);
   int mLen = MOD_LEN(pME);

   const int polLength  = 1;
   BNU_CHUNK_T* pBuffer = gsModPoolAlloc(pME, polLength);
   //tbcd: temporary excluded: assert(NULL!=pBuffer);
   {
      cpSize i;
      BNU_CHUNK_T mask = 0 - (pa[0]&1);
      for(i=0; i<mLen; i++) pBuffer[i] = pm[i] & mask;

      pBuffer[mLen] = cpAdd_BNU(pBuffer, pBuffer, pa, mLen);
      cpLSR_BNU(pBuffer, pBuffer, mLen+1, 1);

      for(i=0; i<mLen; i++) pr[i] = pBuffer[i];
   }
   gsModPoolFree(pME, polLength);
   return pr;
}

/* r = (a*2) mod m */
/*
 * Requirements:
 *   Length of pr data buffer:   modLen
 *   Length of pa data buffer:   modLen
 *   Memory size from the pool:  modLen * sizeof(BNU_CHUNK_T)
 */
IPP_OWN_DEFN (static BNU_CHUNK_T*, gs_mont_mul2, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, gsModEngine* pME))
{
   return gs_mont_add(pr, pa, pa, pME);
}

/* r = (a*3) mod m */
/*
 * Requirements:
 *   Length of pr data buffer:   modLen
 *   Length of pa data buffer:   modLen
 *   Memory size from the pool:  modLen * sizeof(BNU_CHUNK_T) * 2
 */
IPP_OWN_DEFN (static BNU_CHUNK_T*, gs_mont_mul3, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, gsModEngine* pME))
{
   const int polLength  = 1;
   BNU_CHUNK_T* pBuffer = gsModPoolAlloc(pME, polLength);
   //tbcd: temporary excluded: assert(NULL!=pBuffer);

   gs_mont_add(pBuffer, pa, pa, pME);
   gs_mont_add(pr, pa, pBuffer, pME);

   gsModPoolFree(pME, polLength);
   return pr;
}

/* r = prod mod m */
/*
 * Requirements:
 *   Length of pr data buffer:   modLen
 *   Length of prod data buffer: modLen * 2
 *   Memory size from the pool:  n/a
 */
#if ((_IPP <_IPP_W7) && (_IPP32E <_IPP32E_M7)) || defined(_USE_C_cpMontRedAdc_BNU_)
IPP_OWN_DEFN (static BNU_CHUNK_T*, gs_mont_red, (BNU_CHUNK_T* pr, BNU_CHUNK_T* prod, gsModEngine* pME))
{
   const BNU_CHUNK_T* pm = MOD_MODULUS(pME);
   BNU_CHUNK_T k0 = MOD_MNT_FACTOR(pME);
   int mLen = MOD_LEN(pME);

   /* mont mul */
   BNU_CHUNK_T carry = 0;
   int i, j;

   for(i=0; i<mLen; i++, prod++) {
      BNU_CHUNK_T muL, muH, ex_mu;
      BNU_CHUNK_T t;

      /* u = prod[0]*k0 mod B */
      BNU_CHUNK_T u = prod[0] * k0;

      /* (ex_mu, t) = prod[0] + m[0]*u (note t ==0) */
      MUL_AB(muH, muL, pm[0], u);
      ADD_AB(ex_mu, t, prod[0], muL);
      ex_mu += muH;

      for(j=1; j<mLen; j++) {
         BNU_CHUNK_T c;
         MUL_AB(muH, muL, pm[j], u);         /* (H,L) = m[j]*u */
         ADD_AB(ex_mu, t, prod[j], ex_mu);   /* carry in ex_mu,t */ 
         ADD_AB(c, prod[j], t, muL);         /* carry in c */
         ex_mu += muH+c;                     /* accumulates both carrys above */
      }
      ADD_ABC(carry, prod[mLen], prod[mLen], ex_mu, carry);
   }

   {
      carry -= cpSub_BNU(pr, prod, pm, mLen);
      cpMaskMove_gs(pr, prod, mLen, cpIsNonZero(carry));
      return pr;
   }
}

#else
IPP_OWN_DEFN (static BNU_CHUNK_T*, gs_mont_red, (BNU_CHUNK_T* pr, BNU_CHUNK_T* prod, gsModEngine* pME))
{
   const BNU_CHUNK_T* pm = MOD_MODULUS(pME);
   BNU_CHUNK_T k0 = MOD_MNT_FACTOR(pME);
   int mLen = MOD_LEN(pME);

   cpMontRedAdc_BNU(pr, prod, pm, mLen, k0);

   return pr;
}

#if (_IPP32E>=_IPP32E_L9)
IPP_OWN_DEFN (static BNU_CHUNK_T*, gs_mont_redX, (BNU_CHUNK_T* pr, BNU_CHUNK_T* prod, gsModEngine* pME))
{
   const BNU_CHUNK_T* pm = MOD_MODULUS(pME);
   BNU_CHUNK_T k0 = MOD_MNT_FACTOR(pME);
   int mLen = MOD_LEN(pME);

   cpMontRedAdx_BNU(pr, prod, pm, mLen, k0);

   return pr;
}
#endif

#endif


/* r = (a*b) mod m */
/*
 * Requirements:
 *   Length of pr data buffer:   modLen
 *   Length of pa data buffer:   modLen
 *   Length of pb data buffer:   modLen
 *   Memory size from the pool:  modLen * sizeof(BNU_CHUNK_T) * 2
 */
#if ((_IPP <_IPP_W7) && (_IPP32E <_IPP32E_M7))
IPP_OWN_DEFN (static BNU_CHUNK_T*, gs_mont_mul, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, const BNU_CHUNK_T* pb, gsModEngine* pME))
{
   const BNU_CHUNK_T* pm = MOD_MODULUS(pME);
   BNU_CHUNK_T m0 = MOD_MNT_FACTOR(pME);
   int mLen = MOD_LEN(pME);

   const int polLength  = 1;
   BNU_CHUNK_T* pBuffer = gsModPoolAlloc(pME, polLength);
   //tbcd: temporary excluded: assert(NULL!=pBuffer);

   {
      BNU_CHUNK_T carry = 0;
      int i, j;

      /* clear buffer */
      for(i=0; i<mLen; i++) pBuffer[i] = 0;

      /* mont mul */
      for(i=0; i<mLen; i++) {
         BNU_CHUNK_T b = pb[i];

         BNU_CHUNK_T ex_ab = 0;
         BNU_CHUNK_T ex_mu = 0;

         BNU_CHUNK_T u;
         BNU_CHUNK_T abL, abH;
         BNU_CHUNK_T muL, muH;

         // (ex_ab, abL) = T = buffer[0] + a[0]*b
         MUL_AB(abH, abL, pa[0], b);
         ADD_ABC(ex_ab, abL, pBuffer[0], abL, ex_ab);
         ex_ab += abH;

         // u = t*m0 mod B
         u = abL * m0;

         // (ex_mu, abL) = T + m[0]*u (note abL ==0)
         MUL_AB(muH, muL, pm[0], u);
         ADD_ABC(ex_mu, abL, abL, muL, ex_mu);
         ex_mu += muH;

         for(j=1; j<mLen; j++) {
            MUL_AB(abH, abL, pa[j], b);
            ADD_ABC(ex_ab, abL, pBuffer[j], abL, ex_ab);
            ex_ab += abH;

            MUL_AB(muH, muL, pm[j], u);
            ADD_ABC(ex_mu, abL, abL, muL, ex_mu);
            ex_mu += muH;

            pBuffer[j-1] = abL;
         }
         ADD_ABC(carry, pBuffer[mLen-1], ex_ab, ex_mu, carry);
      }

      carry -= cpSub_BNU(pr, pBuffer, pm, mLen);
      cpMaskMove_gs(pr, pBuffer, mLen, cpIsNonZero(carry));
   }

   gsModPoolFree(pME, polLength);
   return pr;
}

#else

IPP_OWN_DEFN (static BNU_CHUNK_T*, gs_mont_mul, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, const BNU_CHUNK_T* pb, gsModEngine* pME))
{
   const BNU_CHUNK_T* pm = MOD_MODULUS(pME);
   BNU_CHUNK_T m0 = MOD_MNT_FACTOR(pME);
   int mLen = MOD_LEN(pME);

   const int polLength  = 2;
   BNU_CHUNK_T* pProduct = gsModPoolAlloc(pME, polLength);
   //tbcd: temporary excluded: assert(NULL!=pProduct);

   cpMulAdc_BNU_school(pProduct, pa,mLen, pb,mLen);
   cpMontRedAdc_BNU(pr, pProduct, pm, mLen, m0);

   gsModPoolFree(pME, polLength);
   return pr;
}

#if (_IPP32E>=_IPP32E_L9)
IPP_OWN_DEFN (static BNU_CHUNK_T*, gs_mont_mulX, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, const BNU_CHUNK_T* pb, gsModEngine* pME))
{
   const BNU_CHUNK_T* pm = MOD_MODULUS(pME);
   BNU_CHUNK_T m0 = MOD_MNT_FACTOR(pME);
   int mLen = MOD_LEN(pME);

   const int polLength  = 2;
   BNU_CHUNK_T* pProduct = gsModPoolAlloc(pME, polLength);
   //tbcd: temporary excluded: assert(NULL!=pProduct);

   cpMulAdx_BNU_school(pProduct, pa,mLen, pb,mLen);
   cpMontRedAdx_BNU(pr, pProduct, pm, mLen, m0);

   gsModPoolFree(pME, polLength);
   return pr;
}
#endif
#endif

/* r = (a^2) mod m */
/*
 * Requirements:
 *   Length of pr data buffer:   modLen
 *   Length of pa data buffer:   modLen
 *   Memory size from the pool:  modLen * sizeof(BNU_CHUNK_T)
 */
IPP_OWN_DEFN (static BNU_CHUNK_T*, gs_mont_sqr, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, gsModEngine* pME))
{
   //return gs_mont_mul(pr, pa, pa, pME);
   const BNU_CHUNK_T* pm = MOD_MODULUS(pME);
   BNU_CHUNK_T m0 = MOD_MNT_FACTOR(pME);
   int mLen = MOD_LEN(pME);

   const int polLength  = 2;
   BNU_CHUNK_T* pProduct = gsModPoolAlloc(pME, polLength);
   //tbcd: temporary excluded: assert(NULL!=pProduct);

   cpSqrAdc_BNU_school(pProduct, pa,mLen);
   cpMontRedAdc_BNU(pr, pProduct, pm, mLen, m0);

   gsModPoolFree(pME, polLength);
   return pr;
}

#if (_IPP32E>=_IPP32E_L9)
IPP_OWN_DEFN (static BNU_CHUNK_T*, gs_mont_sqrX, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, gsModEngine* pME))
{
   const BNU_CHUNK_T* pm = MOD_MODULUS(pME);
   BNU_CHUNK_T m0 = MOD_MNT_FACTOR(pME);
   int mLen = MOD_LEN(pME);

   const int polLength  = 2;
   BNU_CHUNK_T* pProduct = gsModPoolAlloc(pME, polLength);
   //tbcd: temporary excluded: assert(NULL!=pProduct);

   cpSqrAdx_BNU_school(pProduct, pa,mLen);
   cpMontRedAdx_BNU(pr, pProduct, pm, mLen, m0);

   gsModPoolFree(pME, polLength);
   return pr;
}
#endif

/* r = to_mont(a) */
/*
 * Requirements:
 *   Length of pr data buffer:   modLen
 *   Length of pa data buffer:   modLen
 *   Memory size from the pool:  modLen * sizeof(BNU_CHUNK_T)
 */
IPP_OWN_DEFN (static BNU_CHUNK_T*, gs_mont_encode, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, gsModEngine* pME))
{
   //return gs_mont_mul(pr, pa, MOD_MNT_R2(pME), pME);
   const BNU_CHUNK_T* pm = MOD_MODULUS(pME);
   BNU_CHUNK_T m0 = MOD_MNT_FACTOR(pME);
   int mLen = MOD_LEN(pME);

   const int polLength  = 2;
   BNU_CHUNK_T* pProduct = gsModPoolAlloc(pME, polLength);
   //tbcd: temporary excluded: assert(NULL!=pProduct);

   cpMulAdc_BNU_school(pProduct, pa,mLen, MOD_MNT_R2(pME),mLen);
   cpMontRedAdc_BNU(pr, pProduct, pm, mLen, m0);

   gsModPoolFree(pME, polLength);
   return pr;
}

#if (_IPP32E>=_IPP32E_L9)
IPP_OWN_DEFN (static BNU_CHUNK_T*, gs_mont_encodeX, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, gsModEngine* pME))
{
   //return gs_mont_mul(pr, pa, MOD_MNT_R2(pME), pME);
   const BNU_CHUNK_T* pm = MOD_MODULUS(pME);
   BNU_CHUNK_T m0 = MOD_MNT_FACTOR(pME);
   int mLen = MOD_LEN(pME);

   const int polLength  = 2;
   BNU_CHUNK_T* pProduct = gsModPoolAlloc(pME, polLength);
   //tbcd: temporary excluded: assert(NULL!=pProduct);

   cpMulAdx_BNU_school(pProduct, pa,mLen, MOD_MNT_R2(pME),mLen);
   cpMontRedAdx_BNU(pr, pProduct, pm, mLen, m0);

   gsModPoolFree(pME, polLength);
   return pr;
}
#endif

/* r = from_momt(a) */
/*
 * Requirements:
 *   Length of pr data buffer:   modLen
 *   Length of pa data buffer:   modLen
 *   Memory size from the pool:  modLen * sizeof(BNU_CHUNK_T)
 */
IPP_OWN_DEFN (static BNU_CHUNK_T*, gs_mont_decode, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, gsModEngine* pME))
{
   int mLen = MOD_LEN(pME);

   const int polLength  = 2;
   BNU_CHUNK_T* pProduct = gsModPoolAlloc(pME, polLength);
   //tbcd: temporary excluded: assert(NULL!=pProduct);

   ZEXPAND_COPY_BNU(pProduct, 2*mLen, pa, mLen);
   cpMontRedAdc_BNU(pr, pProduct, MOD_MODULUS(pME), mLen, MOD_MNT_FACTOR(pME));

   gsModPoolFree(pME, polLength);
   return pr;
}

#if (_IPP32E>=_IPP32E_L9)
IPP_OWN_DEFN (static BNU_CHUNK_T*, gs_mont_decodeX, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, gsModEngine* pME))
{
   int mLen = MOD_LEN(pME);

   const int polLength  = 2;
   BNU_CHUNK_T* pProduct = gsModPoolAlloc(pME, polLength);
   //tbcd: temporary excluded: assert(NULL!=pProduct);

   ZEXPAND_COPY_BNU(pProduct, 2*mLen, pa, mLen);
   cpMontRedAdx_BNU(pr, pProduct, MOD_MODULUS(pME), mLen, MOD_MNT_FACTOR(pME));

   gsModPoolFree(pME, polLength);
   return pr;
}
#endif

#endif /* _GS_MOD_METHOD_STUFF_H */

