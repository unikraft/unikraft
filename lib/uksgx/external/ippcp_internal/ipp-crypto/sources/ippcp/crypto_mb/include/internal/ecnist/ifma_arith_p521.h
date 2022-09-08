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

#ifndef IFMA_ARITH_P521_H
#define IFMA_ARITH_P521_H

#include <internal/common/ifma_defs.h>
#include <internal/common/ifma_math.h>

/* underlying prime's size */
#define P521_BITSIZE (521)

/* lengths of FF elements */
#define P521_LEN52  NUMBER_OF_DIGITS(P521_BITSIZE,DIGIT_SIZE)
#define P521_LEN64  NUMBER_OF_DIGITS(P521_BITSIZE,64)

__ALIGN64 static const int64u ones[P521_LEN52][sizeof(U64)/sizeof(int64u)] = {
   { REP8_DECL(1) },
   { REP8_DECL(0) },
   { REP8_DECL(0) },
   { REP8_DECL(0) },
   { REP8_DECL(0) },
   { REP8_DECL(0) },
   { REP8_DECL(0) },
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
__INLINE void MB_FUNC_NAME(zero_FE521_)(U64 T[])
{
   T[0] = T[1] = T[2] = T[3] = T[4] = T[5] = T[6] = T[7] = T[8] = T[9] = T[10] = get_zero64();
}

/* check if FE is zero */
__INLINE __mb_mask MB_FUNC_NAME(is_zero_FE521_)(const U64 T[])
{
   U64 Z = or64(or64(or64(or64(T[0], T[1]), or64(T[2], T[3])), or64(or64(T[4], T[5]), or64(T[6], T[7]))), or64(or64(T[8], T[9]), T[10]));
   return cmpeq64_mask(Z, get_zero64());
}

__INLINE U64 cmov_U64(U64 a, U64 b, __mb_mask kmask)
{  return mask_mov64 (a, kmask, b); }

/* move field element */
__INLINE void MB_FUNC_NAME(mov_FE521_)(U64 r[], const U64 a[])
{
   r[0] = a[0];
   r[1] = a[1];
   r[2] = a[2];
   r[3] = a[3];
   r[4] = a[4];
   r[5] = a[5];
   r[6] = a[6];
   r[7] = a[7];
   r[8] = a[8];
   r[9] = a[9];
   r[10]= a[10];
}

/* move coodinate using mask: R = k? A : B */
__INLINE void MB_FUNC_NAME(mask_mov_FE521_)(U64 R[], const U64 B[], __mb_mask k, const U64 A[])
{
   R[0] = mask_mov64(B[0], k, A[0]);
   R[1] = mask_mov64(B[1], k, A[1]);
   R[2] = mask_mov64(B[2], k, A[2]);
   R[3] = mask_mov64(B[3], k, A[3]);
   R[4] = mask_mov64(B[4], k, A[4]);
   R[5] = mask_mov64(B[5], k, A[5]);
   R[6] = mask_mov64(B[6], k, A[6]);
   R[7] = mask_mov64(B[7], k, A[7]);
   R[8] = mask_mov64(B[8], k, A[8]);
   R[9] = mask_mov64(B[9], k, A[9]);
   R[10]= mask_mov64(B[10],k, A[10]);
}

__INLINE void MB_FUNC_NAME(secure_mask_mov_FE521_)(U64 R[], U64 B[], __mb_mask k, const U64 A[])
{
   R[0] = select64(k, B[0], (U64*)(&A[0]));
   R[1] = select64(k, B[1], (U64*)(&A[1]));
   R[2] = select64(k, B[2], (U64*)(&A[2]));
   R[3] = select64(k, B[3], (U64*)(&A[3]));
   R[4] = select64(k, B[4], (U64*)(&A[4]));
   R[5] = select64(k, B[5], (U64*)(&A[5]));
   R[6] = select64(k, B[6], (U64*)(&A[6]));
   R[7] = select64(k, B[7], (U64*)(&A[7]));
   R[8] = select64(k, B[8], (U64*)(&A[8]));
   R[9] = select64(k, B[9], (U64*)(&A[9]));
   R[10]= select64(k,B[10], (U64*)(&A[10]));
}

__INLINE __mb_mask MB_FUNC_NAME(cmp_lt_FE521_)(const U64 A[], const U64 B[])
{
   /* r = a - b */
   U64 r0 = sub64(A[0], B[0]);
   U64 r1 = sub64(A[1], B[1]);
   U64 r2 = sub64(A[2], B[2]);
   U64 r3 = sub64(A[3], B[3]);
   U64 r4 = sub64(A[4], B[4]);
   U64 r5 = sub64(A[5], B[5]);
   U64 r6 = sub64(A[6], B[6]);
   U64 r7 = sub64(A[7], B[7]);
   U64 r8 = sub64(A[8], B[8]);
   U64 r9 = sub64(A[9], B[9]);
   U64 r10= sub64(A[10],B[10]);

   /* normalize r0 – r10 */
   NORM_ASHIFTR(r, 0, 1)
   NORM_ASHIFTR(r, 1, 2)
   NORM_ASHIFTR(r, 2, 3)
   NORM_ASHIFTR(r, 3, 4)
   NORM_ASHIFTR(r, 4, 5)
   NORM_ASHIFTR(r, 5, 6)
   NORM_ASHIFTR(r, 6, 7)
   NORM_ASHIFTR(r, 7, 8)
   NORM_ASHIFTR(r, 8, 9)
   NORM_ASHIFTR(r, 9,10)

   /* return mask LT */
   return cmp64_mask(r10, get_zero64(), _MM_CMPINT_LT);
}

__INLINE __mb_mask MB_FUNC_NAME(cmp_eq_FE521_)(const U64 A[], const U64 B[])
{
    U64 T[P521_LEN52];

    T[0] = xor64(A[0], B[0]);
    T[1] = xor64(A[1], B[1]);
    T[2] = xor64(A[2], B[2]);
    T[3] = xor64(A[3], B[3]);
    T[4] = xor64(A[4], B[4]);
    T[5] = xor64(A[5], B[5]);
    T[6] = xor64(A[6], B[6]);
    T[7] = xor64(A[7], B[7]);
    T[8] = xor64(A[8], B[8]);
    T[9] = xor64(A[9], B[9]);
    T[10] = xor64(A[10], B[10]);

    return MB_FUNC_NAME(is_zero_FE521_)(T);
}

/* Specialized operations over NIST P521 */
EXTERN_C void MB_FUNC_NAME(ifma_tomont52_p521_)(U64 r[], const U64 a[]);
EXTERN_C void MB_FUNC_NAME(ifma_frommont52_p521_)(U64 r[], const U64 a[]);
EXTERN_C void MB_FUNC_NAME(ifma_ams52_p521_)(U64 r[], const U64 va[]);
EXTERN_C void MB_FUNC_NAME(ifma_amm52_p521_)(U64 r[], const U64 va[], const U64 vb[]);
EXTERN_C void MB_FUNC_NAME(ifma_aminv52_p521_)(U64 r[], const U64 z[]);
EXTERN_C void MB_FUNC_NAME(ifma_add52_p521_)(U64 r[], const U64 a[], const U64 b[]);
EXTERN_C void MB_FUNC_NAME(ifma_sub52_p521_)(U64 r[], const U64 a[], const U64 b[]);
EXTERN_C void MB_FUNC_NAME(ifma_neg52_p521_)(U64 r[], const U64 a[]);
EXTERN_C void MB_FUNC_NAME(ifma_double52_p521_)(U64 r[], const U64 a[]);
EXTERN_C void MB_FUNC_NAME(ifma_tripple52_p521_)(U64 r[], const U64 a[]);
EXTERN_C void MB_FUNC_NAME(ifma_half52_p521_)(U64 r[], const U64 a[]);
EXTERN_C __mb_mask MB_FUNC_NAME(ifma_cmp_lt_p521_)(const U64 a[]);
EXTERN_C __mb_mask MB_FUNC_NAME(ifma_check_range_p521_)(const U64 a[]);

/* Specialized operations over EC NIST-P521 order */
EXTERN_C U64* MB_FUNC_NAME(ifma_n521_)(void);
EXTERN_C void MB_FUNC_NAME(ifma_tomont52_n521_)(U64 r[], const U64 a[]);
EXTERN_C void MB_FUNC_NAME(ifma_frommont52_n521_)(U64 r[], const U64 a[]);
EXTERN_C void MB_FUNC_NAME(ifma_ams52_n521_)(U64 r[], const U64 a[]);
EXTERN_C void MB_FUNC_NAME(ifma_amm52_n521_)(U64 r[], const U64 a[], const U64 b[]);
EXTERN_C void MB_FUNC_NAME(ifma_aminv52_n521_)(U64 r[], const U64 a[]);
EXTERN_C void MB_FUNC_NAME(ifma_add52_n521_)(U64 r[], const U64 a[], const U64 b[]);
EXTERN_C void MB_FUNC_NAME(ifma_sub52_n521_)(U64 r[], const U64 a[], const U64 b[]);
EXTERN_C void MB_FUNC_NAME(ifma_neg52_n521_)(U64 r[], const U64 a[]);
EXTERN_C void MB_FUNC_NAME(ifma_fastred52_pn521_)(U64 r[], const U64 a[]);
EXTERN_C __mb_mask MB_FUNC_NAME(ifma_cmp_lt_n521_)(const U64 a[]);
EXTERN_C __mb_mask MB_FUNC_NAME(ifma_check_range_n521_)(const U64 a[]);

#endif  /* IFMA_ARITH_P521_H */
