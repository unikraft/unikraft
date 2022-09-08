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
#include "pcpgfpstuff.h"
#include "pcpgfpmethod.h"
#include "pcpecprime.h"

#if !defined(_PCP_GFPMETHOD_256_H_)
#define _PCP_GFPMETHOD_256_H_

#if(_IPP32E >= _IPP32E_M7)

/* arithmetic over arbitrary 256r-bit modulus */
#define gf256_add OWNAPI(gf256_add)
   IPP_OWN_DECL (BNU_CHUNK_T*, gf256_add, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, const BNU_CHUNK_T* pModulus))
#define gf256_sub OWNAPI(gf256_sub)
   IPP_OWN_DECL (BNU_CHUNK_T*, gf256_sub, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, const BNU_CHUNK_T* pModulus))
#define gf256_neg OWNAPI(gf256_neg)
   IPP_OWN_DECL (BNU_CHUNK_T*, gf256_neg, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pModulus))
#define gf256_mulm OWNAPI(gf256_mulm)
   IPP_OWN_DECL (BNU_CHUNK_T*, gf256_mulm, (BNU_CHUNK_T* pR,const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, const BNU_CHUNK_T* pModulus, BNU_CHUNK_T  m0))
#define gf256_sqrm OWNAPI(gf256_sqrm)
   IPP_OWN_DECL (BNU_CHUNK_T*, gf256_sqrm, (BNU_CHUNK_T* pR,const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pModulus, BNU_CHUNK_T  m0))
#define gf256_div2 OWNAPI(gf256_div2)
   IPP_OWN_DECL (BNU_CHUNK_T*, gf256_div2, (BNU_CHUNK_T* pR,const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pModulus))

#define OPERAND_BITSIZE (256)
#define LEN_P256        (BITS_BNU_CHUNK(OPERAND_BITSIZE))

static BNU_CHUNK_T* p256_add(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, gsEngine* pGFE)
{
   return gf256_add(pR, pA, pB, GFP_MODULUS(pGFE));
}

static BNU_CHUNK_T* p256_sub(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, gsEngine* pGFE)
{
   return gf256_sub(pR, pA, pB, GFP_MODULUS(pGFE));
}

static BNU_CHUNK_T* p256_neg(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE)
{
   return gf256_neg(pR, pA, GFP_MODULUS(pGFE));
}

static BNU_CHUNK_T* p256_div_by_2(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE)
{
   return gf256_div2(pR, pA, GFP_MODULUS(pGFE));
}

static BNU_CHUNK_T* p256_mul_by_2(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE)
{
   return gf256_add(pR, pA, pA, GFP_MODULUS(pGFE));
}

static BNU_CHUNK_T* p256_mul_by_3(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE)
{
   BNU_CHUNK_T tmp[LEN_P256];
   gf256_add(tmp, pA, pA, GFP_MODULUS(pGFE));
   return gf256_add(pR, tmp, pA, GFP_MODULUS(pGFE));
}

static BNU_CHUNK_T* p256_mul_montl(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, gsEngine* pGFE)
{
   return gf256_mulm(pR, pA, pB, GFP_MODULUS(pGFE), GFP_MNT_FACTOR(pGFE));
}

static BNU_CHUNK_T* p256_sqr_montl(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE)
{
   return gf256_sqrm(pR, pA, GFP_MODULUS(pGFE), GFP_MNT_FACTOR(pGFE));
}

static BNU_CHUNK_T* p256_to_mont(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE)
{
   return gf256_mulm(pR, pA, GFP_MNT_RR(pGFE), GFP_MODULUS(pGFE), GFP_MNT_FACTOR(pGFE));
}

static BNU_CHUNK_T one[] = {1,0,0,0};

static BNU_CHUNK_T* p256_mont_back(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE)
{
   return gf256_mulm(pR, pA, one, GFP_MODULUS(pGFE), GFP_MNT_FACTOR(pGFE));
}

/* return specific gf p256 arith methods */
static gsModMethod* gsArithGF_p256(void)
{
   static gsModMethod m = {
      p256_to_mont,
      p256_mont_back,
      p256_mul_montl,
      p256_sqr_montl,
      NULL,
      p256_add,
      p256_sub,
      p256_neg,
      p256_div_by_2,
      p256_mul_by_2,
      p256_mul_by_3,
   };
   return &m;
}
#endif /* _IPP32E >= _IPP32E_M7 */

#undef LEN_P256
#undef OPERAND_BITSIZE

#endif /* #if !defined(_PCP_GFPMETHOD_256_H_) */
