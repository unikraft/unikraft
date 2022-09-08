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

#include <internal/common/ifma_math.h>
#include <internal/rsa/ifma_rsa_arith.h>


#define USE_AMS
#ifdef USE_AMS
    #define SQUARE_52x60_mb8(out, Y, mod, k0) \
         AMS52x60_diagonal_mb8((int64u*)out, (int64u*)Y, (int64u*)mod, (int64u*)k0);

    #define SQUARE_5x52x60_mb8(out, Y, mod, k0) \
         AMS52x60_diagonal_mb8((int64u*)out, (int64u*)Y, (int64u*)mod, (int64u*)k0); \
         AMS52x60_diagonal_mb8((int64u*)out, (int64u*)out, (int64u*)mod, (int64u*)k0); \
         AMS52x60_diagonal_mb8((int64u*)out, (int64u*)out, (int64u*)mod, (int64u*)k0); \
         AMS52x60_diagonal_mb8((int64u*)out, (int64u*)out, (int64u*)mod, (int64u*)k0); \
         AMS52x60_diagonal_mb8((int64u*)out, (int64u*)out, (int64u*)mod, (int64u*)k0);
#else
    #define SQUARE_52x60_mb8(out, Y, mod, k0) \
         ifma_amm52x60_mb8((int64u*)out, (int64u*)Y, (int64u*)Y, (int64u*)mod, (int64u*)k0);
    #define SQUARE_5x52x60_mb8(out, Y, mod, k0) \
         ifma_amm52x60_mb8((int64u*)out, (int64u*)Y, (int64u*)Y, (int64u*)mod, (int64u*)k0); \
         ifma_amm52x60_mb8((int64u*)out, (int64u*)out, (int64u*)out, (int64u*)mod, (int64u*)k0); \
         ifma_amm52x60_mb8((int64u*)out, (int64u*)out, (int64u*)out, (int64u*)mod, (int64u*)k0); \
         ifma_amm52x60_mb8((int64u*)out, (int64u*)out, (int64u*)out, (int64u*)mod, (int64u*)k0); \
         ifma_amm52x60_mb8((int64u*)out, (int64u*)out, (int64u*)out, (int64u*)mod, (int64u*)k0);
#endif

#define BITSIZE_MODULUS (RSA_3K)
#define LEN52           (NUMBER_OF_DIGITS(BITSIZE_MODULUS,DIGIT_SIZE))  //60

void EXP52x60_pub65537_mb8(int64u out[][8],
                     const int64u base[][8],
                     const int64u modulus[][8],
                     const int64u toMont[][8],
                     const int64u k0[8],
                     int64u work_buffer[][8])
{
   /* allocate red(undant) result Y and multiplier X */
   pint64u_x8 red_Y = (pint64u_x8)work_buffer;
   pint64u_x8 red_X = (pint64u_x8)(work_buffer + LEN52);

   /* convert base into redundant domain */
   ifma_amm52x60_mb8((int64u*)red_X, (int64u*)base, (int64u*)toMont, (int64u*)modulus, (int64u*)k0);

   /* exponentition 65537 = 0x10001 */
   SQUARE_52x60_mb8((int64u*)red_Y, (int64u*)red_X, (int64u*)modulus, (int64u*)k0);
   SQUARE_5x52x60_mb8((int64u*)red_Y, (int64u*)red_Y, (int64u*)modulus, (int64u*)k0);
   SQUARE_5x52x60_mb8((int64u*)red_Y, (int64u*)red_Y, (int64u*)modulus, (int64u*)k0);
   SQUARE_5x52x60_mb8((int64u*)red_Y, (int64u*)red_Y, (int64u*)modulus, (int64u*)k0);
   ifma_amm52x60_mb8((int64u*)red_Y, (int64u*)red_Y, (int64u*)red_X, (int64u*)modulus, (int64u*)k0);

   /* convert result back in regular 2^52 domain */
   zero_mb8(red_X, LEN52);
   _mm512_store_si512(red_X, _mm512_set1_epi64(1));
   ifma_amm52x60_mb8((int64u*)out, (int64u*)red_Y, (int64u*)red_X, (int64u*)modulus, (int64u*)k0);
}
