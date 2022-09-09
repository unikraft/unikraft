/*******************************************************************************
* Copyright 2016-2021 Intel Corporation
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
//     GF(p) methods
//
*/
#include "owndefs.h"
#include "owncp.h"

#include "pcpbnumisc.h"
#include "gsmodstuff.h"
#include "pcpgfpstuff.h"
#include "pcpgfpmethod.h"
#include "pcpecprime.h"

//tbcd: temporary excluded: #include <assert.h>

#if(_IPP >= _IPP_P8) || (_IPP32E >= _IPP32E_M7)

/* arithmetic over P-192r1 NIST modulus */
#define p192r1_add OWNAPI(p192r1_add)
   IPP_OWN_DECL (BNU_CHUNK_T*, p192r1_add, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, const BNU_CHUNK_T* b, gsEngine* pGFE))
#define p192r1_sub OWNAPI(p192r1_sub)
   IPP_OWN_DECL (BNU_CHUNK_T*, p192r1_sub, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, const BNU_CHUNK_T* b, gsEngine* pGFE))
#define p192r1_neg OWNAPI(p192r1_neg)
   IPP_OWN_DECL (BNU_CHUNK_T*, p192r1_neg, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE))
#define p192r1_div_by_2  OWNAPI(p192r1_div_by_2)
   IPP_OWN_DECL (BNU_CHUNK_T*, p192r1_div_by_2, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE))
#define p192r1_mul_by_2  OWNAPI(p192r1_mul_by_2)
   IPP_OWN_DECL (BNU_CHUNK_T*, p192r1_mul_by_2, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE))
#define p192r1_mul_by_3  OWNAPI(p192r1_mul_by_3)
   IPP_OWN_DECL (BNU_CHUNK_T*, p192r1_mul_by_3, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE))

#if(_IPP_ARCH ==_IPP_ARCH_EM64T)
#define p192r1_mul_montl OWNAPI(p192r1_mul_montl)
   IPP_OWN_DECL (BNU_CHUNK_T*, p192r1_mul_montl, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, const BNU_CHUNK_T* b, gsEngine* pGFE))
#define p192r1_mul_montx OWNAPI(p192r1_mul_montx)
   IPP_OWN_DECL (BNU_CHUNK_T*, p192r1_mul_montx, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, const BNU_CHUNK_T* b, gsEngine* pGFE))
#define p192r1_sqr_montl OWNAPI(p192r1_sqr_montl)
   IPP_OWN_DECL (BNU_CHUNK_T*, p192r1_sqr_montl, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE))
#define p192r1_sqr_montx OWNAPI(p192r1_sqr_montx)
   IPP_OWN_DECL (BNU_CHUNK_T*, p192r1_sqr_montx, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE))
#define p192r1_to_mont   OWNAPI(p192r1_to_mont)
   IPP_OWN_DECL (BNU_CHUNK_T*, p192r1_to_mont, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE))
#define p192r1_mont_back OWNAPI(p192r1_mont_back)
   IPP_OWN_DECL (BNU_CHUNK_T*, p192r1_mont_back, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE))
#endif

#if(_IPP_ARCH ==_IPP_ARCH_IA32)
#define p192r1_mul_mont_slm OWNAPI(p192r1_mul_mont_slm)
   IPP_OWN_DECL (BNU_CHUNK_T*, p192r1_mul_mont_slm, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, const BNU_CHUNK_T* b, gsEngine* pGFE))
#define p192r1_sqr_mont_slm OWNAPI(p192r1_sqr_mont_slm)
   IPP_OWN_DECL (BNU_CHUNK_T*, p192r1_sqr_mont_slm, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE))
#define p192r1_mred OWNAPI(p192r1_mred)
   IPP_OWN_DECL (BNU_CHUNK_T*, p192r1_mred, (BNU_CHUNK_T* res, BNU_CHUNK_T* product))
#endif

#define OPERAND_BITSIZE (192)
#define LEN_P192        (BITS_BNU_CHUNK(OPERAND_BITSIZE))

/*
// ia32 multiplicative methods
*/
#if (_IPP_ARCH ==_IPP_ARCH_IA32)
IPP_OWN_DEFN (static BNU_CHUNK_T*, p192r1_mul_montl, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, gsEngine* pGFE))
{
   BNU_CHUNK_T* product = cpGFpGetPool(2, pGFE);
   //tbcd: temporary excluded: assert(NULL!=product);

   cpMulAdc_BNU_school(product, pA,LEN_P192, pB,LEN_P192);
   p192r1_mred(pR, product);

   cpGFpReleasePool(2, pGFE);
   return pR;
}

