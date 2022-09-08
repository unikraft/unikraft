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

#ifndef IFMA_ECPOINT_P256_H
#define IFMA_ECPOINT_P256_H

#include <internal/ecnist/ifma_arith_p256.h>

typedef struct {
   U64 X[P256_LEN52];
   U64 Y[P256_LEN52];
   U64 Z[P256_LEN52];
} P256_POINT;

typedef struct {
   U64 x[P256_LEN52];
   U64 y[P256_LEN52];
} P256_POINT_AFFINE;

typedef struct {
   int64u x[P256_LEN52];
   int64u y[P256_LEN52];
} SINGLE_P256_POINT_AFFINE;



/* check if coodinate is zero */
__INLINE __mb_mask MB_FUNC_NAME(is_zero_point_cordinate_)(const U64 T[])
{
   return MB_FUNC_NAME(is_zero_FE256_)(T);
}

/* set point to infinity */
__INLINE void MB_FUNC_NAME(set_point_to_infinity_)(P256_POINT* r)
{
   r->X[0] = r->X[1] = r->X[2] = r->X[3] = r->X[4] = get_zero64();
   r->Y[0] = r->Y[1] = r->Y[2] = r->Y[3] = r->Y[4] = get_zero64();
   r->Z[0] = r->Z[1] = r->Z[2] = r->Z[3] = r->Z[4] = get_zero64();
}

/* set point to infinity by mask */
__INLINE void MB_FUNC_NAME(mask_set_point_to_infinity_)(P256_POINT* r, __mb_mask mask)
{
   U64 zeros = get_zero64();

   r->X[0] = mask_mov64(r->X[0], mask, zeros);
   r->X[1] = mask_mov64(r->X[1], mask, zeros);
   r->X[2] = mask_mov64(r->X[2], mask, zeros);
   r->X[3] = mask_mov64(r->X[3], mask, zeros);
   r->X[4] = mask_mov64(r->X[4], mask, zeros);

   r->Y[0] = mask_mov64(r->Y[0], mask, zeros);
   r->Y[1] = mask_mov64(r->Y[1], mask, zeros);
   r->Y[2] = mask_mov64(r->Y[2], mask, zeros);
   r->Y[3] = mask_mov64(r->Y[3], mask, zeros);
   r->Y[4] = mask_mov64(r->Y[4], mask, zeros);

   r->Z[0] = mask_mov64(r->Z[0], mask, zeros);
   r->Z[1] = mask_mov64(r->Z[1], mask, zeros);
   r->Z[2] = mask_mov64(r->Z[2], mask, zeros);
   r->Z[3] = mask_mov64(r->Z[3], mask, zeros);
   r->Z[4] = mask_mov64(r->Z[4], mask, zeros);
}

/* set affine point to infinity */
__INLINE void MB_FUNC_NAME(set_point_affine_to_infonity_)(P256_POINT_AFFINE* r)
{
   r->x[0] = r->x[1] = r->x[2] = r->x[3] = r->x[4] = get_zero64();
   r->y[0] = r->y[1] = r->y[2] = r->y[3] = r->y[4] = get_zero64();
}

EXTERN_C void MB_FUNC_NAME(ifma_ec_nistp256_dbl_point_)(P256_POINT* r, const P256_POINT* p);
EXTERN_C void MB_FUNC_NAME(ifma_ec_nistp256_add_point_)(P256_POINT* r, const P256_POINT* p, const P256_POINT* q);
EXTERN_C void MB_FUNC_NAME(ifma_ec_nistp256_add_point_affine_)(P256_POINT* r, const P256_POINT* p, const P256_POINT_AFFINE* q);
EXTERN_C void MB_FUNC_NAME(ifma_ec_nistp256_mul_point_)(P256_POINT* r, const P256_POINT* p, const U64* scalar);
EXTERN_C void MB_FUNC_NAME(ifma_ec_nistp256_mul_pointbase_)(P256_POINT* r, const U64* scalar);
EXTERN_C void MB_FUNC_NAME(get_nistp256_ec_affine_coords_)(U64 x[], U64 y[], const P256_POINT* P);
EXTERN_C const U64* MB_FUNC_NAME(ifma_ec_nistp256_coord_one_)(void);
EXTERN_C __mb_mask MB_FUNC_NAME(ifma_is_on_curve_p256_)(const P256_POINT* p, int use_jproj_coords);

#endif  /* IFMA_ECPOINT_P256_H */
