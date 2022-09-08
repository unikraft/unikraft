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

#ifndef IFMA_ARITH_P256_H
#define IFMA_ARITH_P256_H

#include <internal/common/ifma_defs.h>
#include <internal/common/ifma_math.h>

/* underlying prime's size */
#define P256_BITSIZE (256)

/* lengths of FF elements */
#define P256_LEN52  NUMBER_OF_DIGITS(P256_BITSIZE,DIGIT_SIZE)
#define P256_LEN64  NUMBER_OF_DIGITS(P256_BITSIZE,64)

__ALIGN64 static const int64u ones[P256_LEN52][sizeof(U64)/sizeof(int64u)] = {
   { REP8_DECL(1) },
   { REP8_DECL(0) },
   { REP8_DECL(0) },
   { REP8_DECL(0) },
   { REP8_DECL(0) }
};

static const int64u VMASK52[sizeof(U64)/sizeof(int64u)] = {
   REP8_DECL(DIGIT_MASK)
};

#define NORM_LSHIFTR(R, I, J) \
    R##J = add64(R##J, srli64(R##I, DIGIT_SIZE)); \
    R##I = and64(R##I, loadu64(VMASK52));

#define NORM_ASHIFTR(R, I, J) \
    R##J = add64(R##J, srai64(R##I, DIGIT_SIZE)); \
    R##I = and64(R##I, loadu64(VMASK52));


/* set FE to zero */
__INLINE void MB_FUNC_NAME(zero_FE256_)(U64 T[])
{
   T[0] = T[1] = T[2] = T[3] = T[4] = get_zero64();
}

/* check if FE is zero */
__INLINE __mb_mask MB_FUNC_NAME(is_zero_FE256_)(const U64 T[])
{
   U64 Z = or64(or64(T[0], T[1]), or64(or64(T[2], T[3]), T[4]));
   return cmpeq64_mask(Z, get_zero64());
}

__INLINE U64 cmov_U64(U64 a, U64 b, __mb_mask kmask)
{  return mask_mov64 (a, kmask, b); }

/* move field element */
__INLINE void MB_FUNC_NAME(mov_FE256_)(U64 r[], const U64 a[])
{
   r[0] = a[0];
   r[1] = a[1];
   r[2] = a[2];
   r[3] = a[3];
   r[4] = a[4];
}

/* move coodinate using mask: R = k? A : B */
__INLINE void MB_FUNC_NAME(mask_mov_FE256_)(U64 R[], const U64 B[], __mb_mask k, const U64 A[])
{
   R[0] = mask_mov64(B[0], k, A[0]);
   R[1] = mask_mov64(B[1], k, A[1]);
   R[2] = mask_mov64(B[2], k, A[2]);
   R[3] = mask_mov64(B[3], k, A[3]);
   R[4] = mask_mov64(B[4], k, A[4]);
}

__INLINE void MB_FUNC_NAME(secure_mask_mov_FE256_)(U64 R[], U64 B[], __mb_mask k, const U64 A[])
{
   R[0] = select64(k, B[0], (U64*)(&A[0]));
   R[1] = select64(k, B[1], (U64*)(&A[1]));
   R[2] = select64(k, B[2], (U64*)(&A[2]));
   R[3] = select64(k, B[3], (U64*)(&A[3]));
   R[4] = select64(k, B[4], (U64*)(&A[4]));
}

/* compare two FE */
__INLINE __mb_mask MB_FUNC_NAME(cmp_lt_FE256_)(const U64 A[], const U64 B[])
{
   /* r = a - b */
   U64 r0 = sub64(A[0], B[0]);
   U64 r1 = sub64(A[1], B[1]);
   U64 r2 = sub64(A[2], B[2]);
   U64 r3 = sub64(A[3], B[3]);
   U64 r4 = sub64(A[4], B[4]);

   /* normalize r0 – r4 */
   NORM_ASHIFTR(r, 0, 1)
   NORM_ASHIFTR(r, 1, 2)
   NORM_ASHIFTR(r, 2, 3)
   NORM_ASHIFTR(r, 3, 4)

   /* return mask LT */
   return cmp64_mask(r4, get_zero64(), _MM_CMPINT_LT);
}

__INLINE __mb_mask MB_FUNC_NAME(cmp_eq_FE256_)(const U64 A[], const U64 B[]) 
{
   __ALIGN64 U64 msg[P256_LEN52];

   msg[0] = xor64(A[0], B[0]);
   msg[1] = xor64(A[1], B[1]);
   msg[2] = xor64(A[2], B[2]);
   msg[3] = xor64(A[3], B[3]);
   msg[4] = xor64(A[4], B[4]);
   
   return MB_FUNC_NAME(is_zero_FE256_)(msg);
}

