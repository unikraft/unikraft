/*******************************************************************************
* Copyright 2019-2021 Intel Corporation
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

#ifndef IFMA_RSA_ARITH_H
#define IFMA_RSA_ARITH_H

#ifndef BN_OPENSSL_DISABLE
  #include <openssl/bn.h>
#endif

#include <internal/common/ifma_defs.h>

typedef int64u int64u_x8[8];        // alias   of 8-term vector of int64u each
typedef int64u (*pint64u_x8) [8];   // pointer to 8-term vector of int64u each

/* fixed size of RSA */
#define RSA_1K (1024)
#define RSA_2K (2*RSA_1K)
#define RSA_3K (3*RSA_1K)
#define RSA_4K (4*RSA_1K)

#define NUMBER_OF_DIGITS(bitsize, digsize)   (((bitsize) + (digsize)-1)/(digsize))
//#define MS_DIGIT_MASK(bitsize, digsize)      (((int64u)1 <<((bitsize) %digsize)) -1)

//#define RSA_SIZE  (RSA_1K)

#define MULTIPLE_OF(x, factor)   ((x) + (((factor) -((x)%(factor))) %(factor)))

// ============ Multi-Buffer required functions ============

#define redLen    ((RSA_1K+(DIGIT_SIZE-1))/DIGIT_SIZE) //20
EXTERN_C void ifma_extract_amm52x20_mb8(int64u* out_mb8, const int64u* inpA_mb8, int64u MulTbl[][redLen][8], const int64u Idx[8], const int64u* inpM_mb8, const int64u* k0_mb8);

// Multiplication
EXTERN_C void ifma_amm52x10_mb8(int64u* out_mb8, const int64u* inpA_mb8, const int64u* inpB_mb8, const int64u* inpM_mb8, const int64u* k0_mb8);
EXTERN_C void ifma_amm52x20_mb8(int64u* out_mb8, const int64u* inpA_mb8, const int64u* inpB_mb8, const int64u* inpM_mb8, const int64u* k0_mb8);
EXTERN_C void ifma_amm52x60_mb8(int64u* out_mb8, const int64u* inpA_mb8, const int64u* inpB_mb8, const int64u* inpM_mb8, const int64u* k0_mb8);
EXTERN_C void ifma_amm52x40_mb8(int64u* out_mb8, const int64u* inpA_mb8, const int64u* inpB_mb8, const int64u* inpM_mb8, const int64u* k0_mb8);
EXTERN_C void ifma_amm52x30_mb8(int64u* out_mb8, const int64u* inpA_mb8, const int64u* inpB_mb8, const int64u* inpM_mb8, const int64u* k0_mb8);
EXTERN_C void ifma_amm52x79_mb8(int64u* out_mb8, const int64u* inpA_mb8, const int64u* inpB_mb8, const int64u* inpM_mb8, const int64u* k0_mb8);

// 4x Mont Mul
EXTERN_C void ifma_amm52x20_mb4(int64u* out_mb8, const int64u* inpA_mb8, const int64u* inpB_mb8, const int64u* inpM_mb8, const int64u* k0_mb8);


// Diagonal sqr
EXTERN_C void AMS52x10_diagonal_mb8(int64u* out_mb8, const int64u* inpA_mb8, const int64u* inpM_mb8, const int64u* k0_mb8);
EXTERN_C void AMS5x52x10_diagonal_mb8(int64u* out_mb8, const int64u* inpA_mb8, const int64u* inpM_mb8, const int64u* k0_mb8);

EXTERN_C void AMS52x20_diagonal_mb8(int64u* out_mb8, const int64u* inpA_mb8, const int64u* inpM_mb8, const int64u* k0_mb8);
EXTERN_C void AMS5x52x20_diagonal_mb8(int64u* out_mb8, const int64u* inpA_mb8, const int64u* inpM_mb8, const int64u* k0_mb8);

EXTERN_C void AMS52x40_diagonal_mb8(int64u* out_mb8, const int64u* inpA_mb8, const int64u* inpM_mb8, const int64u* k0_mb8);
EXTERN_C void AMS5x52x40_diagonal_mb8(int64u* out_mb8, const int64u* inpA_mb8, const int64u* inpM_mb8, const int64u* k0_mb8);

EXTERN_C void AMS52x60_diagonal_mb8(int64u* out_mb8, const int64u* inpA_mb8, const int64u* inpM_mb8, const int64u* k0_mb8);

EXTERN_C void AMS52x30_diagonal_mb8(int64u* out_mb8, const int64u* inpA_mb8, const int64u* inpM_mb8, const int64u* k0_mb8);

EXTERN_C void AMS52x79_diagonal_mb8(int64u* out_mb8, const int64u* inpA_mb8, const int64u* inpM_mb8, const int64u* k0_mb8);

// 4x Diagonal sqr
EXTERN_C void AMS52x20_diagonal_mb4(int64u* out_mb8, const int64u* inpA_mb8, const int64u* inpM_mb8, const int64u* k0_mb8);
EXTERN_C void AMS5x52x20_diagonal_mb4(int64u* out_mb8, const int64u* inpA_mb8, const int64u* inpM_mb8, const int64u* k0_mb8);


