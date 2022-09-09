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
#include "pcpbnuarith.h"
#include "pcpecprime.h"

//tbcd: temporary excluded: #include <assert.h>

#if(_IPP >= _IPP_P8) || (_IPP32E >= _IPP32E_M7)

/* arithmetic over P-384r1 NIST modulus */
#define p384r1_add OWNAPI(p384r1_add)
   IPP_OWN_DECL (BNU_CHUNK_T*, p384r1_add, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, const BNU_CHUNK_T* b, gsEngine* pGFE))
#define p384r1_sub OWNAPI(p384r1_sub)
   IPP_OWN_DECL (BNU_CHUNK_T*, p384r1_sub, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, const BNU_CHUNK_T* b, gsEngine* pGFE))
#define p384r1_neg OWNAPI(p384r1_neg)
   IPP_OWN_DECL (BNU_CHUNK_T*, p384r1_neg, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE))
#define p384r1_div_by_2 OWNAPI(p384r1_div_by_2)
   IPP_OWN_DECL (BNU_CHUNK_T*, p384r1_div_by_2, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE))
#define p384r1_mul_by_2 OWNAPI(p384r1_mul_by_2)
   IPP_OWN_DECL (BNU_CHUNK_T*, p384r1_mul_by_2, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE))
#define p384r1_mul_by_3 OWNAPI(p384r1_mul_by_3)
   IPP_OWN_DECL (BNU_CHUNK_T*, p384r1_mul_by_3, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE))

#if(_IPP_ARCH ==_IPP_ARCH_EM64T)
//BNU_CHUNK_T* p384r1_mul_montl(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, const BNU_CHUNK_T* b, gsEngine* pGFE);
//BNU_CHUNK_T* p384r1_sqr_montl(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);
//BNU_CHUNK_T* p384r1_mul_montx(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, const BNU_CHUNK_T* b, gsEngine* pGFE);
//BNU_CHUNK_T* p384r1_sqr_montx(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);
//BNU_CHUNK_T* p384r1_to_mont  (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);
//BNU_CHUNK_T* p384r1_mont_back(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);
#endif

#define p384r1_mred OWNAPI(p384r1_mred)
   IPP_OWN_DECL (BNU_CHUNK_T*, p384r1_mred, (BNU_CHUNK_T* res, BNU_CHUNK_T* product))

#if(_IPP_ARCH ==_IPP_ARCH_IA32)
#define p384r1_mul_mont_slm OWNAPI(p384r1_mul_mont_slm)
   IPP_OWN_DECL (BNU_CHUNK_T*, p384r1_mul_mont_slm, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, const BNU_CHUNK_T* b, gsEngine* pGFE))
#define p384r1_sqr_mont_slm OWNAPI(p384r1_sqr_mont_slm)
   IPP_OWN_DECL (BNU_CHUNK_T*, p384r1_sqr_mont_slm, (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE))
#endif

#define OPERAND_BITSIZE (384)
#define LEN_P384        (BITS_BNU_CHUNK(OPERAND_BITSIZE))

/*
// multiplicative methods
*/
IPP_OWN_DEFN (static BNU_CHUNK_T*, p384r1_mul_montl, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, gsEngine* pGFE))
{
   BNU_CHUNK_T* product = cpGFpGetPool(2, pGFE);
   //tbcd: temporary excluded: assert(NULL!=product);

   cpMul_BNU_school(product, pA,LEN_P384, pB,LEN_P384);
   p384r1_mred(pR, product);

   cpGFpReleasePool(2, pGFE);
   return pR;
}

IPP_OWN_DEFN (static BNU_CHUNK_T*, p384r1_sqr_montl, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE))
{
   BNU_CHUNK_T* product = gsModPoolAlloc((gsModEngine*)pGFE, 2);

   cpSqr_BNU_school(product, pA,LEN_P384);
   p384r1_mred(pR, product);

   cpGFpReleasePool(2, pGFE);
   return pR;
}


