/*******************************************************************************
* Copyright 2002-2021 Intel Corporation
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

#include <internal/common/ifma_defs.h>
#include <internal/common/ifma_math.h>

#include <internal/ed25519/ifma_arith_p25519.h>
#include <internal/ed25519/ifma_arith_ed25519.h>
#include <internal/ed25519/ifma_ed25519_precomp4.h>

#define BP_WIN_SIZE  MUL_BASEPOINT_WIN_SIZE  /* defined in the header above */

/*
// Twisted Edwards Curve parameters
*/

#if 0
/* d = -(121665/121666) */
__ALIGN64 static const int64u ed25519_d[FE_LEN52][sizeof(U64)/sizeof(int64u)] = {
   { REP8_DECL(0x000b4dca135978a3) },
   { REP8_DECL(0x0004d4141d8ab75e) },
   { REP8_DECL(0x000779e89800700a) },
   { REP8_DECL(0x000fe738cc740797) },
   { REP8_DECL(0x000052036cee2b6f) }
};
#endif
/* (2*d) */
__ALIGN64 static const int64u ed25519_d2[FE_LEN52][sizeof(U64) / sizeof(int64u)] = {
   { REP8_DECL(0x00069b9426b2f159) },
   { REP8_DECL(0x0009a8283b156ebd) },
   { REP8_DECL(0x000ef3d13000e014) },
   { REP8_DECL(0x000fce7198e80f2e) },
   { REP8_DECL(0x00002406d9dc56df) }
};

/* r = p */
static void ge_ext_to_cached(ge52_cached_mb *r, const ge52_ext_mb* p)
{
   fe52_add(r->YaddX, p->Y, p->X);
   fe52_sub(r->YsubX, p->Y, p->X);
   fe52_copy_mb(r->Z, p->Z);
   fe52_mul(r->T2d, p->T, (U64*)ed25519_d2);
}

/*
// r = p + q
*/
static void ge52_add_precomp(ge52_p1p1_mb *r, const ge52_ext_mb *p, const ge52_precomp_mb *q)
{
   fe52_mb t0;

   fe52_add(r->X, p->Y, p->X);      // X3 = Y1+X1
   fe52_sub(r->Y, p->Y, p->X);      // Y3 = Y1-X1
   fe52_mul(r->Z, r->X, q->yaddx);  // Z3 = X3*yplusx2
   fe52_mul(r->Y, r->Y, q->ysubx);  // Y3 = Y3*yminisx2
   fe52_mul(r->T, q->t2d, p->T);    // T3 = T1*xy2d2
   fe52_add(t0, p->Z, p->Z);        // t0 = Z1+Z1
   fe52_sub(r->X, r->Z, r->Y);      // X3 = Z3-Y3 = X3*yplusx2 - Y3*yminisx2 = (Y1+X1)*yplusx2 - (Y1-X1)*yminisx2
   fe52_add(r->Y, r->Z, r->Y);      // Y3 = Z3+Y3 = X3*yplusx2 + Y3*yminisx2 = (Y1+X1)*yplusx2 + (Y1-X1)*yminisx2
   fe52_add(r->Z, t0, r->T);        // Z3 = 2*Z1 + T1*xy2d2
   fe52_sub(r->T, t0, r->T);        // T3 = 2*Z1 - T1*xy2d2
}

static void ge_add(ge52_p1p1_mb* r, const ge52_ext_mb* p, const ge52_cached_mb* q)
{
   fe52_mb t0;

   fe52_add(r->X, p->Y, p->X);
   fe52_sub(r->Y, p->Y, p->X);
   fe52_mul(r->Z, r->X, q->YaddX);
   fe52_mul(r->Y, r->Y, q->YsubX);
   fe52_mul(r->T, q->T2d, p->T);
   fe52_mul(r->X, p->Z, q->Z);
   fe52_add(t0, r->X, r->X);
   fe52_sub(r->X, r->Z, r->Y);
   fe52_add(r->Y, r->Z, r->Y);
   fe52_add(r->Z, t0, r->T);
   fe52_sub(r->T, t0, r->T);
}

/* r = 2 * p */
static void ge_dbl(ge52_p1p1_mb *r, const ge52_homo_mb* p)
{
   fe52_mb t0;

   fe52_sqr(r->X, p->X);
   fe52_sqr(r->Z, p->Y);
   fe52_sqr(r->T, p->Z);
   fe52_add(r->T, r->T, r->T);
   fe52_add(r->Y, p->X, p->Y);
   fe52_sqr(t0, r->Y);
   fe52_add(r->Y, r->Z, r->X);
   fe52_sub(r->Z, r->Z, r->X);
   fe52_sub(r->X, t0, r->Y);
   fe52_sub(r->T, r->T, r->Z);
}