// clear/copy mb8 buffer
EXTERN_C void zero_mb8(int64u(*redOut)[8], int len);
EXTERN_C void copy_mb8(int64u out[][8], const int64u inp[][8], int len);


// other 2^52 radix arith functions
EXTERN_C void ifma_montFactor52_mb8(int64u k0_mb8[8], const int64u m0_mb8[8]);

EXTERN_C void ifma_modsub52x10_mb8(int64u res[][8], const int64u inpA[][8], const int64u inpB[][8], const int64u inpM[][8]);
EXTERN_C void ifma_modsub52x20_mb8(int64u res[][8], const int64u inpA[][8], const int64u inpB[][8], const int64u inpM[][8]);
EXTERN_C void ifma_modsub52x30_mb8(int64u res[][8], const int64u inpA[][8], const int64u inpB[][8], const int64u inpM[][8]);
EXTERN_C void ifma_modsub52x40_mb8(int64u res[][8], const int64u inpA[][8], const int64u inpB[][8], const int64u inpM[][8]);

EXTERN_C void ifma_addmul52x10_mb8(int64u res[][8], const int64u inpA[][8], const int64u inpB[][8]);
EXTERN_C void ifma_addmul52x20_mb8(int64u res[][8], const int64u inpA[][8], const int64u inpB[][8]);
EXTERN_C void ifma_addmul52x30_mb8(int64u res[][8], const int64u inpA[][8], const int64u inpB[][8]);
EXTERN_C void ifma_addmul52x40_mb8(int64u res[][8], const int64u inpA[][8], const int64u inpB[][8]);

EXTERN_C void ifma_amred52x10_mb8(int64u res[][8], const int64u inpA[][8], const int64u inpM[][8], const int64u k0[8]);
EXTERN_C void ifma_amred52x20_mb8(int64u res[][8], const int64u inpA[][8], const int64u inpM[][8], const int64u k0[8]);
EXTERN_C void ifma_amred52x30_mb8(int64u res[][8], const int64u inpA[][8], const int64u inpM[][8], const int64u k0[8]);
EXTERN_C void ifma_amred52x40_mb8(int64u res[][8], const int64u inpA[][8], const int64u inpM[][8], const int64u k0[8]);

EXTERN_C void ifma_mreduce52x_mb8(int64u pX[][8], int nsX, int64u pM[][8], int nsM);
EXTERN_C void ifma_montRR52x_mb8(int64u pRR[][8], int64u pM[][8], int convBitLen);


// exponentiations
EXTERN_C void EXP52x10_mb8(int64u out[][8],
                     const int64u base[][8],
                     const int64u exponent[][8],
                     const int64u modulus[][8],
                     const int64u toMont[][8],
                     const int64u k0_mb8[8],
                     int64u work_buffer[][8]);

EXTERN_C void EXP52x20_mb8(int64u out[][8],
                     const int64u base[][8],
                     const int64u exponent[][8],
                     const int64u modulus[][8],
                     const int64u toMont[][8],
                     const int64u k0_mb8[8],
                     int64u work_buffer[][8]);

EXTERN_C void EXP52x40_mb8(int64u out[][8],
                     const int64u base[][8],
                     const int64u exponent[][8],
                     const int64u modulus[][8],
                     const int64u toMont[][8],
                     const int64u k0_mb8[8],
                     int64u work_buffer[][8]);

EXTERN_C void EXP52x60_mb8(int64u out[][8],
                     const int64u base[][8],
                     const int64u exponent[][8],
                     const int64u modulus[][8],
                     const int64u toMont[][8],
                     const int64u k0_mb8[8],
                     int64u work_buffer[][8]);

EXTERN_C void EXP52x30_mb8(int64u out[][8],
                     const int64u base[][8],
                     const int64u exponent[][8],
                     const int64u modulus[][8],
                     const int64u toMont[][8],
                     const int64u k0_mb8[8],
                     int64u work_buffer[][8]);

EXTERN_C void EXP52x79_mb8(int64u out[][8],
                     const int64u base[][8],
                     const int64u exponent[][8],
                     const int64u modulus[][8],
                     const int64u toMont[][8],
                     const int64u k0_mb8[8],
                     int64u work_buffer[][8]);

// exponentiations (fixed short exponent ==65537)
EXTERN_C void EXP52x20_pub65537_mb8(int64u out[][8],
                              const int64u base[][8],
                              const int64u modulus[][8],
                              const int64u toMont[][8],
                              const int64u  k0[8],
                              int64u work_buffer[][8]);


EXTERN_C void EXP52x40_pub65537_mb8(int64u out[][8],
                              const int64u base[][8],
                              const int64u modulus[][8],
                              const int64u toMont[][8],
                              const int64u  k0[8],
                              int64u work_buffer[][8]);

EXTERN_C void EXP52x60_pub65537_mb8(int64u out[][8],
                              const int64u base[][8],
                              const int64u modulus[][8],
                              const int64u toMont[][8],
                              const int64u  k0[8],
                              int64u work_buffer[][8]);

EXTERN_C void EXP52x79_pub65537_mb8(int64u out[][8],
                              const int64u base[][8],
                              const int64u modulus[][8],
                              const int64u toMont[][8],
                              const int64u  k0[8],
                              int64u work_buffer[][8]);

#endif /* _IFMA_INTERNAL_H_ */