/*
// Montgomery domain conversion constants
*/
static Ipp64u RR[] = {
   0xfffffffe00000001,0x0000000200000000,0xfffffffe00000000,
   0x0000000200000000,0x0000000000000001,0x0000000000000000
};

static BNU_CHUNK_T one[] = {
#if(_IPP_ARCH == _IPP_ARCH_EM64T)
   1,0,0,0,0,0};
#elif(_IPP_ARCH == _IPP_ARCH_IA32)
   1,0,0,0,0,0,0,0,0,0,0,0};
#endif

IPP_OWN_DEFN (static BNU_CHUNK_T*, p384r1_to_mont, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE))
{
   return p384r1_mul_montl(pR, pA, (BNU_CHUNK_T*)RR, pGFE);
}

IPP_OWN_DEFN (static BNU_CHUNK_T*, p384r1_mont_back, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE))
{
   return p384r1_mul_montl(pR, pA, (BNU_CHUNK_T*)one, pGFE);
}

#if (_ADCOX_NI_ENABLING_==_FEATURE_ON_) || (_ADCOX_NI_ENABLING_==_FEATURE_TICKTOCK_)
//BNU_CHUNK_T* p384r1_mul_montx(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, gsEngine* pGFE)
//BNU_CHUNK_T* p384r1_sqr_montx(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, gsEngine* pGFE)
#endif

#if(_IPP_ARCH ==_IPP_ARCH_IA32)
IPP_OWN_DEFN (static BNU_CHUNK_T*, p384r1_to_mont_slm, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE))
{
   return p384r1_mul_mont_slm(pR, pA, (BNU_CHUNK_T*)RR, pGFE);
}

IPP_OWN_DEFN (static BNU_CHUNK_T*, p384r1_mont_back_slm, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE))
{
   return p384r1_mul_mont_slm(pR, pA, (BNU_CHUNK_T*)one, pGFE);
}
#endif /* _IPP_ARCH ==_IPP_ARCH_IA32 */

/*
// return specific gf p384r1 arith methods,
//    p384r1 = 2^384 -2^128 -2^96 +2^32 -1 (NIST P384r1)
*/
static gsModMethod* gsArithGF_p384r1 (void)
{
   static gsModMethod m = {
      p384r1_to_mont,
      p384r1_mont_back,
      p384r1_mul_montl,
      p384r1_sqr_montl,
      NULL,
      p384r1_add,
      p384r1_sub,
      p384r1_neg,
      p384r1_div_by_2,
      p384r1_mul_by_2,
      p384r1_mul_by_3,
   };

   #if(_IPP_ARCH==_IPP_ARCH_IA32)
   if(IsFeatureEnabled(ippCPUID_SSSE3|ippCPUID_MOVBE) && !IsFeatureEnabled(ippCPUID_AVX)) {
      m.mul = p384r1_mul_mont_slm;
      m.sqr = p384r1_sqr_mont_slm;
      m.encode = p384r1_to_mont_slm;
      m.decode = p384r1_mont_back_slm;
   }
   #endif

   return &m;
}
#endif /* (_IPP >= _IPP_P8) || (_IPP32E >= _IPP32E_M7) */

/*F*
// Name: ippsGFpMethod_p384r1
//
// Purpose: Returns a reference to an implementation of
//          arithmetic operations over GF(q).
//
// Returns:  Pointer to a structure containing an implementation of arithmetic
//           operations over GF(q). q = 2^384 - 2^128 - 2^96 + 2^32 - 1
*F*/

IPPFUN( const IppsGFpMethod*, ippsGFpMethod_p384r1, (void) )
{
   static IppsGFpMethod method = {
      cpID_PrimeP384r1,
      384,
      secp384r1_p,
      NULL
   };

   #if(_IPP >= _IPP_P8) || (_IPP32E >= _IPP32E_M7)
   method.arith = gsArithGF_p384r1();
   #else
   method.arith = gsArithGFp();
   #endif

   return &method;
}

#undef LEN_P384
#undef OPERAND_BITSIZE