/* General 256-bit operations */
EXTERN_C void MB_FUNC_NAME(ifma_amm52x5_)(U64 R[], const U64 inpA[], const U64 inpB[], const U64 inpM[], const int64u* k0_mb);
EXTERN_C void MB_FUNC_NAME(ifma_ams52x5_)(U64 r[], const U64 a[], const U64 m[], const int64u* k0_mb);
EXTERN_C void MB_FUNC_NAME(ifma_add52x5_)(U64 R[], const U64 A[], const U64 B[], const U64 M[]);
EXTERN_C void MB_FUNC_NAME(ifma_sub52x5_)(U64 R[], const U64 A[], const U64 B[], const U64 M[]);
EXTERN_C void MB_FUNC_NAME(ifma_neg52x5_)(U64 R[], const U64 A[], const U64 M[]);

/* Specialized operations over NIST P256 */
EXTERN_C void MB_FUNC_NAME(ifma_tomont52_p256_)(U64 r[], const U64 a[]);
EXTERN_C void MB_FUNC_NAME(ifma_frommont52_p256_)(U64 r[], const U64 a[]);
EXTERN_C void MB_FUNC_NAME(ifma_ams52_p256_)(U64 r[], const U64 va[]);
EXTERN_C void MB_FUNC_NAME(ifma_amm52_p256_)(U64 r[], const U64 va[], const U64 vb[]);
EXTERN_C void MB_FUNC_NAME(ifma_aminv52_p256_)(U64 r[], const U64 z[]);
EXTERN_C void MB_FUNC_NAME(ifma_add52_p256_)(U64 r[], const U64 a[], const U64 b[]);
EXTERN_C void MB_FUNC_NAME(ifma_sub52_p256_)(U64 r[], const U64 a[], const U64 b[]);
EXTERN_C void MB_FUNC_NAME(ifma_neg52_p256_)(U64 r[], const U64 a[]);
EXTERN_C void MB_FUNC_NAME(ifma_double52_p256_)(U64 r[], const U64 a[]);
EXTERN_C void MB_FUNC_NAME(ifma_tripple52_p256_)(U64 r[], const U64 a[]);
EXTERN_C void MB_FUNC_NAME(ifma_half52_p256_)(U64 r[], const U64 a[]);

EXTERN_C void MB_FUNC_NAME(ifma_ams52_p256_dual_)(U64 r0[], U64 r1[],
                                            const U64 inp0[], const U64 inp1[]);
EXTERN_C void MB_FUNC_NAME(ifma_amm52_p256_dual_)(U64 r0[], U64 r1[],
                                            const U64 inp0A[], const U64 inp0B[],
                                           const U64 inp1A[], const U64 inp1B[]);

EXTERN_C __mb_mask MB_FUNC_NAME(ifma_cmp_lt_p256_)(const U64 a[]);
EXTERN_C __mb_mask MB_FUNC_NAME(ifma_check_range_p256_)(const U64 a[]);

/* Specialized operations over EC NIST-P256 order */
EXTERN_C U64* MB_FUNC_NAME(ifma_n256_)(void);
EXTERN_C void MB_FUNC_NAME(ifma_tomont52_n256_)(U64 r[], const U64 a[]);
EXTERN_C void MB_FUNC_NAME(ifma_frommont52_n256_)(U64 r[], const U64 a[]);
EXTERN_C void MB_FUNC_NAME(ifma_ams52_n256_)(U64 r[], const U64 a[]);
EXTERN_C void MB_FUNC_NAME(ifma_amm52_n256_)(U64 r[], const U64 a[], const U64 b[]);
EXTERN_C void MB_FUNC_NAME(ifma_aminv52_n256_)(U64 r[], const U64 a[]);
EXTERN_C void MB_FUNC_NAME(ifma_add52_n256_)(U64 r[], const U64 a[], const U64 b[]);
EXTERN_C void MB_FUNC_NAME(ifma_sub52_n256_)(U64 r[], const U64 a[], const U64 b[]);
EXTERN_C void MB_FUNC_NAME(ifma_neg52_n256_)(U64 r[], const U64 a[]);
EXTERN_C void MB_FUNC_NAME(ifma_fastred52_pn256_)(U64 r[], const U64 a[]);
EXTERN_C __mb_mask MB_FUNC_NAME(ifma_cmp_lt_n256_)(const U64 a[]);
EXTERN_C __mb_mask MB_FUNC_NAME(ifma_check_range_n256_)(const U64 a[]);

#endif  /* IFMA_ARITH_P256_H */