IPP_OWN_DEFN (static BNU_CHUNK_T*, p192r1_sqr_montl, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE))
{
   BNU_CHUNK_T* product = cpGFpGetPool(2, pGFE);
   //tbcd: temporary excluded: assert(NULL!=product);

   cpSqrAdc_BNU_school(product, pA,LEN_P192);
   p192r1_mred(pR, product);

   cpGFpReleasePool(2, pGFE);
   return pR;
}


/*
// Montgomery domain conversion constants
*/
static BNU_CHUNK_T RR[] = {
   0x00000001,0x00000000,0x00000002,0x00000000,
   0x00000001,0x00000000};

static BNU_CHUNK_T one[] = {
   1,0,0,0,0,0};

IPP_OWN_DEFN (static BNU_CHUNK_T*, p192r1_to_mont, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE))
{
   return p192r1_mul_montl(pR, pA, (BNU_CHUNK_T*)RR, pGFE);
}

IPP_OWN_DEFN (static BNU_CHUNK_T*, p192r1_mont_back, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE))
{
   return p192r1_mul_montl(pR, pA, (BNU_CHUNK_T*)one, pGFE);
}

IPP_OWN_DEFN (static BNU_CHUNK_T*, p192r1_to_mont_slm, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE))
{
   return p192r1_mul_mont_slm(pR, pA, (BNU_CHUNK_T*)RR, pGFE);
}

IPP_OWN_DEFN (static BNU_CHUNK_T*, p192r1_mont_back_slm, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE))
{
   return p192r1_mul_mont_slm(pR, pA, (BNU_CHUNK_T*)one, pGFE);
}
#endif /* _IPP >= _IPP_P8 */

/*
// return specific gf p192r1 arith methods,
//    p192r1 = 2^192 -2^64 -1 (NIST P192r1)
*/
static gsModMethod* gsArithGF_p192r1 (void)
{
   static gsModMethod m = {
      p192r1_to_mont,
      p192r1_mont_back,
      p192r1_mul_montl,
      p192r1_sqr_montl,
      NULL,
      p192r1_add,
      p192r1_sub,
      p192r1_neg,
      p192r1_div_by_2,
      p192r1_mul_by_2,
      p192r1_mul_by_3,
   };

   #if(_IPP_ARCH==_IPP_ARCH_EM64T) && ((_ADCOX_NI_ENABLING_==_FEATURE_ON_) || (_ADCOX_NI_ENABLING_==_FEATURE_TICKTOCK_))
   if(IsFeatureEnabled(ippCPUID_ADCOX)) {
      m.mul = p192r1_mul_montx;
      m.sqr = p192r1_sqr_montx;
   }
   #endif

   #if(_IPP_ARCH==_IPP_ARCH_IA32)
   if(IsFeatureEnabled(ippCPUID_SSSE3|ippCPUID_MOVBE) && !IsFeatureEnabled(ippCPUID_AVX)) {
      m.mul = p192r1_mul_mont_slm;
      m.sqr = p192r1_sqr_mont_slm;
      m.encode = p192r1_to_mont_slm;
      m.decode = p192r1_mont_back_slm;
   }
   #endif

   return &m;
}
#endif /* (_IPP >= _IPP_P8) || (_IPP32E >= _IPP32E_M7) */

/*F*
// Name: ippsGFpMethod_p192r1
//
// Purpose: Returns a reference to an implementation of
//          arithmetic operations over GF(q).
//
// Returns:  Pointer to a structure containing an implementation of arithmetic
//           operations over GF(q). q = 2^192 - 2^64 - 1
*F*/

IPPFUN( const IppsGFpMethod*, ippsGFpMethod_p192r1, (void) )
{
   static IppsGFpMethod method = {
      cpID_PrimeP192r1,
      192,
      secp192r1_p,
      NULL
   };

   #if(_IPP >= _IPP_P8) || (_IPP32E >= _IPP32E_M7)
   method.arith = gsArithGF_p192r1();
   #else
   method.arith = gsArithGFp();
   #endif

   return &method;
}

#undef LEN_P192
#undef OPERAND_BITSIZE