/* point compress */
void ge52_ext_compress(fe52_mb r, const ge52_ext_mb* p)
{
   fe52_mb recip;
   fe52_mb x;
   fe52_mb y;

   fe52_inv(recip, p->Z);
   fe52_mul(x, p->X, recip);
   fe52_mul(y, p->Y, recip);

   __mb_mask is_negative = cmp64_mask(and64_const(x[0], 1), set1(1), _MM_CMPINT_EQ);
   y[(GE25519_COMP_BITSIZE-1)/DIGIT_SIZE] = _mm512_mask_or_epi64(y[(GE25519_COMP_BITSIZE-1)/DIGIT_SIZE], is_negative, y[(GE25519_COMP_BITSIZE - 1) / DIGIT_SIZE], set1(1LL << (GE25519_COMP_BITSIZE-1)%DIGIT_SIZE));

   fe52_copy_mb(r, y);
}


/* select from the pre-computed table */
static void extract_precomputed_basepoint_dual(ge52_precomp_mb* p0,
                                               ge52_precomp_mb* p1,
                                         const ge52_precomp* tbl,
                                               U64 idx0, U64 idx1)
{
   /* set h0, h1 to neutral point */
   neutral_ge52_precomp_mb(p0);
   neutral_ge52_precomp_mb(p1);

   /* indexes are considered as signed values */
   __mb_mask is_neg_idx0 = cmp64_mask(idx0, get_zero64(), _MM_CMPINT_LT);
   __mb_mask is_neg_idx1 = cmp64_mask(idx1, get_zero64(), _MM_CMPINT_LT);
   idx0 = mask_sub64(idx0, is_neg_idx0, get_zero64(), idx0);
   idx1 = mask_sub64(idx1, is_neg_idx1, get_zero64(), idx1);

   /* select p0, p1 wrt idx0, idx1 indexes */
   ge52_cmov1_precomp_mb(p0, p0, cmp64_mask(idx0, set1(1), _MM_CMPINT_EQ), tbl);
   ge52_cmov1_precomp_mb(p1, p1, cmp64_mask(idx1, set1(1), _MM_CMPINT_EQ), tbl);
   tbl++;
   ge52_cmov1_precomp_mb(p0, p0, cmp64_mask(idx0, set1(2), _MM_CMPINT_EQ), tbl);
   ge52_cmov1_precomp_mb(p1, p1, cmp64_mask(idx1, set1(2), _MM_CMPINT_EQ), tbl);
   tbl++;
   ge52_cmov1_precomp_mb(p0, p0, cmp64_mask(idx0, set1(3), _MM_CMPINT_EQ), tbl);
   ge52_cmov1_precomp_mb(p1, p1, cmp64_mask(idx1, set1(3), _MM_CMPINT_EQ), tbl);
   tbl++;
   ge52_cmov1_precomp_mb(p0, p0, cmp64_mask(idx0, set1(4), _MM_CMPINT_EQ), tbl);
   ge52_cmov1_precomp_mb(p1, p1, cmp64_mask(idx1, set1(4), _MM_CMPINT_EQ), tbl);
   tbl++;
   ge52_cmov1_precomp_mb(p0, p0, cmp64_mask(idx0, set1(5), _MM_CMPINT_EQ), tbl);
   ge52_cmov1_precomp_mb(p1, p1, cmp64_mask(idx1, set1(5), _MM_CMPINT_EQ), tbl);
   tbl++;
   ge52_cmov1_precomp_mb(p0, p0, cmp64_mask(idx0, set1(6), _MM_CMPINT_EQ), tbl);
   ge52_cmov1_precomp_mb(p1, p1, cmp64_mask(idx1, set1(6), _MM_CMPINT_EQ), tbl);
   tbl++;
   ge52_cmov1_precomp_mb(p0, p0, cmp64_mask(idx0, set1(7), _MM_CMPINT_EQ), tbl);
   ge52_cmov1_precomp_mb(p1, p1, cmp64_mask(idx1, set1(7), _MM_CMPINT_EQ), tbl);
   tbl++;
   ge52_cmov1_precomp_mb(p0, p0, cmp64_mask(idx0, set1(8), _MM_CMPINT_EQ), tbl);
   ge52_cmov1_precomp_mb(p1, p1, cmp64_mask(idx1, set1(8), _MM_CMPINT_EQ), tbl);

   /* adjust for sign */
   fe52_mb neg;
   fe52_neg(neg, p0->t2d);
   fe52_cswap_mb(p0->ysubx, is_neg_idx0, p0->yaddx);
   fe52_cmov_mb(p0->t2d, p0->t2d, is_neg_idx0, neg);

   fe52_neg(neg, p1->t2d);
   fe52_cswap_mb(p1->ysubx, is_neg_idx1, p1->yaddx);
   fe52_cmov_mb(p1->t2d, p1->t2d, is_neg_idx1, neg);
}

