/*******************************************************************************
* Copyright 2021-2021 Intel Corporation
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

#ifndef IFMA_ED25519_H
#define IFMA_ED25519_H

#include <internal/ed25519/ifma_arith_p25519.h>

/* homogeneous: (X:Y:Z) satisfying x=X/Z, y=Y/Z */
typedef struct ge52_homo_mb_t {
   fe52_mb X;
   fe52_mb Y;
   fe52_mb Z;
} ge52_homo_mb;

/* extended homogeneous: (X:Y:Z:T) satisfying x=X/Z, y=Y/Z, XY=ZT */
typedef struct ge52_mb_t {
   fe52_mb X;
   fe52_mb Y;
   fe52_mb T;
   fe52_mb Z;
} ge52_ext_mb;

/* copmpleted: (X:Y:Z:T) satisfying x=X/Z, y=Y/T */
typedef struct ge52_p1p1_mb_t {
   fe52_mb X;
   fe52_mb Y;
   fe52_mb T;
   fe52_mb Z;
} ge52_p1p1_mb;

/* scalar precomputed group element: (y-x:y+x:2*t*d), t=x*y, ed25519 parameter d = -(121665/121666)*/
typedef struct ge52_precomp_t {
   fe52 ysubx;
   fe52 yaddx;
   fe52 t2d;
} ge52_precomp;

/* mb precomputed group element: (y-x:y+x:2*t*d), t=x*y, ed25519 parameter d = -(121665/121666)*/
typedef struct ge52_precomp_mb_t {
   fe52_mb ysubx;
   fe52_mb yaddx;
   fe52_mb t2d;
} ge52_precomp_mb;

/* projective falvor of the ge52_precomp_mb */
typedef struct ge52_cached_mb_t {
   fe52_mb YsubX;
   fe52_mb YaddX;
   fe52_mb T2d;
   fe52_mb Z;
} ge52_cached_mb;

/* bitsize of compression point */
#define GE25519_COMP_BITSIZE  (P25519_BITSIZE+1)

/*
// conversion
*/

/* ext => homo */
__INLINE void ge52_ext_to_homo_mb(ge52_homo_mb*r, const ge52_ext_mb* p)
{
   fe52_copy_mb(r->X, p->X);
   fe52_copy_mb(r->Y, p->Y);
   fe52_copy_mb(r->Z, p->Z);
}

/* p1p1 => homo */
__INLINE void ge52_p1p1_to_homo_mb(ge52_homo_mb *r, const ge52_p1p1_mb *p)
{
   fe52_mul(r->X, p->X, p->T);
   fe52_mul(r->Y, p->Y, p->Z);
   fe52_mul(r->Z, p->Z, p->T);
}

/* p1p1 => ext */
__INLINE void ge52_p1p1_to_ext_mb(ge52_ext_mb *r, const ge52_p1p1_mb *p)
{
   fe52_mul(r->X, p->X, p->T);
   fe52_mul(r->Y, p->Y, p->Z);
   fe52_mul(r->T, p->X, p->Y);
   fe52_mul(r->Z, p->Z, p->T);
}

/* set GE to neutral */
__INLINE void neutral_ge52_ext_mb(ge52_ext_mb* ge)
{
   fe52_0_mb(ge->X);
   fe52_1_mb(ge->Y);
   fe52_0_mb(ge->T);
   fe52_1_mb(ge->Z);
}
__INLINE void neutral_ge52_precomp_mb(ge52_precomp_mb *ge)
{
   fe52_1_mb(ge->ysubx);
   fe52_1_mb(ge->yaddx);
   fe52_0_mb(ge->t2d);
}

/* move GE under mask (conditionally): r = k? a : b */
__INLINE void ge52_cmov1_precomp_mb(ge52_precomp_mb* r, const ge52_precomp_mb* b, __mb_mask k, const ge52_precomp* a)
{
   fe52_cmov1_mb(r->ysubx, b->ysubx, k, a->ysubx);
   fe52_cmov1_mb(r->yaddx, b->yaddx, k, a->yaddx);
   fe52_cmov1_mb(r->t2d,   b->t2d,   k, a->t2d);
}
__INLINE void cmov_ge52_precomp_mb(ge52_precomp_mb* r, const ge52_precomp_mb* b, __mb_mask k, const ge52_precomp_mb* a)
{
   fe52_cmov_mb(r->ysubx, b->ysubx, k, a->ysubx);
   fe52_cmov_mb(r->yaddx, b->yaddx, k, a->yaddx);
   fe52_cmov_mb(r->t2d,   b->t2d,   k, a->t2d);
}


/* private functions */
void ifma_ed25519_mul_pointbase(ge52_ext_mb* r, const U64 scalar[]);
void ge52_ext_compress(fe52_mb r, const ge52_ext_mb* p);

#endif /* IFMA_ED25519_H */