/*
 * r = [s]*G
 *
 * where s = s[0] + 256*s[1] +...+ 256^31 s[31]
 * G is the Ed25519 base point (x,4/5) with x positive.
 *
 * Preconditions:
 *   s[31] <= 127
 */

/* if msb set */
__INLINE int32u isMsb_ct(int32u a)
{ return (int32u)0 - (a >> (sizeof(a) * 8 - 1)); }

/* tests if a==0 */
__INLINE int32u isZero(int32u a)
{ return isMsb_ct(~a & (a - 1)); }

/* tests if a==b */
__INLINE int32u isEqu(int32u a, int32u b)
{ return isZero(a ^ b); }

void ifma_ed25519_mul_pointbase(ge52_ext_mb* r, const U64 scalar[])
{
   /* implementation uses scalar representation over base b=16, q[i] are half-bytes */
   __ALIGN64 ge52_ext_mb r0; /* q[0]*16^0 + q[2]*16^2 + q[4]*16^4 + ...+ q[30]*16^30 */
   __ALIGN64 ge52_ext_mb r1; /* q[1]*16^1 + q[3]*16^3 + q[5]*16^5 + ...+ q[31]*16^31 */

   /* point extracted from the pre-computed table */
   __ALIGN64 ge52_precomp_mb h0;
   __ALIGN64 ge52_precomp_mb h1;

   /* temporary */
   __ALIGN64 ge52_p1p1_mb t;
   __ALIGN64 ge52_homo_mb s;

   /* inital values are nuetral */
   neutral_ge52_ext_mb(&r0);
   neutral_ge52_ext_mb(&r1);

   /* pre-computed basepoint table  */
   ge52_precomp* tbl = &ifma_ed25519_bp_precomp[0][0];

   __mb_mask carry = 0;    /* used in convertion to signed value */

   for(int n=0; n< FE_LEN64; n++) {
      /* scalar value*/
      U64 scalarV = loadu64(&scalar[n]);

      for(int m=0; m<sizeof(int64u); m++) {
         /* set if last byte processed */
         __mb_mask last_byte = (__mb_mask)(isEqu((n*sizeof(int64u)+m), FE_LEN64*sizeof(int64u)-1));

         /* extract 2 half-bytes */
         U64 q_even = and64_const(scalarV, 0x0f);
         U64 q_odd  = and64_const(srli64(scalarV, 4), 0x0f);
         /* to the next byte of scalar value */
         scalarV = srli64(scalarV, 8);

         /* convert half-bytes to signed */
         q_even = mask_add64(q_even, carry, q_even, set1(1));
         carry = cmp64_mask(set1(8), q_even, _MM_CMPINT_LE);
         q_even = mask_sub64(q_even, carry, q_even, set1(0x10));

         q_odd = mask_add64(q_odd, carry, q_odd, set1(1));
         carry = cmp64_mask(set1(8), q_odd, _MM_CMPINT_LE);
         carry &= ~last_byte;  /* avoid sign conversion for the last half-byte*/
         q_odd = mask_sub64(q_odd, carry, q_odd, set1(0x10));

         /* extract points from the pre-computed table */
         extract_precomputed_basepoint_dual(&h0, &h1, tbl, q_even, q_odd);
         tbl += BP_N_ENTRY;

         /* r0 += h0, r1 += h1 */
         ge52_add_precomp(&t, &r0, &h0);
         ge52_p1p1_to_ext_mb(&r0, &t);

         ge52_add_precomp(&t, &r1, &h1);
         ge52_p1p1_to_ext_mb(&r1, &t);
      }
   }

   /* r1 = [16]*r1 */
   ge52_ext_to_homo_mb(&s, &r1);

   ge_dbl(&t, &s);
   ge52_p1p1_to_homo_mb(&s, &t);

   ge_dbl(&t, &s);
   ge52_p1p1_to_homo_mb(&s, &t);

   ge_dbl(&t, &s);
   ge52_p1p1_to_homo_mb(&s, &t);

   ge_dbl(&t, &s);
   ge52_p1p1_to_ext_mb(&r1, &t);

   // r = r0 + r1
   __ALIGN64 ge52_cached_mb c;
   ge_ext_to_cached(&c, &r0); //
   ge_add(&t, &r1, &c);

   ge52_p1p1_to_ext_mb(r, &t);
}
