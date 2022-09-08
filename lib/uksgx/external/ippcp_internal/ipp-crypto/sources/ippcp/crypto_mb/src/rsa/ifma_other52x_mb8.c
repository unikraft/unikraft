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

#include <internal/common/ifma_defs.h>
#include <internal/common/ifma_math.h>
#include <internal/rsa/ifma_rsa_arith.h>

#ifdef __GNUC__
#define ASM(a) __asm__(a);
#else
#define ASM(a)
#endif

__NOINLINE
void zero_mb8(int64u (*out)[8], int len)
{
#if defined(__GNUC__)
   // Avoid dead code elimination for GNU compilers
   ASM("");
#endif
   __m512i T = _mm512_setzero_si512();
   int i;
   for(i=0; i<len; i++)
      _mm512_store_si512(out[i], T);
}

void copy_mb8(int64u out[][8], const int64u inp[][8], int len)
{
   int i;
   for(i=0; i<len; i++)
      _mm512_store_si512(out[i], _mm512_load_si512(inp[i]));
}

/* k0 = - ( m0 ^(-1) mod 2^52 ) */
void ifma_montFactor52_mb8(int64u k0_mb8[8], const int64u m0_mb8[8])
{
   __m512i m0 = _mm512_loadu_si512(m0_mb8);

   __m512i y = _mm512_set1_epi64(1);                           /*     1 */
   __m512i x = _mm512_set1_epi64(2);                           /*     2 */
   __m512i nx = _mm512_add_epi64(x, x);
   __m512i mask = _mm512_sub_epi64(nx, y);                    /* 2*x-1 */


   int n;
   for(n=2; n<=DIGIT_SIZE; n++ /*, x=nx , nx=_mm512_add_epi64(nx,nx)*/ ) {
      __m512i rL = _mm512_madd52lo_epu64(_mm512_setzero_si512(), m0, y);               /* rL = m0*y; */
      __mmask8 k = _mm512_cmplt_epu64_mask(x, _mm512_and_si512(rL, mask));             /* if( x < (rL & mask) )  < == > x < ((m0*y) mod (2*x)) */
      y = _mm512_mask_add_epi64(y, k, y,x);                                            /*    y+=x */
      /* mask = 2*x-1 */
      x  = nx;
      nx = _mm512_add_epi64(nx, nx);
      mask = _mm512_sub_epi64(nx, _mm512_set1_epi64(1));
   }
   y = _mm512_sub_epi64(_mm512_setzero_si512(), y);                                    /* return (0-y)     */
   y = _mm512_and_si512(y, _mm512_set1_epi64(DIGIT_MASK));

   _mm512_storeu_si512(k0_mb8,  y);
}

/* r = (a-b) mod m */
void ifma_modsub52x10_mb8(int64u res[][8], const int64u inpA[][8], const int64u inpB[][8], const int64u inpM[][8])
{
#define BITSIZE   (512)
#define LEN52     (NUMBER_OF_DIGITS(BITSIZE,52))
#define MSD_MSK   (MS_DIGIT_MASK(BITSIZE,52))

   __m512i* pr = (__m512i*)res;
   __m512i* pa = (__m512i*)inpA;
   __m512i* pb = (__m512i*)inpB;
   __m512i* pm = (__m512i*)inpM;

   __m512i DIG_MASK = _mm512_set1_epi64(DIGIT_MASK);
   __m512i MSD_MASK = _mm512_set1_epi64(MSD_MSK);

   __m512i T[LEN52];
   __m512i R, CARRY, BORROW;
   int n;

   /* T[] = a[] - b[] */
   BORROW = _mm512_setzero_si512();

   for (n = 0; n < LEN52; n++) {
      R = _mm512_sub_epi64(pa[n], pb[n]);
      R = _mm512_sub_epi64(R, BORROW);
      BORROW = _mm512_srli_epi64(R, (64 - 1));
      R = _mm512_and_epi64(R, DIG_MASK);
      _mm512_store_si512(T + n, R);
   }

   /* correct last digit */
   R = _mm512_and_epi64(R, MSD_MASK);
   _mm512_store_si512(T + LEN52 - 1, R);

   /* masked modulus add: r[] = T[] + BORROW? m[] : 0 */
   CARRY = _mm512_setzero_si512();
   BORROW = _mm512_sub_epi64(CARRY, BORROW); /* RORROW -> mask */

   for (n = 0; n < LEN52; n++) {
      R = _mm512_and_epi64(BORROW, pm[n]);
      R = _mm512_add_epi64(R, T[n]);
      R = _mm512_add_epi64(R, CARRY);
      CARRY = _mm512_srli_epi64(R, DIGIT_SIZE);
      R = _mm512_and_epi64(R, DIG_MASK);
      _mm512_store_si512(pr + n, R);
   }
   /* correct last digit */
   R = _mm512_and_epi64(R, MSD_MASK);
   _mm512_store_si512(pr + LEN52 - 1, R);

#undef BITSIZE
#undef LEN52
#undef MSD_MSK
}

void ifma_modsub52x20_mb8(int64u res[][8], const int64u inpA[][8], const int64u inpB[][8], const int64u inpM[][8])
{
#define BITSIZE   (1024)
#define LEN52     (NUMBER_OF_DIGITS(BITSIZE,52))
#define MSD_MSK   (MS_DIGIT_MASK(BITSIZE,52))

   __m512i* pr = (__m512i*)res;
   __m512i* pa = (__m512i*)inpA;
   __m512i* pb = (__m512i*)inpB;
   __m512i* pm = (__m512i*)inpM;

   __m512i DIG_MASK = _mm512_set1_epi64(DIGIT_MASK);
   __m512i MSD_MASK = _mm512_set1_epi64(MSD_MSK);

   __m512i T[LEN52];
   __m512i R, CARRY, BORROW;
   int n;

   /* T[] = a[] - b[] */
   BORROW = _mm512_setzero_si512();

   for (n = 0; n < LEN52; n++) {
      R = _mm512_sub_epi64(pa[n], pb[n]);
      R = _mm512_sub_epi64(R, BORROW);
      BORROW = _mm512_srli_epi64(R, (64 - 1));
      R = _mm512_and_epi64(R, DIG_MASK);
      _mm512_store_si512(T + n, R);
   }

   /* correct last digit */
   R = _mm512_and_epi64(R, MSD_MASK);
   _mm512_store_si512(T + LEN52 - 1, R);

   /* masked modulus add: r[] = T[] + BORROW? m[] : 0 */
   CARRY = _mm512_setzero_si512();
   BORROW = _mm512_sub_epi64(CARRY, BORROW); /* RORROW -> mask */

   for (n = 0; n < LEN52; n++) {
      R = _mm512_and_epi64(BORROW, pm[n]);
      R = _mm512_add_epi64(R, T[n]);
      R = _mm512_add_epi64(R, CARRY);
      CARRY = _mm512_srli_epi64(R, DIGIT_SIZE);
      R = _mm512_and_epi64(R, DIG_MASK);
      _mm512_store_si512(pr + n, R);
   }
   /* correct last digit */
   R = _mm512_and_epi64(R, MSD_MASK);
   _mm512_store_si512(pr + LEN52 - 1, R);

   #undef BITSIZE
   #undef LEN52
   #undef MSD_MSK
}

void ifma_modsub52x30_mb8(int64u res[][8], const int64u inpA[][8], const int64u inpB[][8], const int64u inpM[][8])
{
   #define BITSIZE   (1536)
   #define LEN52     (NUMBER_OF_DIGITS(BITSIZE,52))
   #define MSD_MSK   (MS_DIGIT_MASK(BITSIZE,52))

   __m512i* pr = (__m512i*)res;
   __m512i* pa = (__m512i*)inpA;
   __m512i* pb = (__m512i*)inpB;
   __m512i* pm = (__m512i*)inpM;

   __m512i DIG_MASK = _mm512_set1_epi64(DIGIT_MASK);
   __m512i MSD_MASK = _mm512_set1_epi64(MSD_MSK);

   __m512i T[LEN52];
   __m512i R, CARRY, BORROW;
   int n;

   /* T[] = a[] - b[] */
   BORROW = _mm512_setzero_si512();

   for (n = 0; n < LEN52; n++) {
      R = _mm512_sub_epi64(pa[n], pb[n]);
      R = _mm512_sub_epi64(R, BORROW);
      BORROW = _mm512_srli_epi64(R, (64 - 1));
      R = _mm512_and_epi64(R, DIG_MASK);
      _mm512_store_si512(T + n, R);
   }

   /* correct last digit */
   R = _mm512_and_epi64(R, MSD_MASK);
   _mm512_store_si512(T + LEN52 - 1, R);

   /* masked modulus add: r[] = T[] + BORROW? m[] : 0 */
   CARRY = _mm512_setzero_si512();
   BORROW = _mm512_sub_epi64(CARRY, BORROW); /* RORROW -> mask */

   for (n = 0; n < LEN52; n++) {
      R = _mm512_and_epi64(BORROW, pm[n]);
      R = _mm512_add_epi64(R, T[n]);
      R = _mm512_add_epi64(R, CARRY);
      CARRY = _mm512_srli_epi64(R, DIGIT_SIZE);
      R = _mm512_and_epi64(R, DIG_MASK);
      _mm512_store_si512(pr + n, R);
   }
   /* correct last digit */
   R = _mm512_and_epi64(R, MSD_MASK);
   _mm512_store_si512(pr + LEN52 - 1, R);

   #undef BITSIZE
   #undef LEN52
   #undef MSD_MSK
}

void ifma_modsub52x40_mb8(int64u res[][8], const int64u inpA[][8], const int64u inpB[][8], const int64u inpM[][8])
{
#define BITSIZE   (2048)
#define LEN52     (NUMBER_OF_DIGITS(BITSIZE,52))
#define MSD_MSK   (MS_DIGIT_MASK(BITSIZE,52))

   __m512i* pr = (__m512i*)res;
   __m512i* pa = (__m512i*)inpA;
   __m512i* pb = (__m512i*)inpB;
   __m512i* pm = (__m512i*)inpM;

   __m512i DIG_MASK = _mm512_set1_epi64(DIGIT_MASK);
   __m512i MSD_MASK = _mm512_set1_epi64(MSD_MSK);

   __m512i T[LEN52];
   __m512i R, CARRY, BORROW;
   int n;

   /* T[] = a[] - b[] */
   BORROW = _mm512_setzero_si512();

   for (n = 0; n < LEN52; n++) {
      R = _mm512_sub_epi64(pa[n], pb[n]);
      R = _mm512_sub_epi64(R, BORROW);
      BORROW = _mm512_srli_epi64(R, (64 - 1));
      R = _mm512_and_epi64(R, DIG_MASK);
      _mm512_store_si512(T + n, R);
   }

   /* correct last digit */
   R = _mm512_and_epi64(R, MSD_MASK);
   _mm512_store_si512(T + LEN52 - 1, R);

   /* masked modulus add: r[] = T[] + BORROW? m[] : 0 */
   CARRY = _mm512_setzero_si512();
   BORROW = _mm512_sub_epi64(CARRY, BORROW); /* RORROW -> mask */

   for (n = 0; n < LEN52; n++) {
      R = _mm512_and_epi64(BORROW, pm[n]);
      R = _mm512_add_epi64(R, T[n]);
      R = _mm512_add_epi64(R, CARRY);
      CARRY = _mm512_srli_epi64(R, DIGIT_SIZE);
      R = _mm512_and_epi64(R, DIG_MASK);
      _mm512_store_si512(pr + n, R);
   }
   /* correct last digit */
   R = _mm512_and_epi64(R, MSD_MASK);
   _mm512_store_si512(pr + LEN52 - 1, R);

#undef BITSIZE
#undef LEN52
#undef MSD_MSK
}


/* r += a*b */
void ifma_addmul52x10_mb8(int64u pRes[][8], const int64u inpA[][8], const int64u inpB[][8])
{
#define BITSIZE   (RSA_1K/2)
#define LEN52     (NUMBER_OF_DIGITS(BITSIZE,52))
#define MSD_MSK   (MS_DIGIT_MASK(RSA_2K,52))

   __m512i* pr = (__m512i*)pRes;
   __m512i* pa = (__m512i*)inpA;
   __m512i* pb = (__m512i*)inpB;

   __m512i DIG_MASK = _mm512_set1_epi64(DIGIT_MASK);

   __m512i R00 = _mm512_load_si512(pr + 0); /* load pr[nsM-1],...,R[0] */
   __m512i R01 = _mm512_load_si512(pr + 1);
   __m512i R02 = _mm512_load_si512(pr + 2);
   __m512i R03 = _mm512_load_si512(pr + 3);
   __m512i R04 = _mm512_load_si512(pr + 4);
   __m512i R05 = _mm512_load_si512(pr + 5);
   __m512i R06 = _mm512_load_si512(pr + 6);
   __m512i R07 = _mm512_load_si512(pr + 7);
   __m512i R08 = _mm512_load_si512(pr + 8);
   __m512i R09 = _mm512_load_si512(pr + 9);

   int itr;
   for (itr = 0; itr < LEN52; itr++) {
      __m512i Bi = _mm512_load_si512(pb);
      __m512i nxtR = _mm512_load_si512(pr + LEN52);
      pb++;

      _mm512_madd52lo_epu64_(R00, R00, Bi, pa, 64 * 0);
      _mm512_madd52lo_epu64_(R01, R01, Bi, pa, 64 * 1);
      _mm512_madd52lo_epu64_(R02, R02, Bi, pa, 64 * 2);
      _mm512_madd52lo_epu64_(R03, R03, Bi, pa, 64 * 3);
      _mm512_madd52lo_epu64_(R04, R04, Bi, pa, 64 * 4);
      _mm512_madd52lo_epu64_(R05, R05, Bi, pa, 64 * 5);
      _mm512_madd52lo_epu64_(R06, R06, Bi, pa, 64 * 6);
      _mm512_madd52lo_epu64_(R07, R07, Bi, pa, 64 * 7);
      _mm512_madd52lo_epu64_(R08, R08, Bi, pa, 64 * 8);
      _mm512_madd52lo_epu64_(R09, R09, Bi, pa, 64 * 9);

      _mm512_store_si512(pr, _mm512_and_epi64(R00, DIG_MASK)); /* store normalized result */
      pr++;

      R00 = _mm512_srli_epi64(R00, DIGIT_SIZE);
      R01 = _mm512_add_epi64(R01, R00);

      _mm512_madd52hi_epu64_(R00, R01, Bi, pa, 64 * 0);
      _mm512_madd52hi_epu64_(R01, R02, Bi, pa, 64 * 1);
      _mm512_madd52hi_epu64_(R02, R03, Bi, pa, 64 * 2);
      _mm512_madd52hi_epu64_(R03, R04, Bi, pa, 64 * 3);
      _mm512_madd52hi_epu64_(R04, R05, Bi, pa, 64 * 4);
      _mm512_madd52hi_epu64_(R05, R06, Bi, pa, 64 * 5);
      _mm512_madd52hi_epu64_(R06, R07, Bi, pa, 64 * 6);
      _mm512_madd52hi_epu64_(R07, R08, Bi, pa, 64 * 7);
      _mm512_madd52hi_epu64_(R08, R09, Bi, pa, 64 * 8);
      _mm512_madd52hi_epu64_(R09, nxtR, Bi, pa, 64 * 9);
   }
   /* normalization */
   {
      __m512i
         T = _mm512_srli_epi64(R00, DIGIT_SIZE);
      R00 = _mm512_and_epi64(R00, DIG_MASK);
      _mm512_store_si512(pr + 0, R00);

      R01 = _mm512_add_epi64(R01, T);
      T = _mm512_srli_epi64(R01, DIGIT_SIZE);
      R01 = _mm512_and_epi64(R01, DIG_MASK);
      _mm512_store_si512(pr + 1, R01);

      R02 = _mm512_add_epi64(R02, T);
      T = _mm512_srli_epi64(R02, DIGIT_SIZE);
      R02 = _mm512_and_epi64(R02, DIG_MASK);
      _mm512_store_si512(pr + 2, R02);

      R03 = _mm512_add_epi64(R03, T);
      T = _mm512_srli_epi64(R03, DIGIT_SIZE);
      R03 = _mm512_and_epi64(R03, DIG_MASK);
      _mm512_store_si512(pr + 3, R03);

      R04 = _mm512_add_epi64(R04, T);
      T = _mm512_srli_epi64(R04, DIGIT_SIZE);
      R04 = _mm512_and_epi64(R04, DIG_MASK);
      _mm512_store_si512(pr + 4, R04);

      R05 = _mm512_add_epi64(R05, T);
      T = _mm512_srli_epi64(R05, DIGIT_SIZE);
      R05 = _mm512_and_epi64(R05, DIG_MASK);
      _mm512_store_si512(pr + 5, R05);

      R06 = _mm512_add_epi64(R06, T);
      T = _mm512_srli_epi64(R06, DIGIT_SIZE);
      R06 = _mm512_and_epi64(R06, DIG_MASK);
      _mm512_store_si512(pr + 6, R06);

      R07 = _mm512_add_epi64(R07, T);
      T = _mm512_srli_epi64(R07, DIGIT_SIZE);
      R07 = _mm512_and_epi64(R07, DIG_MASK);
      _mm512_store_si512(pr + 7, R07);

      R08 = _mm512_add_epi64(R08, T);
      T = _mm512_srli_epi64(R08, DIGIT_SIZE);
      R08 = _mm512_and_epi64(R08, DIG_MASK);
      _mm512_store_si512(pr + 8, R08);

      R09 = _mm512_add_epi64(R09, T);
      T = _mm512_srli_epi64(R09, DIGIT_SIZE);
      R09 = _mm512_and_epi64(R09, DIG_MASK);
      _mm512_store_si512(pr + 9, R09);
   }

#undef BITSIZE
#undef LEN52
#undef MSD_MSK
}

void ifma_addmul52x20_mb8(int64u pRes[][8], const int64u inpA[][8], const int64u inpB[][8])
{
   #define BITSIZE   (RSA_2K/2)
   #define LEN52     (NUMBER_OF_DIGITS(BITSIZE,52))
   #define MSD_MSK   (MS_DIGIT_MASK(RSA_2K,52))

   __m512i* pr = (__m512i*)pRes;
   __m512i* pa = (__m512i*)inpA;
   __m512i* pb = (__m512i*)inpB;

   __m512i DIG_MASK = _mm512_set1_epi64(DIGIT_MASK);
   __m512i MS_DIG_MASK = _mm512_set1_epi64(MSD_MSK);

   __m512i R00 = _mm512_load_si512(pr + 0); /* load pr[nsM-1],...,R[0] */
   __m512i R01 = _mm512_load_si512(pr + 1);
   __m512i R02 = _mm512_load_si512(pr + 2);
   __m512i R03 = _mm512_load_si512(pr + 3);
   __m512i R04 = _mm512_load_si512(pr + 4);
   __m512i R05 = _mm512_load_si512(pr + 5);
   __m512i R06 = _mm512_load_si512(pr + 6);
   __m512i R07 = _mm512_load_si512(pr + 7);
   __m512i R08 = _mm512_load_si512(pr + 8);
   __m512i R09 = _mm512_load_si512(pr + 9);
   __m512i R10 = _mm512_load_si512(pr + 10);
   __m512i R11 = _mm512_load_si512(pr + 11);
   __m512i R12 = _mm512_load_si512(pr + 12);
   __m512i R13 = _mm512_load_si512(pr + 13);
   __m512i R14 = _mm512_load_si512(pr + 14);
   __m512i R15 = _mm512_load_si512(pr + 15);
   __m512i R16 = _mm512_load_si512(pr + 16);
   __m512i R17 = _mm512_load_si512(pr + 17);
   __m512i R18 = _mm512_load_si512(pr + 18);
   __m512i R19 = _mm512_load_si512(pr + 19);

   int itr;
   for (itr = 0; itr < LEN52; itr++) {
      __m512i Bi = _mm512_load_si512(pb);
      __m512i nxtR = _mm512_load_si512(pr + LEN52);
      pb++;

      _mm512_madd52lo_epu64_(R00, R00, Bi, pa, 64 * 0);
      _mm512_madd52lo_epu64_(R01, R01, Bi, pa, 64 * 1);
      _mm512_madd52lo_epu64_(R02, R02, Bi, pa, 64 * 2);
      _mm512_madd52lo_epu64_(R03, R03, Bi, pa, 64 * 3);
      _mm512_madd52lo_epu64_(R04, R04, Bi, pa, 64 * 4);
      _mm512_madd52lo_epu64_(R05, R05, Bi, pa, 64 * 5);
      _mm512_madd52lo_epu64_(R06, R06, Bi, pa, 64 * 6);
      _mm512_madd52lo_epu64_(R07, R07, Bi, pa, 64 * 7);
      _mm512_madd52lo_epu64_(R08, R08, Bi, pa, 64 * 8);
      _mm512_madd52lo_epu64_(R09, R09, Bi, pa, 64 * 9);
      _mm512_madd52lo_epu64_(R10, R10, Bi, pa, 64 * 10);
      _mm512_madd52lo_epu64_(R11, R11, Bi, pa, 64 * 11);
      _mm512_madd52lo_epu64_(R12, R12, Bi, pa, 64 * 12);
      _mm512_madd52lo_epu64_(R13, R13, Bi, pa, 64 * 13);
      _mm512_madd52lo_epu64_(R14, R14, Bi, pa, 64 * 14);
      _mm512_madd52lo_epu64_(R15, R15, Bi, pa, 64 * 15);
      _mm512_madd52lo_epu64_(R16, R16, Bi, pa, 64 * 16);
      _mm512_madd52lo_epu64_(R17, R17, Bi, pa, 64 * 17);
      _mm512_madd52lo_epu64_(R18, R18, Bi, pa, 64 * 18);
      _mm512_madd52lo_epu64_(R19, R19, Bi, pa, 64 * 19);

      _mm512_store_si512(pr, _mm512_and_epi64(R00, DIG_MASK)); /* store normalized result */
      pr++;

      R00 = _mm512_srli_epi64(R00, DIGIT_SIZE);
      R01 = _mm512_add_epi64(R01, R00);

      _mm512_madd52hi_epu64_(R00, R01, Bi, pa, 64 * 0);
      _mm512_madd52hi_epu64_(R01, R02, Bi, pa, 64 * 1);
      _mm512_madd52hi_epu64_(R02, R03, Bi, pa, 64 * 2);
      _mm512_madd52hi_epu64_(R03, R04, Bi, pa, 64 * 3);
      _mm512_madd52hi_epu64_(R04, R05, Bi, pa, 64 * 4);
      _mm512_madd52hi_epu64_(R05, R06, Bi, pa, 64 * 5);
      _mm512_madd52hi_epu64_(R06, R07, Bi, pa, 64 * 6);
      _mm512_madd52hi_epu64_(R07, R08, Bi, pa, 64 * 7);
      _mm512_madd52hi_epu64_(R08, R09, Bi, pa, 64 * 8);
      _mm512_madd52hi_epu64_(R09, R10, Bi, pa, 64 * 9);
      _mm512_madd52hi_epu64_(R10, R11, Bi, pa, 64 * 10);
      _mm512_madd52hi_epu64_(R11, R12, Bi, pa, 64 * 11);
      _mm512_madd52hi_epu64_(R12, R13, Bi, pa, 64 * 12);
      _mm512_madd52hi_epu64_(R13, R14, Bi, pa, 64 * 13);
      _mm512_madd52hi_epu64_(R14, R15, Bi, pa, 64 * 14);
      _mm512_madd52hi_epu64_(R15, R16, Bi, pa, 64 * 15);
      _mm512_madd52hi_epu64_(R16, R17, Bi, pa, 64 * 16);
      _mm512_madd52hi_epu64_(R17, R18, Bi, pa, 64 * 17);
      _mm512_madd52hi_epu64_(R18, R19, Bi, pa, 64 * 18);
      _mm512_madd52hi_epu64_(R19, nxtR, Bi, pa, 64 * 19);
   }
   /* normalization */
   {
      __m512i
         T = _mm512_srli_epi64(R00, DIGIT_SIZE);
      R00 = _mm512_and_epi64(R00, DIG_MASK);
      _mm512_store_si512(pr + 0, R00);

      R01 = _mm512_add_epi64(R01, T);
      T = _mm512_srli_epi64(R01, DIGIT_SIZE);
      R01 = _mm512_and_epi64(R01, DIG_MASK);
      _mm512_store_si512(pr + 1, R01);

      R02 = _mm512_add_epi64(R02, T);
      T = _mm512_srli_epi64(R02, DIGIT_SIZE);
      R02 = _mm512_and_epi64(R02, DIG_MASK);
      _mm512_store_si512(pr + 2, R02);

      R03 = _mm512_add_epi64(R03, T);
      T = _mm512_srli_epi64(R03, DIGIT_SIZE);
      R03 = _mm512_and_epi64(R03, DIG_MASK);
      _mm512_store_si512(pr + 3, R03);

      R04 = _mm512_add_epi64(R04, T);
      T = _mm512_srli_epi64(R04, DIGIT_SIZE);
      R04 = _mm512_and_epi64(R04, DIG_MASK);
      _mm512_store_si512(pr + 4, R04);

      R05 = _mm512_add_epi64(R05, T);
      T = _mm512_srli_epi64(R05, DIGIT_SIZE);
      R05 = _mm512_and_epi64(R05, DIG_MASK);
      _mm512_store_si512(pr + 5, R05);

      R06 = _mm512_add_epi64(R06, T);
      T = _mm512_srli_epi64(R06, DIGIT_SIZE);
      R06 = _mm512_and_epi64(R06, DIG_MASK);
      _mm512_store_si512(pr + 6, R06);

      R07 = _mm512_add_epi64(R07, T);
      T = _mm512_srli_epi64(R07, DIGIT_SIZE);
      R07 = _mm512_and_epi64(R07, DIG_MASK);
      _mm512_store_si512(pr + 7, R07);

      R08 = _mm512_add_epi64(R08, T);
      T = _mm512_srli_epi64(R08, DIGIT_SIZE);
      R08 = _mm512_and_epi64(R08, DIG_MASK);
      _mm512_store_si512(pr + 8, R08);

      R09 = _mm512_add_epi64(R09, T);
      T = _mm512_srli_epi64(R09, DIGIT_SIZE);
      R09 = _mm512_and_epi64(R09, DIG_MASK);
      _mm512_store_si512(pr + 9, R09);

      R10 = _mm512_add_epi64(R10, T);
      T = _mm512_srli_epi64(R10, DIGIT_SIZE);
      R10 = _mm512_and_epi64(R10, DIG_MASK);
      _mm512_store_si512(pr + 10, R10);

      R11 = _mm512_add_epi64(R11, T);
      T = _mm512_srli_epi64(R11, DIGIT_SIZE);
      R11 = _mm512_and_epi64(R11, DIG_MASK);
      _mm512_store_si512(pr + 11, R11);

      R12 = _mm512_add_epi64(R12, T);
      T = _mm512_srli_epi64(R12, DIGIT_SIZE);
      R12 = _mm512_and_epi64(R12, DIG_MASK);
      _mm512_store_si512(pr + 12, R12);

      R13 = _mm512_add_epi64(R13, T);
      T = _mm512_srli_epi64(R13, DIGIT_SIZE);
      R13 = _mm512_and_epi64(R13, DIG_MASK);
      _mm512_store_si512(pr + 13, R13);

      R14 = _mm512_add_epi64(R14, T);
      T = _mm512_srli_epi64(R14, DIGIT_SIZE);
      R14 = _mm512_and_epi64(R14, DIG_MASK);
      _mm512_store_si512(pr + 14, R14);

      R15 = _mm512_add_epi64(R15, T);
      T = _mm512_srli_epi64(R15, DIGIT_SIZE);
      R15 = _mm512_and_epi64(R15, DIG_MASK);
      _mm512_store_si512(pr + 15, R15);

      R16 = _mm512_add_epi64(R16, T);
      T = _mm512_srli_epi64(R16, DIGIT_SIZE);
      R16 = _mm512_and_epi64(R16, DIG_MASK);
      _mm512_store_si512(pr + 16, R16);

      R17 = _mm512_add_epi64(R17, T);
      T = _mm512_srli_epi64(R17, DIGIT_SIZE);
      R17 = _mm512_and_epi64(R17, DIG_MASK);
      _mm512_store_si512(pr + 17, R17);

      R18 = _mm512_add_epi64(R18, T);
      T = _mm512_srli_epi64(R18, DIGIT_SIZE);
      R18 = _mm512_and_epi64(R18, DIG_MASK);
      _mm512_store_si512(pr + 18, R18);

      R19 = _mm512_add_epi64(R19, T);
      T = _mm512_srli_epi64(R19, DIGIT_SIZE);
      R19 = _mm512_and_epi64(R19, MS_DIG_MASK);
      _mm512_store_si512(pr + 19, R19);
   }

   #undef BITSIZE
   #undef LEN52
   #undef MSD_MSK
}

void ifma_addmul52x30_mb8(int64u pRes[][8], const int64u inpA[][8], const int64u inpB[][8])
{
#define BITSIZE   (RSA_3K/2)
#define LEN52     (NUMBER_OF_DIGITS(BITSIZE,52))
#define MSD_MSK   (MS_DIGIT_MASK(RSA_3K,52))

   __m512i* pr = (__m512i*)pRes;
   __m512i* pa = (__m512i*)inpA;
   __m512i* pb = (__m512i*)inpB;

   __m512i DIG_MASK = _mm512_set1_epi64(DIGIT_MASK);
   __m512i MS_DIG_MASK = _mm512_set1_epi64(MSD_MSK);

   __m512i R00 = _mm512_load_si512(pr + 0); /* load pr[nsM-1],...,R[0] */
   __m512i R01 = _mm512_load_si512(pr + 1);
   __m512i R02 = _mm512_load_si512(pr + 2);
   __m512i R03 = _mm512_load_si512(pr + 3);
   __m512i R04 = _mm512_load_si512(pr + 4);
   __m512i R05 = _mm512_load_si512(pr + 5);
   __m512i R06 = _mm512_load_si512(pr + 6);
   __m512i R07 = _mm512_load_si512(pr + 7);
   __m512i R08 = _mm512_load_si512(pr + 8);
   __m512i R09 = _mm512_load_si512(pr + 9);
   __m512i R10 = _mm512_load_si512(pr + 10);
   __m512i R11 = _mm512_load_si512(pr + 11);
   __m512i R12 = _mm512_load_si512(pr + 12);
   __m512i R13 = _mm512_load_si512(pr + 13);
   __m512i R14 = _mm512_load_si512(pr + 14);
   __m512i R15 = _mm512_load_si512(pr + 15);
   __m512i R16 = _mm512_load_si512(pr + 16);
   __m512i R17 = _mm512_load_si512(pr + 17);
   __m512i R18 = _mm512_load_si512(pr + 18);
   __m512i R19 = _mm512_load_si512(pr + 19);
   __m512i R20 = _mm512_load_si512(pr + 20);
   __m512i R21 = _mm512_load_si512(pr + 21);
   __m512i R22 = _mm512_load_si512(pr + 22);
   __m512i R23 = _mm512_load_si512(pr + 23);
   __m512i R24 = _mm512_load_si512(pr + 24);
   __m512i R25 = _mm512_load_si512(pr + 25);
   __m512i R26 = _mm512_load_si512(pr + 26);
   __m512i R27 = _mm512_load_si512(pr + 27);
   __m512i R28 = _mm512_load_si512(pr + 28);
   __m512i R29 = _mm512_load_si512(pr + 29);

   int itr;
   for (itr = 0; itr < LEN52; itr++) {
      __m512i Bi = _mm512_load_si512(pb);
      __m512i nxtR = _mm512_load_si512(pr + LEN52);
      pb++;

      _mm512_madd52lo_epu64_(R00, R00, Bi, pa, 64 * 0);
      _mm512_madd52lo_epu64_(R01, R01, Bi, pa, 64 * 1);
      _mm512_madd52lo_epu64_(R02, R02, Bi, pa, 64 * 2);
      _mm512_madd52lo_epu64_(R03, R03, Bi, pa, 64 * 3);
      _mm512_madd52lo_epu64_(R04, R04, Bi, pa, 64 * 4);
      _mm512_madd52lo_epu64_(R05, R05, Bi, pa, 64 * 5);
      _mm512_madd52lo_epu64_(R06, R06, Bi, pa, 64 * 6);
      _mm512_madd52lo_epu64_(R07, R07, Bi, pa, 64 * 7);
      _mm512_madd52lo_epu64_(R08, R08, Bi, pa, 64 * 8);
      _mm512_madd52lo_epu64_(R09, R09, Bi, pa, 64 * 9);
      _mm512_madd52lo_epu64_(R10, R10, Bi, pa, 64 * 10);
      _mm512_madd52lo_epu64_(R11, R11, Bi, pa, 64 * 11);
      _mm512_madd52lo_epu64_(R12, R12, Bi, pa, 64 * 12);
      _mm512_madd52lo_epu64_(R13, R13, Bi, pa, 64 * 13);
      _mm512_madd52lo_epu64_(R14, R14, Bi, pa, 64 * 14);
      _mm512_madd52lo_epu64_(R15, R15, Bi, pa, 64 * 15);
      _mm512_madd52lo_epu64_(R16, R16, Bi, pa, 64 * 16);
      _mm512_madd52lo_epu64_(R17, R17, Bi, pa, 64 * 17);
      _mm512_madd52lo_epu64_(R18, R18, Bi, pa, 64 * 18);
      _mm512_madd52lo_epu64_(R19, R19, Bi, pa, 64 * 19);
      _mm512_madd52lo_epu64_(R20, R20, Bi, pa, 64 * 20);
      _mm512_madd52lo_epu64_(R21, R21, Bi, pa, 64 * 21);
      _mm512_madd52lo_epu64_(R22, R22, Bi, pa, 64 * 22);
      _mm512_madd52lo_epu64_(R23, R23, Bi, pa, 64 * 23);
      _mm512_madd52lo_epu64_(R24, R24, Bi, pa, 64 * 24);
      _mm512_madd52lo_epu64_(R25, R25, Bi, pa, 64 * 25);
      _mm512_madd52lo_epu64_(R26, R26, Bi, pa, 64 * 26);
      _mm512_madd52lo_epu64_(R27, R27, Bi, pa, 64 * 27);
      _mm512_madd52lo_epu64_(R28, R28, Bi, pa, 64 * 28);
      _mm512_madd52lo_epu64_(R29, R29, Bi, pa, 64 * 29);

      _mm512_store_si512(pr, _mm512_and_epi64(R00, DIG_MASK)); /* store normalized result */
      pr++;

      R00 = _mm512_srli_epi64(R00, DIGIT_SIZE);
      R01 = _mm512_add_epi64(R01, R00);

      _mm512_madd52hi_epu64_(R00, R01, Bi, pa, 64 * 0);
      _mm512_madd52hi_epu64_(R01, R02, Bi, pa, 64 * 1);
      _mm512_madd52hi_epu64_(R02, R03, Bi, pa, 64 * 2);
      _mm512_madd52hi_epu64_(R03, R04, Bi, pa, 64 * 3);
      _mm512_madd52hi_epu64_(R04, R05, Bi, pa, 64 * 4);
      _mm512_madd52hi_epu64_(R05, R06, Bi, pa, 64 * 5);
      _mm512_madd52hi_epu64_(R06, R07, Bi, pa, 64 * 6);
      _mm512_madd52hi_epu64_(R07, R08, Bi, pa, 64 * 7);
      _mm512_madd52hi_epu64_(R08, R09, Bi, pa, 64 * 8);
      _mm512_madd52hi_epu64_(R09, R10, Bi, pa, 64 * 9);
      _mm512_madd52hi_epu64_(R10, R11, Bi, pa, 64 * 10);
      _mm512_madd52hi_epu64_(R11, R12, Bi, pa, 64 * 11);
      _mm512_madd52hi_epu64_(R12, R13, Bi, pa, 64 * 12);
      _mm512_madd52hi_epu64_(R13, R14, Bi, pa, 64 * 13);
      _mm512_madd52hi_epu64_(R14, R15, Bi, pa, 64 * 14);
      _mm512_madd52hi_epu64_(R15, R16, Bi, pa, 64 * 15);
      _mm512_madd52hi_epu64_(R16, R17, Bi, pa, 64 * 16);
      _mm512_madd52hi_epu64_(R17, R18, Bi, pa, 64 * 17);
      _mm512_madd52hi_epu64_(R18, R19, Bi, pa, 64 * 18);
      _mm512_madd52hi_epu64_(R19, R20, Bi, pa, 64 * 19);
      _mm512_madd52hi_epu64_(R20, R21, Bi, pa, 64 * 20);
      _mm512_madd52hi_epu64_(R21, R22, Bi, pa, 64 * 21);
      _mm512_madd52hi_epu64_(R22, R23, Bi, pa, 64 * 22);
      _mm512_madd52hi_epu64_(R23, R24, Bi, pa, 64 * 23);
      _mm512_madd52hi_epu64_(R24, R25, Bi, pa, 64 * 24);
      _mm512_madd52hi_epu64_(R25, R26, Bi, pa, 64 * 25);
      _mm512_madd52hi_epu64_(R26, R27, Bi, pa, 64 * 26);
      _mm512_madd52hi_epu64_(R27, R28, Bi, pa, 64 * 27);
      _mm512_madd52hi_epu64_(R28, R29, Bi, pa, 64 * 28);
      _mm512_madd52hi_epu64_(R29, nxtR, Bi, pa, 64 * 29);
   }
   /* normalization */
   {
      __m512i
         T = _mm512_srli_epi64(R00, DIGIT_SIZE);
      R00 = _mm512_and_epi64(R00, DIG_MASK);
      _mm512_store_si512(pr + 0, R00);

      R01 = _mm512_add_epi64(R01, T);
      T = _mm512_srli_epi64(R01, DIGIT_SIZE);
      R01 = _mm512_and_epi64(R01, DIG_MASK);
      _mm512_store_si512(pr + 1, R01);

      R02 = _mm512_add_epi64(R02, T);
      T = _mm512_srli_epi64(R02, DIGIT_SIZE);
      R02 = _mm512_and_epi64(R02, DIG_MASK);
      _mm512_store_si512(pr + 2, R02);

      R03 = _mm512_add_epi64(R03, T);
      T = _mm512_srli_epi64(R03, DIGIT_SIZE);
      R03 = _mm512_and_epi64(R03, DIG_MASK);
      _mm512_store_si512(pr + 3, R03);

      R04 = _mm512_add_epi64(R04, T);
      T = _mm512_srli_epi64(R04, DIGIT_SIZE);
      R04 = _mm512_and_epi64(R04, DIG_MASK);
      _mm512_store_si512(pr + 4, R04);

      R05 = _mm512_add_epi64(R05, T);
      T = _mm512_srli_epi64(R05, DIGIT_SIZE);
      R05 = _mm512_and_epi64(R05, DIG_MASK);
      _mm512_store_si512(pr + 5, R05);

      R06 = _mm512_add_epi64(R06, T);
      T = _mm512_srli_epi64(R06, DIGIT_SIZE);
      R06 = _mm512_and_epi64(R06, DIG_MASK);
      _mm512_store_si512(pr + 6, R06);

      R07 = _mm512_add_epi64(R07, T);
      T = _mm512_srli_epi64(R07, DIGIT_SIZE);
      R07 = _mm512_and_epi64(R07, DIG_MASK);
      _mm512_store_si512(pr + 7, R07);

      R08 = _mm512_add_epi64(R08, T);
      T = _mm512_srli_epi64(R08, DIGIT_SIZE);
      R08 = _mm512_and_epi64(R08, DIG_MASK);
      _mm512_store_si512(pr + 8, R08);

      R09 = _mm512_add_epi64(R09, T);
      T = _mm512_srli_epi64(R09, DIGIT_SIZE);
      R09 = _mm512_and_epi64(R09, DIG_MASK);
      _mm512_store_si512(pr + 9, R09);

      R10 = _mm512_add_epi64(R10, T);
      T = _mm512_srli_epi64(R10, DIGIT_SIZE);
      R10 = _mm512_and_epi64(R10, DIG_MASK);
      _mm512_store_si512(pr + 10, R10);

      R11 = _mm512_add_epi64(R11, T);
      T = _mm512_srli_epi64(R11, DIGIT_SIZE);
      R11 = _mm512_and_epi64(R11, DIG_MASK);
      _mm512_store_si512(pr + 11, R11);

      R12 = _mm512_add_epi64(R12, T);
      T = _mm512_srli_epi64(R12, DIGIT_SIZE);
      R12 = _mm512_and_epi64(R12, DIG_MASK);
      _mm512_store_si512(pr + 12, R12);

      R13 = _mm512_add_epi64(R13, T);
      T = _mm512_srli_epi64(R13, DIGIT_SIZE);
      R13 = _mm512_and_epi64(R13, DIG_MASK);
      _mm512_store_si512(pr + 13, R13);

      R14 = _mm512_add_epi64(R14, T);
      T = _mm512_srli_epi64(R14, DIGIT_SIZE);
      R14 = _mm512_and_epi64(R14, DIG_MASK);
      _mm512_store_si512(pr + 14, R14);

      R15 = _mm512_add_epi64(R15, T);
      T = _mm512_srli_epi64(R15, DIGIT_SIZE);
      R15 = _mm512_and_epi64(R15, DIG_MASK);
      _mm512_store_si512(pr + 15, R15);

      R16 = _mm512_add_epi64(R16, T);
      T = _mm512_srli_epi64(R16, DIGIT_SIZE);
      R16 = _mm512_and_epi64(R16, DIG_MASK);
      _mm512_store_si512(pr + 16, R16);

      R17 = _mm512_add_epi64(R17, T);
      T = _mm512_srli_epi64(R17, DIGIT_SIZE);
      R17 = _mm512_and_epi64(R17, DIG_MASK);
      _mm512_store_si512(pr + 17, R17);

      R18 = _mm512_add_epi64(R18, T);
      T = _mm512_srli_epi64(R18, DIGIT_SIZE);
      R18 = _mm512_and_epi64(R18, DIG_MASK);
      _mm512_store_si512(pr + 18, R18);

      R19 = _mm512_add_epi64(R19, T);
      T = _mm512_srli_epi64(R19, DIGIT_SIZE);
      R19 = _mm512_and_epi64(R19, DIG_MASK);
      _mm512_store_si512(pr + 19, R19);

      R20 = _mm512_add_epi64(R20, T);
      T = _mm512_srli_epi64(R20, DIGIT_SIZE);
      R20 = _mm512_and_epi64(R20, DIG_MASK);
      _mm512_store_si512(pr + 20, R20);

      R21 = _mm512_add_epi64(R21, T);
      T = _mm512_srli_epi64(R21, DIGIT_SIZE);
      R21 = _mm512_and_epi64(R21, DIG_MASK);
      _mm512_store_si512(pr + 21, R21);

      R22 = _mm512_add_epi64(R22, T);
      T = _mm512_srli_epi64(R22, DIGIT_SIZE);
      R22 = _mm512_and_epi64(R22, DIG_MASK);
      _mm512_store_si512(pr + 22, R22);

      R23 = _mm512_add_epi64(R23, T);
      T = _mm512_srli_epi64(R23, DIGIT_SIZE);
      R23 = _mm512_and_epi64(R23, DIG_MASK);
      _mm512_store_si512(pr + 23, R23);

      R24 = _mm512_add_epi64(R24, T);
      T = _mm512_srli_epi64(R24, DIGIT_SIZE);
      R24 = _mm512_and_epi64(R24, DIG_MASK);
      _mm512_store_si512(pr + 24, R24);

      R25 = _mm512_add_epi64(R25, T);
      T = _mm512_srli_epi64(R25, DIGIT_SIZE);
      R25 = _mm512_and_epi64(R25, DIG_MASK);
      _mm512_store_si512(pr + 25, R25);

      R26 = _mm512_add_epi64(R26, T);
      T = _mm512_srli_epi64(R26, DIGIT_SIZE);
      R26 = _mm512_and_epi64(R26, DIG_MASK);
      _mm512_store_si512(pr + 26, R26);

      R27 = _mm512_add_epi64(R27, T);
      T = _mm512_srli_epi64(R27, DIGIT_SIZE);
      R27 = _mm512_and_epi64(R27, DIG_MASK);
      _mm512_store_si512(pr + 27, R27);

      R28 = _mm512_add_epi64(R28, T);
      T = _mm512_srli_epi64(R28, DIGIT_SIZE);
      R28 = _mm512_and_epi64(R28, DIG_MASK);
      _mm512_store_si512(pr + 28, R28);

      R29 = _mm512_add_epi64(R29, T);
      T = _mm512_srli_epi64(R29, DIGIT_SIZE);
      R29 = _mm512_and_epi64(R29, MS_DIG_MASK);
      _mm512_store_si512(pr + 29, R29);
   }

   #undef BITSIZE
   #undef LEN52
   #undef MSD_MSK
}


void ifma_addmul52x40_mb8(int64u pRes[][8], const int64u inpA[][8], const int64u inpB[][8])
{
#define BITSIZE   (RSA_4K/2)
#define LEN52     (NUMBER_OF_DIGITS(BITSIZE,52))
#define MSD_MSK   (MS_DIGIT_MASK(RSA_4K,52))

   __m512i* pr = (__m512i*)pRes;
   __m512i* pa = (__m512i*)inpA;
   __m512i* pb = (__m512i*)inpB;

   __m512i DIG_MASK = _mm512_set1_epi64(DIGIT_MASK);
   __m512i MS_DIG_MASK = _mm512_set1_epi64(MSD_MSK);

   __m512i R00 = _mm512_load_si512(pr + 0); /* load pr[nsM-1],...,R[0] */
   __m512i R01 = _mm512_load_si512(pr + 1);
   __m512i R02 = _mm512_load_si512(pr + 2);
   __m512i R03 = _mm512_load_si512(pr + 3);
   __m512i R04 = _mm512_load_si512(pr + 4);
   __m512i R05 = _mm512_load_si512(pr + 5);
   __m512i R06 = _mm512_load_si512(pr + 6);
   __m512i R07 = _mm512_load_si512(pr + 7);
   __m512i R08 = _mm512_load_si512(pr + 8);
   __m512i R09 = _mm512_load_si512(pr + 9);
   __m512i R10 = _mm512_load_si512(pr + 10);
   __m512i R11 = _mm512_load_si512(pr + 11);
   __m512i R12 = _mm512_load_si512(pr + 12);
   __m512i R13 = _mm512_load_si512(pr + 13);
   __m512i R14 = _mm512_load_si512(pr + 14);
   __m512i R15 = _mm512_load_si512(pr + 15);
   __m512i R16 = _mm512_load_si512(pr + 16);
   __m512i R17 = _mm512_load_si512(pr + 17);
   __m512i R18 = _mm512_load_si512(pr + 18);
   __m512i R19 = _mm512_load_si512(pr + 19);
   __m512i R20 = _mm512_load_si512(pr + 20);
   __m512i R21 = _mm512_load_si512(pr + 21);
   __m512i R22 = _mm512_load_si512(pr + 22);
   __m512i R23 = _mm512_load_si512(pr + 23);
   __m512i R24 = _mm512_load_si512(pr + 24);
   __m512i R25 = _mm512_load_si512(pr + 25);
   __m512i R26 = _mm512_load_si512(pr + 26);
   __m512i R27 = _mm512_load_si512(pr + 27);
   __m512i R28 = _mm512_load_si512(pr + 28);
   __m512i R29 = _mm512_load_si512(pr + 29);
   __m512i R30 = _mm512_load_si512(pr + 30);
   __m512i R31 = _mm512_load_si512(pr + 31);
   __m512i R32 = _mm512_load_si512(pr + 32);
   __m512i R33 = _mm512_load_si512(pr + 33);
   __m512i R34 = _mm512_load_si512(pr + 34);
   __m512i R35 = _mm512_load_si512(pr + 35);
   __m512i R36 = _mm512_load_si512(pr + 36);
   __m512i R37 = _mm512_load_si512(pr + 37);
   __m512i R38 = _mm512_load_si512(pr + 38);
   __m512i R39 = _mm512_load_si512(pr + 39);

   int itr;
   for (itr = 0; itr < LEN52; itr++) {
      __m512i Bi = _mm512_load_si512(pb);
      __m512i nxtR = _mm512_load_si512(pr + LEN52);
      pb++;

      _mm512_madd52lo_epu64_(R00, R00, Bi, pa, 64 * 0);
      _mm512_madd52lo_epu64_(R01, R01, Bi, pa, 64 * 1);
      _mm512_madd52lo_epu64_(R02, R02, Bi, pa, 64 * 2);
      _mm512_madd52lo_epu64_(R03, R03, Bi, pa, 64 * 3);
      _mm512_madd52lo_epu64_(R04, R04, Bi, pa, 64 * 4);
      _mm512_madd52lo_epu64_(R05, R05, Bi, pa, 64 * 5);
      _mm512_madd52lo_epu64_(R06, R06, Bi, pa, 64 * 6);
      _mm512_madd52lo_epu64_(R07, R07, Bi, pa, 64 * 7);
      _mm512_madd52lo_epu64_(R08, R08, Bi, pa, 64 * 8);
      _mm512_madd52lo_epu64_(R09, R09, Bi, pa, 64 * 9);
      _mm512_madd52lo_epu64_(R10, R10, Bi, pa, 64 * 10);
      _mm512_madd52lo_epu64_(R11, R11, Bi, pa, 64 * 11);
      _mm512_madd52lo_epu64_(R12, R12, Bi, pa, 64 * 12);
      _mm512_madd52lo_epu64_(R13, R13, Bi, pa, 64 * 13);
      _mm512_madd52lo_epu64_(R14, R14, Bi, pa, 64 * 14);
      _mm512_madd52lo_epu64_(R15, R15, Bi, pa, 64 * 15);
      _mm512_madd52lo_epu64_(R16, R16, Bi, pa, 64 * 16);
      _mm512_madd52lo_epu64_(R17, R17, Bi, pa, 64 * 17);
      _mm512_madd52lo_epu64_(R18, R18, Bi, pa, 64 * 18);
      _mm512_madd52lo_epu64_(R19, R19, Bi, pa, 64 * 19);
      _mm512_madd52lo_epu64_(R20, R20, Bi, pa, 64 * 20);
      _mm512_madd52lo_epu64_(R21, R21, Bi, pa, 64 * 21);
      _mm512_madd52lo_epu64_(R22, R22, Bi, pa, 64 * 22);
      _mm512_madd52lo_epu64_(R23, R23, Bi, pa, 64 * 23);
      _mm512_madd52lo_epu64_(R24, R24, Bi, pa, 64 * 24);
      _mm512_madd52lo_epu64_(R25, R25, Bi, pa, 64 * 25);
      _mm512_madd52lo_epu64_(R26, R26, Bi, pa, 64 * 26);
      _mm512_madd52lo_epu64_(R27, R27, Bi, pa, 64 * 27);
      _mm512_madd52lo_epu64_(R28, R28, Bi, pa, 64 * 28);
      _mm512_madd52lo_epu64_(R29, R29, Bi, pa, 64 * 29);
      _mm512_madd52lo_epu64_(R30, R30, Bi, pa, 64 * 30);
      _mm512_madd52lo_epu64_(R31, R31, Bi, pa, 64 * 31);
      _mm512_madd52lo_epu64_(R32, R32, Bi, pa, 64 * 32);
      _mm512_madd52lo_epu64_(R33, R33, Bi, pa, 64 * 33);
      _mm512_madd52lo_epu64_(R34, R34, Bi, pa, 64 * 34);
      _mm512_madd52lo_epu64_(R35, R35, Bi, pa, 64 * 35);
      _mm512_madd52lo_epu64_(R36, R36, Bi, pa, 64 * 36);
      _mm512_madd52lo_epu64_(R37, R37, Bi, pa, 64 * 37);
      _mm512_madd52lo_epu64_(R38, R38, Bi, pa, 64 * 38);
      _mm512_madd52lo_epu64_(R39, R39, Bi, pa, 64 * 39);

      _mm512_store_si512(pr, _mm512_and_epi64(R00, DIG_MASK)); /* store normalized result */
      pr++;

      R00 = _mm512_srli_epi64(R00, DIGIT_SIZE);
      R01 = _mm512_add_epi64(R01, R00);

      _mm512_madd52hi_epu64_(R00, R01, Bi, pa, 64 * 0);
      _mm512_madd52hi_epu64_(R01, R02, Bi, pa, 64 * 1);
      _mm512_madd52hi_epu64_(R02, R03, Bi, pa, 64 * 2);
      _mm512_madd52hi_epu64_(R03, R04, Bi, pa, 64 * 3);
      _mm512_madd52hi_epu64_(R04, R05, Bi, pa, 64 * 4);
      _mm512_madd52hi_epu64_(R05, R06, Bi, pa, 64 * 5);
      _mm512_madd52hi_epu64_(R06, R07, Bi, pa, 64 * 6);
      _mm512_madd52hi_epu64_(R07, R08, Bi, pa, 64 * 7);
      _mm512_madd52hi_epu64_(R08, R09, Bi, pa, 64 * 8);
      _mm512_madd52hi_epu64_(R09, R10, Bi, pa, 64 * 9);
      _mm512_madd52hi_epu64_(R10, R11, Bi, pa, 64 * 10);
      _mm512_madd52hi_epu64_(R11, R12, Bi, pa, 64 * 11);
      _mm512_madd52hi_epu64_(R12, R13, Bi, pa, 64 * 12);
      _mm512_madd52hi_epu64_(R13, R14, Bi, pa, 64 * 13);
      _mm512_madd52hi_epu64_(R14, R15, Bi, pa, 64 * 14);
      _mm512_madd52hi_epu64_(R15, R16, Bi, pa, 64 * 15);
      _mm512_madd52hi_epu64_(R16, R17, Bi, pa, 64 * 16);
      _mm512_madd52hi_epu64_(R17, R18, Bi, pa, 64 * 17);
      _mm512_madd52hi_epu64_(R18, R19, Bi, pa, 64 * 18);
      _mm512_madd52hi_epu64_(R19, R20, Bi, pa, 64 * 19);
      _mm512_madd52hi_epu64_(R20, R21, Bi, pa, 64 * 20);
      _mm512_madd52hi_epu64_(R21, R22, Bi, pa, 64 * 21);
      _mm512_madd52hi_epu64_(R22, R23, Bi, pa, 64 * 22);
      _mm512_madd52hi_epu64_(R23, R24, Bi, pa, 64 * 23);
      _mm512_madd52hi_epu64_(R24, R25, Bi, pa, 64 * 24);
      _mm512_madd52hi_epu64_(R25, R26, Bi, pa, 64 * 25);
      _mm512_madd52hi_epu64_(R26, R27, Bi, pa, 64 * 26);
      _mm512_madd52hi_epu64_(R27, R28, Bi, pa, 64 * 27);
      _mm512_madd52hi_epu64_(R28, R29, Bi, pa, 64 * 28);
      _mm512_madd52hi_epu64_(R29, R30, Bi, pa, 64 * 29);
      _mm512_madd52hi_epu64_(R30, R31, Bi, pa, 64 * 30);
      _mm512_madd52hi_epu64_(R31, R32, Bi, pa, 64 * 31);
      _mm512_madd52hi_epu64_(R32, R33, Bi, pa, 64 * 32);
      _mm512_madd52hi_epu64_(R33, R34, Bi, pa, 64 * 33);
      _mm512_madd52hi_epu64_(R34, R35, Bi, pa, 64 * 34);
      _mm512_madd52hi_epu64_(R35, R36, Bi, pa, 64 * 35);
      _mm512_madd52hi_epu64_(R36, R37, Bi, pa, 64 * 36);
      _mm512_madd52hi_epu64_(R37, R38, Bi, pa, 64 * 37);
      _mm512_madd52hi_epu64_(R38, R39, Bi, pa, 64 * 38);
      _mm512_madd52hi_epu64_(R39, nxtR, Bi, pa, 64 * 39);

   }
   /* normalization */
   {
      __m512i
         T = _mm512_srli_epi64(R00, DIGIT_SIZE);
      R00 = _mm512_and_epi64(R00, DIG_MASK);
      _mm512_store_si512(pr + 0, R00);

      R01 = _mm512_add_epi64(R01, T);
      T = _mm512_srli_epi64(R01, DIGIT_SIZE);
      R01 = _mm512_and_epi64(R01, DIG_MASK);
      _mm512_store_si512(pr + 1, R01);

      R02 = _mm512_add_epi64(R02, T);
      T = _mm512_srli_epi64(R02, DIGIT_SIZE);
      R02 = _mm512_and_epi64(R02, DIG_MASK);
      _mm512_store_si512(pr + 2, R02);

      R03 = _mm512_add_epi64(R03, T);
      T = _mm512_srli_epi64(R03, DIGIT_SIZE);
      R03 = _mm512_and_epi64(R03, DIG_MASK);
      _mm512_store_si512(pr + 3, R03);

      R04 = _mm512_add_epi64(R04, T);
      T = _mm512_srli_epi64(R04, DIGIT_SIZE);
      R04 = _mm512_and_epi64(R04, DIG_MASK);
      _mm512_store_si512(pr + 4, R04);

      R05 = _mm512_add_epi64(R05, T);
      T = _mm512_srli_epi64(R05, DIGIT_SIZE);
      R05 = _mm512_and_epi64(R05, DIG_MASK);
      _mm512_store_si512(pr + 5, R05);

      R06 = _mm512_add_epi64(R06, T);
      T = _mm512_srli_epi64(R06, DIGIT_SIZE);
      R06 = _mm512_and_epi64(R06, DIG_MASK);
      _mm512_store_si512(pr + 6, R06);

      R07 = _mm512_add_epi64(R07, T);
      T = _mm512_srli_epi64(R07, DIGIT_SIZE);
      R07 = _mm512_and_epi64(R07, DIG_MASK);
      _mm512_store_si512(pr + 7, R07);

      R08 = _mm512_add_epi64(R08, T);
      T = _mm512_srli_epi64(R08, DIGIT_SIZE);
      R08 = _mm512_and_epi64(R08, DIG_MASK);
      _mm512_store_si512(pr + 8, R08);

      R09 = _mm512_add_epi64(R09, T);
      T = _mm512_srli_epi64(R09, DIGIT_SIZE);
      R09 = _mm512_and_epi64(R09, DIG_MASK);
      _mm512_store_si512(pr + 9, R09);

      R10 = _mm512_add_epi64(R10, T);
      T = _mm512_srli_epi64(R10, DIGIT_SIZE);
      R10 = _mm512_and_epi64(R10, DIG_MASK);
      _mm512_store_si512(pr + 10, R10);

      R11 = _mm512_add_epi64(R11, T);
      T = _mm512_srli_epi64(R11, DIGIT_SIZE);
      R11 = _mm512_and_epi64(R11, DIG_MASK);
      _mm512_store_si512(pr + 11, R11);

      R12 = _mm512_add_epi64(R12, T);
      T = _mm512_srli_epi64(R12, DIGIT_SIZE);
      R12 = _mm512_and_epi64(R12, DIG_MASK);
      _mm512_store_si512(pr + 12, R12);

      R13 = _mm512_add_epi64(R13, T);
      T = _mm512_srli_epi64(R13, DIGIT_SIZE);
      R13 = _mm512_and_epi64(R13, DIG_MASK);
      _mm512_store_si512(pr + 13, R13);

      R14 = _mm512_add_epi64(R14, T);
      T = _mm512_srli_epi64(R14, DIGIT_SIZE);
      R14 = _mm512_and_epi64(R14, DIG_MASK);
      _mm512_store_si512(pr + 14, R14);

      R15 = _mm512_add_epi64(R15, T);
      T = _mm512_srli_epi64(R15, DIGIT_SIZE);
      R15 = _mm512_and_epi64(R15, DIG_MASK);
      _mm512_store_si512(pr + 15, R15);

      R16 = _mm512_add_epi64(R16, T);
      T = _mm512_srli_epi64(R16, DIGIT_SIZE);
      R16 = _mm512_and_epi64(R16, DIG_MASK);
      _mm512_store_si512(pr + 16, R16);

      R17 = _mm512_add_epi64(R17, T);
      T = _mm512_srli_epi64(R17, DIGIT_SIZE);
      R17 = _mm512_and_epi64(R17, DIG_MASK);
      _mm512_store_si512(pr + 17, R17);

      R18 = _mm512_add_epi64(R18, T);
      T = _mm512_srli_epi64(R18, DIGIT_SIZE);
      R18 = _mm512_and_epi64(R18, DIG_MASK);
      _mm512_store_si512(pr + 18, R18);

      R19 = _mm512_add_epi64(R19, T);
      T = _mm512_srli_epi64(R19, DIGIT_SIZE);
      R19 = _mm512_and_epi64(R19, DIG_MASK);
      _mm512_store_si512(pr + 19, R19);

      R20 = _mm512_add_epi64(R20, T);
      T = _mm512_srli_epi64(R20, DIGIT_SIZE);
      R20 = _mm512_and_epi64(R20, DIG_MASK);
      _mm512_store_si512(pr + 20, R20);

      R21 = _mm512_add_epi64(R21, T);
      T = _mm512_srli_epi64(R21, DIGIT_SIZE);
      R21 = _mm512_and_epi64(R21, DIG_MASK);
      _mm512_store_si512(pr + 21, R21);

      R22 = _mm512_add_epi64(R22, T);
      T = _mm512_srli_epi64(R22, DIGIT_SIZE);
      R22 = _mm512_and_epi64(R22, DIG_MASK);
      _mm512_store_si512(pr + 22, R22);

      R23 = _mm512_add_epi64(R23, T);
      T = _mm512_srli_epi64(R23, DIGIT_SIZE);
      R23 = _mm512_and_epi64(R23, DIG_MASK);
      _mm512_store_si512(pr + 23, R23);

      R24 = _mm512_add_epi64(R24, T);
      T = _mm512_srli_epi64(R24, DIGIT_SIZE);
      R24 = _mm512_and_epi64(R24, DIG_MASK);
      _mm512_store_si512(pr + 24, R24);

      R25 = _mm512_add_epi64(R25, T);
      T = _mm512_srli_epi64(R25, DIGIT_SIZE);
      R25 = _mm512_and_epi64(R25, DIG_MASK);
      _mm512_store_si512(pr + 25, R25);

      R26 = _mm512_add_epi64(R26, T);
      T = _mm512_srli_epi64(R26, DIGIT_SIZE);
      R26 = _mm512_and_epi64(R26, DIG_MASK);
      _mm512_store_si512(pr + 26, R26);

      R27 = _mm512_add_epi64(R27, T);
      T = _mm512_srli_epi64(R27, DIGIT_SIZE);
      R27 = _mm512_and_epi64(R27, DIG_MASK);
      _mm512_store_si512(pr + 27, R27);

      R28 = _mm512_add_epi64(R28, T);
      T = _mm512_srli_epi64(R28, DIGIT_SIZE);
      R28 = _mm512_and_epi64(R28, DIG_MASK);
      _mm512_store_si512(pr + 28, R28);

      R29 = _mm512_add_epi64(R29, T);
      T = _mm512_srli_epi64(R29, DIGIT_SIZE);
      R29 = _mm512_and_epi64(R29, DIG_MASK);
      _mm512_store_si512(pr + 29, R29);

      R30 = _mm512_add_epi64(R30, T);
      T = _mm512_srli_epi64(R30, DIGIT_SIZE);
      R30 = _mm512_and_epi64(R30, DIG_MASK);
      _mm512_store_si512(pr + 30, R30);

      R31 = _mm512_add_epi64(R31, T);
      T = _mm512_srli_epi64(R31, DIGIT_SIZE);
      R31 = _mm512_and_epi64(R31, DIG_MASK);
      _mm512_store_si512(pr + 31, R31);

      R32 = _mm512_add_epi64(R32, T);
      T = _mm512_srli_epi64(R32, DIGIT_SIZE);
      R32 = _mm512_and_epi64(R32, DIG_MASK);
      _mm512_store_si512(pr + 32, R32);

      R33 = _mm512_add_epi64(R33, T);
      T = _mm512_srli_epi64(R33, DIGIT_SIZE);
      R33 = _mm512_and_epi64(R33, DIG_MASK);
      _mm512_store_si512(pr + 33, R33);

      R34 = _mm512_add_epi64(R34, T);
      T = _mm512_srli_epi64(R34, DIGIT_SIZE);
      R34 = _mm512_and_epi64(R34, DIG_MASK);
      _mm512_store_si512(pr + 34, R34);

      R35 = _mm512_add_epi64(R35, T);
      T = _mm512_srli_epi64(R35, DIGIT_SIZE);
      R35 = _mm512_and_epi64(R35, DIG_MASK);
      _mm512_store_si512(pr + 35, R35);

      R36 = _mm512_add_epi64(R36, T);
      T = _mm512_srli_epi64(R36, DIGIT_SIZE);
      R36 = _mm512_and_epi64(R36, DIG_MASK);
      _mm512_store_si512(pr + 36, R36);

      R37 = _mm512_add_epi64(R37, T);
      T = _mm512_srli_epi64(R37, DIGIT_SIZE);
      R37 = _mm512_and_epi64(R37, DIG_MASK);
      _mm512_store_si512(pr + 37, R37);

      R38 = _mm512_add_epi64(R38, T);
      T = _mm512_srli_epi64(R38, DIGIT_SIZE);
      R38 = _mm512_and_epi64(R38, MS_DIG_MASK);
      _mm512_store_si512(pr + 38, R38);
   }

   #undef BITSIZE
   #undef LEN52
   #undef MSD_MSK
}


/* r = x * (R^-1) mod q */
void ifma_amred52x10_mb8(int64u res[][8],
   const int64u inpA[][8], /* int nsA == 2*nsM */
   const int64u inpM[][8], /* int nsM ==10 */
   const int64u k0[8])
{
   int64u* pA = (int64u*)inpA;
   int64u* pM = (int64u*)inpM;
   int64u* pR = (int64u*)res;

   __m512i K = _mm512_load_si512(k0);     /* k0[] */

   __m512i R00 = _mm512_load_si512(pA + 8 * 0); /* load A[nsM-1],...,A[0] */
   __m512i R01 = _mm512_load_si512(pA + 8 * 1);
   __m512i R02 = _mm512_load_si512(pA + 8 * 2);
   __m512i R03 = _mm512_load_si512(pA + 8 * 3);
   __m512i R04 = _mm512_load_si512(pA + 8 * 4);
   __m512i R05 = _mm512_load_si512(pA + 8 * 5);
   __m512i R06 = _mm512_load_si512(pA + 8 * 6);
   __m512i R07 = _mm512_load_si512(pA + 8 * 7);
   __m512i R08 = _mm512_load_si512(pA + 8 * 8);
   __m512i R09 = _mm512_load_si512(pA + 8 * 9);

   int itr;
   for (itr = 0, pA += 8 * 10; itr < 10; itr++, pA += 8) {
      __m512i Yi = _mm512_madd52lo_epu64(_mm512_setzero_si512(), R00, K);
      __m512i nxtA = _mm512_load_si512(pA);

      _mm512_madd52lo_epu64_(R00, R00, Yi, pM, 64 * 0);
      _mm512_madd52lo_epu64_(R01, R01, Yi, pM, 64 * 1);
      _mm512_madd52lo_epu64_(R02, R02, Yi, pM, 64 * 2);
      _mm512_madd52lo_epu64_(R03, R03, Yi, pM, 64 * 3);
      _mm512_madd52lo_epu64_(R04, R04, Yi, pM, 64 * 4);
      _mm512_madd52lo_epu64_(R05, R05, Yi, pM, 64 * 5);
      _mm512_madd52lo_epu64_(R06, R06, Yi, pM, 64 * 6);
      _mm512_madd52lo_epu64_(R07, R07, Yi, pM, 64 * 7);
      _mm512_madd52lo_epu64_(R08, R08, Yi, pM, 64 * 8);
      _mm512_madd52lo_epu64_(R09, R09, Yi, pM, 64 * 9);

      R00 = _mm512_srli_epi64(R00, DIGIT_SIZE);
      R01 = _mm512_add_epi64(R01, R00);

      _mm512_madd52hi_epu64_(R00, R01, Yi, pM, 64 * 0);
      _mm512_madd52hi_epu64_(R01, R02, Yi, pM, 64 * 1);
      _mm512_madd52hi_epu64_(R02, R03, Yi, pM, 64 * 2);
      _mm512_madd52hi_epu64_(R03, R04, Yi, pM, 64 * 3);
      _mm512_madd52hi_epu64_(R04, R05, Yi, pM, 64 * 4);
      _mm512_madd52hi_epu64_(R05, R06, Yi, pM, 64 * 5);
      _mm512_madd52hi_epu64_(R06, R07, Yi, pM, 64 * 6);
      _mm512_madd52hi_epu64_(R07, R08, Yi, pM, 64 * 7);
      _mm512_madd52hi_epu64_(R08, R09, Yi, pM, 64 * 8);
      _mm512_madd52hi_epu64_(R09, nxtA, Yi, pM, 64 * 9);
   }

   /* normalization */
   {
      __m512i MASK = _mm512_set1_epi64(DIGIT_MASK);

      __m512i
         T = _mm512_srli_epi64(R00, DIGIT_SIZE);
      R00 = _mm512_and_epi64(R00, MASK);
      _mm512_store_si512(pR + 8 * 0, R00);

      R01 = _mm512_add_epi64(R01, T);
      T = _mm512_srli_epi64(R01, DIGIT_SIZE);
      R01 = _mm512_and_epi64(R01, MASK);
      _mm512_store_si512(pR + 8 * 1, R01);

      R02 = _mm512_add_epi64(R02, T);
      T = _mm512_srli_epi64(R02, DIGIT_SIZE);
      R02 = _mm512_and_epi64(R02, MASK);
      _mm512_store_si512(pR + 8 * 2, R02);

      R03 = _mm512_add_epi64(R03, T);
      T = _mm512_srli_epi64(R03, DIGIT_SIZE);
      R03 = _mm512_and_epi64(R03, MASK);
      _mm512_store_si512(pR + 8 * 3, R03);

      R04 = _mm512_add_epi64(R04, T);
      T = _mm512_srli_epi64(R04, DIGIT_SIZE);
      R04 = _mm512_and_epi64(R04, MASK);
      _mm512_store_si512(pR + 8 * 4, R04);

      R05 = _mm512_add_epi64(R05, T);
      T = _mm512_srli_epi64(R05, DIGIT_SIZE);
      R05 = _mm512_and_epi64(R05, MASK);
      _mm512_store_si512(pR + 8 * 5, R05);

      R06 = _mm512_add_epi64(R06, T);
      T = _mm512_srli_epi64(R06, DIGIT_SIZE);
      R06 = _mm512_and_epi64(R06, MASK);
      _mm512_store_si512(pR + 8 * 6, R06);

      R07 = _mm512_add_epi64(R07, T);
      T = _mm512_srli_epi64(R07, DIGIT_SIZE);
      R07 = _mm512_and_epi64(R07, MASK);
      _mm512_store_si512(pR + 8 * 7, R07);

      R08 = _mm512_add_epi64(R08, T);
      T = _mm512_srli_epi64(R08, DIGIT_SIZE);
      R08 = _mm512_and_epi64(R08, MASK);
      _mm512_store_si512(pR + 8 * 8, R08);

      R09 = _mm512_add_epi64(R09, T);
      T = _mm512_srli_epi64(R09, DIGIT_SIZE);
      R09 = _mm512_and_epi64(R09, MASK);
      _mm512_store_si512(pR + 8 * 9, R09);
   }
}

void ifma_amred52x20_mb8(int64u res[][8],
                   const int64u inpA[][8], /* int nsA == 2*nsM */
                   const int64u inpM[][8], /* int nsM ==20 */
                   const int64u k0[8])
{
   int64u* pA = (int64u*)inpA;
   int64u* pM = (int64u*)inpM;
   int64u* pR = (int64u*)res;

   __m512i K = _mm512_load_si512(k0);     /* k0[] */

   __m512i R00 = _mm512_load_si512(pA+8*0); /* load A[nsM-1],...,A[0] */
   __m512i R01 = _mm512_load_si512(pA+8*1);
   __m512i R02 = _mm512_load_si512(pA+8*2);
   __m512i R03 = _mm512_load_si512(pA+8*3);
   __m512i R04 = _mm512_load_si512(pA+8*4);
   __m512i R05 = _mm512_load_si512(pA+8*5);
   __m512i R06 = _mm512_load_si512(pA+8*6);
   __m512i R07 = _mm512_load_si512(pA+8*7);
   __m512i R08 = _mm512_load_si512(pA+8*8);
   __m512i R09 = _mm512_load_si512(pA+8*9);
   __m512i R10 = _mm512_load_si512(pA+8*10);
   __m512i R11 = _mm512_load_si512(pA+8*11);
   __m512i R12 = _mm512_load_si512(pA+8*12);
   __m512i R13 = _mm512_load_si512(pA+8*13);
   __m512i R14 = _mm512_load_si512(pA+8*14);
   __m512i R15 = _mm512_load_si512(pA+8*15);
   __m512i R16 = _mm512_load_si512(pA+8*16);
   __m512i R17 = _mm512_load_si512(pA+8*17);
   __m512i R18 = _mm512_load_si512(pA+8*18);
   __m512i R19 = _mm512_load_si512(pA+8*19);
   
   int itr;
   for(itr=0, pA+=8*20; itr<20; itr++, pA+=8) {
      __m512i Yi = _mm512_madd52lo_epu64(_mm512_setzero_si512(), R00, K);
      __m512i nxtA = _mm512_load_si512(pA);

      _mm512_madd52lo_epu64_(R00, R00, Yi, pM, 64*0);
      _mm512_madd52lo_epu64_(R01, R01, Yi, pM, 64*1);
      _mm512_madd52lo_epu64_(R02, R02, Yi, pM, 64*2);
      _mm512_madd52lo_epu64_(R03, R03, Yi, pM, 64*3);
      _mm512_madd52lo_epu64_(R04, R04, Yi, pM, 64*4);
      _mm512_madd52lo_epu64_(R05, R05, Yi, pM, 64*5);
      _mm512_madd52lo_epu64_(R06, R06, Yi, pM, 64*6);
      _mm512_madd52lo_epu64_(R07, R07, Yi, pM, 64*7);
      _mm512_madd52lo_epu64_(R08, R08, Yi, pM, 64*8);
      _mm512_madd52lo_epu64_(R09, R09, Yi, pM, 64*9);
      _mm512_madd52lo_epu64_(R10, R10, Yi, pM, 64*10);
      _mm512_madd52lo_epu64_(R11, R11, Yi, pM, 64*11);
      _mm512_madd52lo_epu64_(R12, R12, Yi, pM, 64*12);
      _mm512_madd52lo_epu64_(R13, R13, Yi, pM, 64*13);
      _mm512_madd52lo_epu64_(R14, R14, Yi, pM, 64*14);
      _mm512_madd52lo_epu64_(R15, R15, Yi, pM, 64*15);
      _mm512_madd52lo_epu64_(R16, R16, Yi, pM, 64*16);
      _mm512_madd52lo_epu64_(R17, R17, Yi, pM, 64*17);
      _mm512_madd52lo_epu64_(R18, R18, Yi, pM, 64*18);
      _mm512_madd52lo_epu64_(R19, R19, Yi, pM, 64*19);

      R00 = _mm512_srli_epi64(R00, DIGIT_SIZE);
      R01 = _mm512_add_epi64(R01, R00);

      _mm512_madd52hi_epu64_(R00, R01, Yi, pM, 64*0);
      _mm512_madd52hi_epu64_(R01, R02, Yi, pM, 64*1);
      _mm512_madd52hi_epu64_(R02, R03, Yi, pM, 64*2);
      _mm512_madd52hi_epu64_(R03, R04, Yi, pM, 64*3);
      _mm512_madd52hi_epu64_(R04, R05, Yi, pM, 64*4);
      _mm512_madd52hi_epu64_(R05, R06, Yi, pM, 64*5);
      _mm512_madd52hi_epu64_(R06, R07, Yi, pM, 64*6);
      _mm512_madd52hi_epu64_(R07, R08, Yi, pM, 64*7);
      _mm512_madd52hi_epu64_(R08, R09, Yi, pM, 64*8);
      _mm512_madd52hi_epu64_(R09, R10, Yi, pM, 64*9);
      _mm512_madd52hi_epu64_(R10, R11, Yi, pM, 64*10);
      _mm512_madd52hi_epu64_(R11, R12, Yi, pM, 64*11);
      _mm512_madd52hi_epu64_(R12, R13, Yi, pM, 64*12);
      _mm512_madd52hi_epu64_(R13, R14, Yi, pM, 64*13);
      _mm512_madd52hi_epu64_(R14, R15, Yi, pM, 64*14);
      _mm512_madd52hi_epu64_(R15, R16, Yi, pM, 64*15);
      _mm512_madd52hi_epu64_(R16, R17, Yi, pM, 64*16);
      _mm512_madd52hi_epu64_(R17, R18, Yi, pM, 64*17);
      _mm512_madd52hi_epu64_(R18, R19, Yi, pM, 64*18);
      _mm512_madd52hi_epu64_(R19, nxtA,Yi, pM, 64*19);
   }

   /* normalization */
   {
      __m512i MASK = _mm512_set1_epi64(DIGIT_MASK);

      __m512i
      T = _mm512_srli_epi64(R00, DIGIT_SIZE);
      R00 = _mm512_and_epi64(R00, MASK);
      _mm512_store_si512(pR+8*0, R00);

      R01 = _mm512_add_epi64(R01, T);
      T = _mm512_srli_epi64(R01, DIGIT_SIZE);
      R01 = _mm512_and_epi64(R01, MASK);
      _mm512_store_si512(pR+8*1, R01);

      R02 = _mm512_add_epi64(R02, T);
      T = _mm512_srli_epi64(R02, DIGIT_SIZE);
      R02 = _mm512_and_epi64(R02, MASK);
      _mm512_store_si512(pR+8*2, R02);

      R03 = _mm512_add_epi64(R03, T);
      T = _mm512_srli_epi64(R03, DIGIT_SIZE);
      R03 = _mm512_and_epi64(R03, MASK);
      _mm512_store_si512(pR+8*3, R03);

      R04 = _mm512_add_epi64(R04, T);
      T = _mm512_srli_epi64(R04, DIGIT_SIZE);
      R04 = _mm512_and_epi64(R04, MASK);
      _mm512_store_si512(pR+8*4, R04);

      R05 = _mm512_add_epi64(R05, T);
      T = _mm512_srli_epi64(R05, DIGIT_SIZE);
      R05 = _mm512_and_epi64(R05, MASK);
      _mm512_store_si512(pR+8*5, R05);

      R06 = _mm512_add_epi64(R06, T);
      T = _mm512_srli_epi64(R06, DIGIT_SIZE);
      R06 = _mm512_and_epi64(R06, MASK);
      _mm512_store_si512(pR+8*6, R06);

      R07 = _mm512_add_epi64(R07, T);
      T = _mm512_srli_epi64(R07, DIGIT_SIZE);
      R07 = _mm512_and_epi64(R07, MASK);
      _mm512_store_si512(pR+8*7, R07);

      R08 = _mm512_add_epi64(R08, T);
      T = _mm512_srli_epi64(R08, DIGIT_SIZE);
      R08 = _mm512_and_epi64(R08, MASK);
      _mm512_store_si512(pR+8*8, R08);

      R09 = _mm512_add_epi64(R09, T);
      T = _mm512_srli_epi64(R09, DIGIT_SIZE);
      R09 = _mm512_and_epi64(R09, MASK);
      _mm512_store_si512(pR+8*9, R09);

      R10 = _mm512_add_epi64(R10, T);
      T = _mm512_srli_epi64(R10, DIGIT_SIZE);
      R10 = _mm512_and_epi64(R10, MASK);
      _mm512_store_si512(pR+8*10, R10);

      R11 = _mm512_add_epi64(R11, T);
      T = _mm512_srli_epi64(R11, DIGIT_SIZE);
      R11 = _mm512_and_epi64(R11, MASK);
      _mm512_store_si512(pR+8*11, R11);

      R12 = _mm512_add_epi64(R12, T);
      T = _mm512_srli_epi64(R12, DIGIT_SIZE);
      R12 = _mm512_and_epi64(R12, MASK);
      _mm512_store_si512(pR+8*12, R12);

      R13 = _mm512_add_epi64(R13, T);
      T = _mm512_srli_epi64(R13, DIGIT_SIZE);
      R13 = _mm512_and_epi64(R13, MASK);
      _mm512_store_si512(pR+8*13, R13);

      R14 = _mm512_add_epi64(R14, T);
      T = _mm512_srli_epi64(R14, DIGIT_SIZE);
      R14 = _mm512_and_epi64(R14, MASK);
      _mm512_store_si512(pR+8*14, R14);

      R15 = _mm512_add_epi64(R15, T);
      T = _mm512_srli_epi64(R15, DIGIT_SIZE);
      R15 = _mm512_and_epi64(R15, MASK);
      _mm512_store_si512(pR+8*15, R15);

      R16 = _mm512_add_epi64(R16, T);
      T = _mm512_srli_epi64(R16, DIGIT_SIZE);
      R16 = _mm512_and_epi64(R16, MASK);
      _mm512_store_si512(pR+8*16, R16);

      R17 = _mm512_add_epi64(R17, T);
      T = _mm512_srli_epi64(R17, DIGIT_SIZE);
      R17 = _mm512_and_epi64(R17, MASK);
      _mm512_store_si512(pR+8*17, R17);

      R18 = _mm512_add_epi64(R18, T);
      T = _mm512_srli_epi64(R18, DIGIT_SIZE);
      R18 = _mm512_and_epi64(R18, MASK);
      _mm512_store_si512(pR+8*18, R18);

      R19 = _mm512_add_epi64(R19, T);
      T = _mm512_srli_epi64(R19, DIGIT_SIZE);
      R19 = _mm512_and_epi64(R19, MASK);
      _mm512_store_si512(pR+8*19, R19);
   }
}

void ifma_amred52x30_mb8(int64u res[][8],
   const int64u inpA[][8], /* int nsA == 2*nsM */
   const int64u inpM[][8], /* int nsM ==20 */
   const int64u k0[8])
{
   int64u* pA = (int64u*)inpA;
   int64u* pM = (int64u*)inpM;
   int64u* pR = (int64u*)res;

   __m512i K = _mm512_load_si512(k0);     /* k0[] */

   __m512i R00 = _mm512_load_si512(pA + 8 * 0); /* load A[nsM-1],...,A[0] */
   __m512i R01 = _mm512_load_si512(pA + 8 * 1);
   __m512i R02 = _mm512_load_si512(pA + 8 * 2);
   __m512i R03 = _mm512_load_si512(pA + 8 * 3);
   __m512i R04 = _mm512_load_si512(pA + 8 * 4);
   __m512i R05 = _mm512_load_si512(pA + 8 * 5);
   __m512i R06 = _mm512_load_si512(pA + 8 * 6);
   __m512i R07 = _mm512_load_si512(pA + 8 * 7);
   __m512i R08 = _mm512_load_si512(pA + 8 * 8);
   __m512i R09 = _mm512_load_si512(pA + 8 * 9);
   __m512i R10 = _mm512_load_si512(pA + 8 * 10);
   __m512i R11 = _mm512_load_si512(pA + 8 * 11);
   __m512i R12 = _mm512_load_si512(pA + 8 * 12);
   __m512i R13 = _mm512_load_si512(pA + 8 * 13);
   __m512i R14 = _mm512_load_si512(pA + 8 * 14);
   __m512i R15 = _mm512_load_si512(pA + 8 * 15);
   __m512i R16 = _mm512_load_si512(pA + 8 * 16);
   __m512i R17 = _mm512_load_si512(pA + 8 * 17);
   __m512i R18 = _mm512_load_si512(pA + 8 * 18);
   __m512i R19 = _mm512_load_si512(pA + 8 * 19);
   __m512i R20 = _mm512_load_si512(pA + 8 * 20);
   __m512i R21 = _mm512_load_si512(pA + 8 * 21);
   __m512i R22 = _mm512_load_si512(pA + 8 * 22);
   __m512i R23 = _mm512_load_si512(pA + 8 * 23);
   __m512i R24 = _mm512_load_si512(pA + 8 * 24);
   __m512i R25 = _mm512_load_si512(pA + 8 * 25);
   __m512i R26 = _mm512_load_si512(pA + 8 * 26);
   __m512i R27 = _mm512_load_si512(pA + 8 * 27);
   __m512i R28 = _mm512_load_si512(pA + 8 * 28);
   __m512i R29 = _mm512_load_si512(pA + 8 * 29);

   int itr;
   for (itr = 0, pA += 8 * 30; itr < 30; itr++, pA += 8) {
      __m512i Yi = _mm512_madd52lo_epu64(_mm512_setzero_si512(), R00, K);
      __m512i nxtA = _mm512_load_si512(pA);

      _mm512_madd52lo_epu64_(R00, R00, Yi, pM, 64 * 0);
      _mm512_madd52lo_epu64_(R01, R01, Yi, pM, 64 * 1);
      _mm512_madd52lo_epu64_(R02, R02, Yi, pM, 64 * 2);
      _mm512_madd52lo_epu64_(R03, R03, Yi, pM, 64 * 3);
      _mm512_madd52lo_epu64_(R04, R04, Yi, pM, 64 * 4);
      _mm512_madd52lo_epu64_(R05, R05, Yi, pM, 64 * 5);
      _mm512_madd52lo_epu64_(R06, R06, Yi, pM, 64 * 6);
      _mm512_madd52lo_epu64_(R07, R07, Yi, pM, 64 * 7);
      _mm512_madd52lo_epu64_(R08, R08, Yi, pM, 64 * 8);
      _mm512_madd52lo_epu64_(R09, R09, Yi, pM, 64 * 9);
      _mm512_madd52lo_epu64_(R10, R10, Yi, pM, 64 * 10);
      _mm512_madd52lo_epu64_(R11, R11, Yi, pM, 64 * 11);
      _mm512_madd52lo_epu64_(R12, R12, Yi, pM, 64 * 12);
      _mm512_madd52lo_epu64_(R13, R13, Yi, pM, 64 * 13);
      _mm512_madd52lo_epu64_(R14, R14, Yi, pM, 64 * 14);
      _mm512_madd52lo_epu64_(R15, R15, Yi, pM, 64 * 15);
      _mm512_madd52lo_epu64_(R16, R16, Yi, pM, 64 * 16);
      _mm512_madd52lo_epu64_(R17, R17, Yi, pM, 64 * 17);
      _mm512_madd52lo_epu64_(R18, R18, Yi, pM, 64 * 18);
      _mm512_madd52lo_epu64_(R19, R19, Yi, pM, 64 * 19);
      _mm512_madd52lo_epu64_(R20, R20, Yi, pM, 64 * 20);
      _mm512_madd52lo_epu64_(R21, R21, Yi, pM, 64 * 21);
      _mm512_madd52lo_epu64_(R22, R22, Yi, pM, 64 * 22);
      _mm512_madd52lo_epu64_(R23, R23, Yi, pM, 64 * 23);
      _mm512_madd52lo_epu64_(R24, R24, Yi, pM, 64 * 24);
      _mm512_madd52lo_epu64_(R25, R25, Yi, pM, 64 * 25);
      _mm512_madd52lo_epu64_(R26, R26, Yi, pM, 64 * 26);
      _mm512_madd52lo_epu64_(R27, R27, Yi, pM, 64 * 27);
      _mm512_madd52lo_epu64_(R28, R28, Yi, pM, 64 * 28);
      _mm512_madd52lo_epu64_(R29, R29, Yi, pM, 64 * 29);

      R00 = _mm512_srli_epi64(R00, DIGIT_SIZE);
      R01 = _mm512_add_epi64(R01, R00);

      _mm512_madd52hi_epu64_(R00, R01, Yi, pM, 64 * 0);
      _mm512_madd52hi_epu64_(R01, R02, Yi, pM, 64 * 1);
      _mm512_madd52hi_epu64_(R02, R03, Yi, pM, 64 * 2);
      _mm512_madd52hi_epu64_(R03, R04, Yi, pM, 64 * 3);
      _mm512_madd52hi_epu64_(R04, R05, Yi, pM, 64 * 4);
      _mm512_madd52hi_epu64_(R05, R06, Yi, pM, 64 * 5);
      _mm512_madd52hi_epu64_(R06, R07, Yi, pM, 64 * 6);
      _mm512_madd52hi_epu64_(R07, R08, Yi, pM, 64 * 7);
      _mm512_madd52hi_epu64_(R08, R09, Yi, pM, 64 * 8);
      _mm512_madd52hi_epu64_(R09, R10, Yi, pM, 64 * 9);
      _mm512_madd52hi_epu64_(R10, R11, Yi, pM, 64 * 10);
      _mm512_madd52hi_epu64_(R11, R12, Yi, pM, 64 * 11);
      _mm512_madd52hi_epu64_(R12, R13, Yi, pM, 64 * 12);
      _mm512_madd52hi_epu64_(R13, R14, Yi, pM, 64 * 13);
      _mm512_madd52hi_epu64_(R14, R15, Yi, pM, 64 * 14);
      _mm512_madd52hi_epu64_(R15, R16, Yi, pM, 64 * 15);
      _mm512_madd52hi_epu64_(R16, R17, Yi, pM, 64 * 16);
      _mm512_madd52hi_epu64_(R17, R18, Yi, pM, 64 * 17);
      _mm512_madd52hi_epu64_(R18, R19, Yi, pM, 64 * 18);
      _mm512_madd52hi_epu64_(R19, R20, Yi, pM, 64 * 19);
      _mm512_madd52hi_epu64_(R20, R21, Yi, pM, 64 * 20);
      _mm512_madd52hi_epu64_(R21, R22, Yi, pM, 64 * 21);
      _mm512_madd52hi_epu64_(R22, R23, Yi, pM, 64 * 22);
      _mm512_madd52hi_epu64_(R23, R24, Yi, pM, 64 * 23);
      _mm512_madd52hi_epu64_(R24, R25, Yi, pM, 64 * 24);
      _mm512_madd52hi_epu64_(R25, R26, Yi, pM, 64 * 25);
      _mm512_madd52hi_epu64_(R26, R27, Yi, pM, 64 * 26);
      _mm512_madd52hi_epu64_(R27, R28, Yi, pM, 64 * 27);
      _mm512_madd52hi_epu64_(R28, R29, Yi, pM, 64 * 28);
      _mm512_madd52hi_epu64_(R29, nxtA, Yi, pM, 64 * 29);
   }

   /* normalization */
   {
      __m512i MASK = _mm512_set1_epi64(DIGIT_MASK);

      __m512i
         T = _mm512_srli_epi64(R00, DIGIT_SIZE);
      R00 = _mm512_and_epi64(R00, MASK);
      _mm512_store_si512(pR + 8 * 0, R00);

      R01 = _mm512_add_epi64(R01, T);
      T = _mm512_srli_epi64(R01, DIGIT_SIZE);
      R01 = _mm512_and_epi64(R01, MASK);
      _mm512_store_si512(pR + 8 * 1, R01);

      R02 = _mm512_add_epi64(R02, T);
      T = _mm512_srli_epi64(R02, DIGIT_SIZE);
      R02 = _mm512_and_epi64(R02, MASK);
      _mm512_store_si512(pR + 8 * 2, R02);

      R03 = _mm512_add_epi64(R03, T);
      T = _mm512_srli_epi64(R03, DIGIT_SIZE);
      R03 = _mm512_and_epi64(R03, MASK);
      _mm512_store_si512(pR + 8 * 3, R03);

      R04 = _mm512_add_epi64(R04, T);
      T = _mm512_srli_epi64(R04, DIGIT_SIZE);
      R04 = _mm512_and_epi64(R04, MASK);
      _mm512_store_si512(pR + 8 * 4, R04);

      R05 = _mm512_add_epi64(R05, T);
      T = _mm512_srli_epi64(R05, DIGIT_SIZE);
      R05 = _mm512_and_epi64(R05, MASK);
      _mm512_store_si512(pR + 8 * 5, R05);

      R06 = _mm512_add_epi64(R06, T);
      T = _mm512_srli_epi64(R06, DIGIT_SIZE);
      R06 = _mm512_and_epi64(R06, MASK);
      _mm512_store_si512(pR + 8 * 6, R06);

      R07 = _mm512_add_epi64(R07, T);
      T = _mm512_srli_epi64(R07, DIGIT_SIZE);
      R07 = _mm512_and_epi64(R07, MASK);
      _mm512_store_si512(pR + 8 * 7, R07);

      R08 = _mm512_add_epi64(R08, T);
      T = _mm512_srli_epi64(R08, DIGIT_SIZE);
      R08 = _mm512_and_epi64(R08, MASK);
      _mm512_store_si512(pR + 8 * 8, R08);

      R09 = _mm512_add_epi64(R09, T);
      T = _mm512_srli_epi64(R09, DIGIT_SIZE);
      R09 = _mm512_and_epi64(R09, MASK);
      _mm512_store_si512(pR + 8 * 9, R09);

      R10 = _mm512_add_epi64(R10, T);
      T = _mm512_srli_epi64(R10, DIGIT_SIZE);
      R10 = _mm512_and_epi64(R10, MASK);
      _mm512_store_si512(pR + 8 * 10, R10);

      R11 = _mm512_add_epi64(R11, T);
      T = _mm512_srli_epi64(R11, DIGIT_SIZE);
      R11 = _mm512_and_epi64(R11, MASK);
      _mm512_store_si512(pR + 8 * 11, R11);

      R12 = _mm512_add_epi64(R12, T);
      T = _mm512_srli_epi64(R12, DIGIT_SIZE);
      R12 = _mm512_and_epi64(R12, MASK);
      _mm512_store_si512(pR + 8 * 12, R12);

      R13 = _mm512_add_epi64(R13, T);
      T = _mm512_srli_epi64(R13, DIGIT_SIZE);
      R13 = _mm512_and_epi64(R13, MASK);
      _mm512_store_si512(pR + 8 * 13, R13);

      R14 = _mm512_add_epi64(R14, T);
      T = _mm512_srli_epi64(R14, DIGIT_SIZE);
      R14 = _mm512_and_epi64(R14, MASK);
      _mm512_store_si512(pR + 8 * 14, R14);

      R15 = _mm512_add_epi64(R15, T);
      T = _mm512_srli_epi64(R15, DIGIT_SIZE);
      R15 = _mm512_and_epi64(R15, MASK);
      _mm512_store_si512(pR + 8 * 15, R15);

      R16 = _mm512_add_epi64(R16, T);
      T = _mm512_srli_epi64(R16, DIGIT_SIZE);
      R16 = _mm512_and_epi64(R16, MASK);
      _mm512_store_si512(pR + 8 * 16, R16);

      R17 = _mm512_add_epi64(R17, T);
      T = _mm512_srli_epi64(R17, DIGIT_SIZE);
      R17 = _mm512_and_epi64(R17, MASK);
      _mm512_store_si512(pR + 8 * 17, R17);

      R18 = _mm512_add_epi64(R18, T);
      T = _mm512_srli_epi64(R18, DIGIT_SIZE);
      R18 = _mm512_and_epi64(R18, MASK);
      _mm512_store_si512(pR + 8 * 18, R18);

      R19 = _mm512_add_epi64(R19, T);
      T = _mm512_srli_epi64(R19, DIGIT_SIZE);
      R19 = _mm512_and_epi64(R19, MASK);
      _mm512_store_si512(pR + 8 * 19, R19);

      R20 = _mm512_add_epi64(R20, T);
      T = _mm512_srli_epi64(R20, DIGIT_SIZE);
      R20 = _mm512_and_epi64(R20, MASK);
      _mm512_store_si512(pR + 8 * 20, R20);

      R21 = _mm512_add_epi64(R21, T);
      T = _mm512_srli_epi64(R21, DIGIT_SIZE);
      R21 = _mm512_and_epi64(R21, MASK);
      _mm512_store_si512(pR + 8 * 21, R21);

      R22 = _mm512_add_epi64(R22, T);
      T = _mm512_srli_epi64(R22, DIGIT_SIZE);
      R22 = _mm512_and_epi64(R22, MASK);
      _mm512_store_si512(pR + 8 * 22, R22);

      R23 = _mm512_add_epi64(R23, T);
      T = _mm512_srli_epi64(R23, DIGIT_SIZE);
      R23 = _mm512_and_epi64(R23, MASK);
      _mm512_store_si512(pR + 8 * 23, R23);

      R24 = _mm512_add_epi64(R24, T);
      T = _mm512_srli_epi64(R24, DIGIT_SIZE);
      R24 = _mm512_and_epi64(R24, MASK);
      _mm512_store_si512(pR + 8 * 24, R24);

      R25 = _mm512_add_epi64(R25, T);
      T = _mm512_srli_epi64(R25, DIGIT_SIZE);
      R25 = _mm512_and_epi64(R25, MASK);
      _mm512_store_si512(pR + 8 * 25, R25);

      R26 = _mm512_add_epi64(R26, T);
      T = _mm512_srli_epi64(R26, DIGIT_SIZE);
      R26 = _mm512_and_epi64(R26, MASK);
      _mm512_store_si512(pR + 8 * 26, R26);

      R27 = _mm512_add_epi64(R27, T);
      T = _mm512_srli_epi64(R27, DIGIT_SIZE);
      R27 = _mm512_and_epi64(R27, MASK);
      _mm512_store_si512(pR + 8 * 27, R27);

      R28 = _mm512_add_epi64(R28, T);
      T = _mm512_srli_epi64(R28, DIGIT_SIZE);
      R28 = _mm512_and_epi64(R28, MASK);
      _mm512_store_si512(pR + 8 * 28, R28);

      R29 = _mm512_add_epi64(R29, T);
      T = _mm512_srli_epi64(R29, DIGIT_SIZE);
      R29 = _mm512_and_epi64(R29, MASK);
      _mm512_store_si512(pR + 8 * 29, R29);
   }
}

void ifma_amred52x40_mb8(int64u res[][8],
   const int64u inpA[][8], /* int nsA == 2*nsM */
   const int64u inpM[][8], /* int nsM ==20 */
   const int64u k0[8])
{
   int64u* pA = (int64u*)inpA;
   int64u* pM = (int64u*)inpM;
   int64u* pR = (int64u*)res;

   __m512i K = _mm512_load_si512(k0);     /* k0[] */

   __m512i R00 = _mm512_load_si512(pA + 8 * 0); /* load A[nsM-1],...,A[0] */
   __m512i R01 = _mm512_load_si512(pA + 8 * 1);
   __m512i R02 = _mm512_load_si512(pA + 8 * 2);
   __m512i R03 = _mm512_load_si512(pA + 8 * 3);
   __m512i R04 = _mm512_load_si512(pA + 8 * 4);
   __m512i R05 = _mm512_load_si512(pA + 8 * 5);
   __m512i R06 = _mm512_load_si512(pA + 8 * 6);
   __m512i R07 = _mm512_load_si512(pA + 8 * 7);
   __m512i R08 = _mm512_load_si512(pA + 8 * 8);
   __m512i R09 = _mm512_load_si512(pA + 8 * 9);
   __m512i R10 = _mm512_load_si512(pA + 8 * 10);
   __m512i R11 = _mm512_load_si512(pA + 8 * 11);
   __m512i R12 = _mm512_load_si512(pA + 8 * 12);
   __m512i R13 = _mm512_load_si512(pA + 8 * 13);
   __m512i R14 = _mm512_load_si512(pA + 8 * 14);
   __m512i R15 = _mm512_load_si512(pA + 8 * 15);
   __m512i R16 = _mm512_load_si512(pA + 8 * 16);
   __m512i R17 = _mm512_load_si512(pA + 8 * 17);
   __m512i R18 = _mm512_load_si512(pA + 8 * 18);
   __m512i R19 = _mm512_load_si512(pA + 8 * 19);
   __m512i R20 = _mm512_load_si512(pA + 8 * 20);
   __m512i R21 = _mm512_load_si512(pA + 8 * 21);
   __m512i R22 = _mm512_load_si512(pA + 8 * 22);
   __m512i R23 = _mm512_load_si512(pA + 8 * 23);
   __m512i R24 = _mm512_load_si512(pA + 8 * 24);
   __m512i R25 = _mm512_load_si512(pA + 8 * 25);
   __m512i R26 = _mm512_load_si512(pA + 8 * 26);
   __m512i R27 = _mm512_load_si512(pA + 8 * 27);
   __m512i R28 = _mm512_load_si512(pA + 8 * 28);
   __m512i R29 = _mm512_load_si512(pA + 8 * 29);
   __m512i R30 = _mm512_load_si512(pA + 8 * 30);
   __m512i R31 = _mm512_load_si512(pA + 8 * 31);
   __m512i R32 = _mm512_load_si512(pA + 8 * 32);
   __m512i R33 = _mm512_load_si512(pA + 8 * 33);
   __m512i R34 = _mm512_load_si512(pA + 8 * 34);
   __m512i R35 = _mm512_load_si512(pA + 8 * 35);
   __m512i R36 = _mm512_load_si512(pA + 8 * 36);
   __m512i R37 = _mm512_load_si512(pA + 8 * 37);
   __m512i R38 = _mm512_load_si512(pA + 8 * 38);
   __m512i R39 = _mm512_load_si512(pA + 8 * 39);

   int itr;
   for (itr = 0, pA += 8 * 40; itr < 40; itr++, pA += 8) {
      __m512i Yi = _mm512_madd52lo_epu64(_mm512_setzero_si512(), R00, K);
      __m512i nxtA = _mm512_load_si512(pA);

      _mm512_madd52lo_epu64_(R00, R00, Yi, pM, 64 * 0);
      _mm512_madd52lo_epu64_(R01, R01, Yi, pM, 64 * 1);
      _mm512_madd52lo_epu64_(R02, R02, Yi, pM, 64 * 2);
      _mm512_madd52lo_epu64_(R03, R03, Yi, pM, 64 * 3);
      _mm512_madd52lo_epu64_(R04, R04, Yi, pM, 64 * 4);
      _mm512_madd52lo_epu64_(R05, R05, Yi, pM, 64 * 5);
      _mm512_madd52lo_epu64_(R06, R06, Yi, pM, 64 * 6);
      _mm512_madd52lo_epu64_(R07, R07, Yi, pM, 64 * 7);
      _mm512_madd52lo_epu64_(R08, R08, Yi, pM, 64 * 8);
      _mm512_madd52lo_epu64_(R09, R09, Yi, pM, 64 * 9);
      _mm512_madd52lo_epu64_(R10, R10, Yi, pM, 64 * 10);
      _mm512_madd52lo_epu64_(R11, R11, Yi, pM, 64 * 11);
      _mm512_madd52lo_epu64_(R12, R12, Yi, pM, 64 * 12);
      _mm512_madd52lo_epu64_(R13, R13, Yi, pM, 64 * 13);
      _mm512_madd52lo_epu64_(R14, R14, Yi, pM, 64 * 14);
      _mm512_madd52lo_epu64_(R15, R15, Yi, pM, 64 * 15);
      _mm512_madd52lo_epu64_(R16, R16, Yi, pM, 64 * 16);
      _mm512_madd52lo_epu64_(R17, R17, Yi, pM, 64 * 17);
      _mm512_madd52lo_epu64_(R18, R18, Yi, pM, 64 * 18);
      _mm512_madd52lo_epu64_(R19, R19, Yi, pM, 64 * 19);
      _mm512_madd52lo_epu64_(R20, R20, Yi, pM, 64 * 20);
      _mm512_madd52lo_epu64_(R21, R21, Yi, pM, 64 * 21);
      _mm512_madd52lo_epu64_(R22, R22, Yi, pM, 64 * 22);
      _mm512_madd52lo_epu64_(R23, R23, Yi, pM, 64 * 23);
      _mm512_madd52lo_epu64_(R24, R24, Yi, pM, 64 * 24);
      _mm512_madd52lo_epu64_(R25, R25, Yi, pM, 64 * 25);
      _mm512_madd52lo_epu64_(R26, R26, Yi, pM, 64 * 26);
      _mm512_madd52lo_epu64_(R27, R27, Yi, pM, 64 * 27);
      _mm512_madd52lo_epu64_(R28, R28, Yi, pM, 64 * 28);
      _mm512_madd52lo_epu64_(R29, R29, Yi, pM, 64 * 29);
      _mm512_madd52lo_epu64_(R30, R30, Yi, pM, 64 * 30);
      _mm512_madd52lo_epu64_(R31, R31, Yi, pM, 64 * 31);
      _mm512_madd52lo_epu64_(R32, R32, Yi, pM, 64 * 32);
      _mm512_madd52lo_epu64_(R33, R33, Yi, pM, 64 * 33);
      _mm512_madd52lo_epu64_(R34, R34, Yi, pM, 64 * 34);
      _mm512_madd52lo_epu64_(R35, R35, Yi, pM, 64 * 35);
      _mm512_madd52lo_epu64_(R36, R36, Yi, pM, 64 * 36);
      _mm512_madd52lo_epu64_(R37, R37, Yi, pM, 64 * 37);
      _mm512_madd52lo_epu64_(R38, R38, Yi, pM, 64 * 38);
      _mm512_madd52lo_epu64_(R39, R39, Yi, pM, 64 * 39);

      R00 = _mm512_srli_epi64(R00, DIGIT_SIZE);
      R01 = _mm512_add_epi64(R01, R00);

      _mm512_madd52hi_epu64_(R00, R01, Yi, pM, 64 * 0);
      _mm512_madd52hi_epu64_(R01, R02, Yi, pM, 64 * 1);
      _mm512_madd52hi_epu64_(R02, R03, Yi, pM, 64 * 2);
      _mm512_madd52hi_epu64_(R03, R04, Yi, pM, 64 * 3);
      _mm512_madd52hi_epu64_(R04, R05, Yi, pM, 64 * 4);
      _mm512_madd52hi_epu64_(R05, R06, Yi, pM, 64 * 5);
      _mm512_madd52hi_epu64_(R06, R07, Yi, pM, 64 * 6);
      _mm512_madd52hi_epu64_(R07, R08, Yi, pM, 64 * 7);
      _mm512_madd52hi_epu64_(R08, R09, Yi, pM, 64 * 8);
      _mm512_madd52hi_epu64_(R09, R10, Yi, pM, 64 * 9);
      _mm512_madd52hi_epu64_(R10, R11, Yi, pM, 64 * 10);
      _mm512_madd52hi_epu64_(R11, R12, Yi, pM, 64 * 11);
      _mm512_madd52hi_epu64_(R12, R13, Yi, pM, 64 * 12);
      _mm512_madd52hi_epu64_(R13, R14, Yi, pM, 64 * 13);
      _mm512_madd52hi_epu64_(R14, R15, Yi, pM, 64 * 14);
      _mm512_madd52hi_epu64_(R15, R16, Yi, pM, 64 * 15);
      _mm512_madd52hi_epu64_(R16, R17, Yi, pM, 64 * 16);
      _mm512_madd52hi_epu64_(R17, R18, Yi, pM, 64 * 17);
      _mm512_madd52hi_epu64_(R18, R19, Yi, pM, 64 * 18);
      _mm512_madd52hi_epu64_(R19, R20, Yi, pM, 64 * 19);
      _mm512_madd52hi_epu64_(R20, R21, Yi, pM, 64 * 20);
      _mm512_madd52hi_epu64_(R21, R22, Yi, pM, 64 * 21);
      _mm512_madd52hi_epu64_(R22, R23, Yi, pM, 64 * 22);
      _mm512_madd52hi_epu64_(R23, R24, Yi, pM, 64 * 23);
      _mm512_madd52hi_epu64_(R24, R25, Yi, pM, 64 * 24);
      _mm512_madd52hi_epu64_(R25, R26, Yi, pM, 64 * 25);
      _mm512_madd52hi_epu64_(R26, R27, Yi, pM, 64 * 26);
      _mm512_madd52hi_epu64_(R27, R28, Yi, pM, 64 * 27);
      _mm512_madd52hi_epu64_(R28, R29, Yi, pM, 64 * 28);
      _mm512_madd52hi_epu64_(R29, R30, Yi, pM, 64 * 29);
      _mm512_madd52hi_epu64_(R30, R31, Yi, pM, 64 * 30);
      _mm512_madd52hi_epu64_(R31, R32, Yi, pM, 64 * 31);
      _mm512_madd52hi_epu64_(R32, R33, Yi, pM, 64 * 32);
      _mm512_madd52hi_epu64_(R33, R34, Yi, pM, 64 * 33);
      _mm512_madd52hi_epu64_(R34, R35, Yi, pM, 64 * 34);
      _mm512_madd52hi_epu64_(R35, R36, Yi, pM, 64 * 35);
      _mm512_madd52hi_epu64_(R36, R37, Yi, pM, 64 * 36);
      _mm512_madd52hi_epu64_(R37, R38, Yi, pM, 64 * 37);
      _mm512_madd52hi_epu64_(R38, R39, Yi, pM, 64 * 38);
      _mm512_madd52hi_epu64_(R39, nxtA, Yi, pM, 64 * 39);
   }

   /* normalization */
   {
      __m512i MASK = _mm512_set1_epi64(DIGIT_MASK);

      __m512i
         T = _mm512_srli_epi64(R00, DIGIT_SIZE);
      R00 = _mm512_and_epi64(R00, MASK);
      _mm512_store_si512(pR + 8 * 0, R00);

      R01 = _mm512_add_epi64(R01, T);
      T = _mm512_srli_epi64(R01, DIGIT_SIZE);
      R01 = _mm512_and_epi64(R01, MASK);
      _mm512_store_si512(pR + 8 * 1, R01);

      R02 = _mm512_add_epi64(R02, T);
      T = _mm512_srli_epi64(R02, DIGIT_SIZE);
      R02 = _mm512_and_epi64(R02, MASK);
      _mm512_store_si512(pR + 8 * 2, R02);

      R03 = _mm512_add_epi64(R03, T);
      T = _mm512_srli_epi64(R03, DIGIT_SIZE);
      R03 = _mm512_and_epi64(R03, MASK);
      _mm512_store_si512(pR + 8 * 3, R03);

      R04 = _mm512_add_epi64(R04, T);
      T = _mm512_srli_epi64(R04, DIGIT_SIZE);
      R04 = _mm512_and_epi64(R04, MASK);
      _mm512_store_si512(pR + 8 * 4, R04);

      R05 = _mm512_add_epi64(R05, T);
      T = _mm512_srli_epi64(R05, DIGIT_SIZE);
      R05 = _mm512_and_epi64(R05, MASK);
      _mm512_store_si512(pR + 8 * 5, R05);

      R06 = _mm512_add_epi64(R06, T);
      T = _mm512_srli_epi64(R06, DIGIT_SIZE);
      R06 = _mm512_and_epi64(R06, MASK);
      _mm512_store_si512(pR + 8 * 6, R06);

      R07 = _mm512_add_epi64(R07, T);
      T = _mm512_srli_epi64(R07, DIGIT_SIZE);
      R07 = _mm512_and_epi64(R07, MASK);
      _mm512_store_si512(pR + 8 * 7, R07);

      R08 = _mm512_add_epi64(R08, T);
      T = _mm512_srli_epi64(R08, DIGIT_SIZE);
      R08 = _mm512_and_epi64(R08, MASK);
      _mm512_store_si512(pR + 8 * 8, R08);

      R09 = _mm512_add_epi64(R09, T);
      T = _mm512_srli_epi64(R09, DIGIT_SIZE);
      R09 = _mm512_and_epi64(R09, MASK);
      _mm512_store_si512(pR + 8 * 9, R09);

      R10 = _mm512_add_epi64(R10, T);
      T = _mm512_srli_epi64(R10, DIGIT_SIZE);
      R10 = _mm512_and_epi64(R10, MASK);
      _mm512_store_si512(pR + 8 * 10, R10);

      R11 = _mm512_add_epi64(R11, T);
      T = _mm512_srli_epi64(R11, DIGIT_SIZE);
      R11 = _mm512_and_epi64(R11, MASK);
      _mm512_store_si512(pR + 8 * 11, R11);

      R12 = _mm512_add_epi64(R12, T);
      T = _mm512_srli_epi64(R12, DIGIT_SIZE);
      R12 = _mm512_and_epi64(R12, MASK);
      _mm512_store_si512(pR + 8 * 12, R12);

      R13 = _mm512_add_epi64(R13, T);
      T = _mm512_srli_epi64(R13, DIGIT_SIZE);
      R13 = _mm512_and_epi64(R13, MASK);
      _mm512_store_si512(pR + 8 * 13, R13);

      R14 = _mm512_add_epi64(R14, T);
      T = _mm512_srli_epi64(R14, DIGIT_SIZE);
      R14 = _mm512_and_epi64(R14, MASK);
      _mm512_store_si512(pR + 8 * 14, R14);

      R15 = _mm512_add_epi64(R15, T);
      T = _mm512_srli_epi64(R15, DIGIT_SIZE);
      R15 = _mm512_and_epi64(R15, MASK);
      _mm512_store_si512(pR + 8 * 15, R15);

      R16 = _mm512_add_epi64(R16, T);
      T = _mm512_srli_epi64(R16, DIGIT_SIZE);
      R16 = _mm512_and_epi64(R16, MASK);
      _mm512_store_si512(pR + 8 * 16, R16);

      R17 = _mm512_add_epi64(R17, T);
      T = _mm512_srli_epi64(R17, DIGIT_SIZE);
      R17 = _mm512_and_epi64(R17, MASK);
      _mm512_store_si512(pR + 8 * 17, R17);

      R18 = _mm512_add_epi64(R18, T);
      T = _mm512_srli_epi64(R18, DIGIT_SIZE);
      R18 = _mm512_and_epi64(R18, MASK);
      _mm512_store_si512(pR + 8 * 18, R18);

      R19 = _mm512_add_epi64(R19, T);
      T = _mm512_srli_epi64(R19, DIGIT_SIZE);
      R19 = _mm512_and_epi64(R19, MASK);
      _mm512_store_si512(pR + 8 * 19, R19);

      R20 = _mm512_add_epi64(R20, T);
      T = _mm512_srli_epi64(R20, DIGIT_SIZE);
      R20 = _mm512_and_epi64(R20, MASK);
      _mm512_store_si512(pR + 8 * 20, R20);

      R21 = _mm512_add_epi64(R21, T);
      T = _mm512_srli_epi64(R21, DIGIT_SIZE);
      R21 = _mm512_and_epi64(R21, MASK);
      _mm512_store_si512(pR + 8 * 21, R21);

      R22 = _mm512_add_epi64(R22, T);
      T = _mm512_srli_epi64(R22, DIGIT_SIZE);
      R22 = _mm512_and_epi64(R22, MASK);
      _mm512_store_si512(pR + 8 * 22, R22);

      R23 = _mm512_add_epi64(R23, T);
      T = _mm512_srli_epi64(R23, DIGIT_SIZE);
      R23 = _mm512_and_epi64(R23, MASK);
      _mm512_store_si512(pR + 8 * 23, R23);

      R24 = _mm512_add_epi64(R24, T);
      T = _mm512_srli_epi64(R24, DIGIT_SIZE);
      R24 = _mm512_and_epi64(R24, MASK);
      _mm512_store_si512(pR + 8 * 24, R24);

      R25 = _mm512_add_epi64(R25, T);
      T = _mm512_srli_epi64(R25, DIGIT_SIZE);
      R25 = _mm512_and_epi64(R25, MASK);
      _mm512_store_si512(pR + 8 * 25, R25);

      R26 = _mm512_add_epi64(R26, T);
      T = _mm512_srli_epi64(R26, DIGIT_SIZE);
      R26 = _mm512_and_epi64(R26, MASK);
      _mm512_store_si512(pR + 8 * 26, R26);

      R27 = _mm512_add_epi64(R27, T);
      T = _mm512_srli_epi64(R27, DIGIT_SIZE);
      R27 = _mm512_and_epi64(R27, MASK);
      _mm512_store_si512(pR + 8 * 27, R27);

      R28 = _mm512_add_epi64(R28, T);
      T = _mm512_srli_epi64(R28, DIGIT_SIZE);
      R28 = _mm512_and_epi64(R28, MASK);
      _mm512_store_si512(pR + 8 * 28, R28);

      R29 = _mm512_add_epi64(R29, T);
      T = _mm512_srli_epi64(R29, DIGIT_SIZE);
      R29 = _mm512_and_epi64(R29, MASK);
      _mm512_store_si512(pR + 8 * 29, R29);

      R30 = _mm512_add_epi64(R30, T);
      T = _mm512_srli_epi64(R30, DIGIT_SIZE);
      R30 = _mm512_and_epi64(R30, MASK);
      _mm512_store_si512(pR + 8 * 30, R30);

      R31 = _mm512_add_epi64(R31, T);
      T = _mm512_srli_epi64(R31, DIGIT_SIZE);
      R31 = _mm512_and_epi64(R31, MASK);
      _mm512_store_si512(pR + 8 * 31, R31);

      R32 = _mm512_add_epi64(R32, T);
      T = _mm512_srli_epi64(R32, DIGIT_SIZE);
      R32 = _mm512_and_epi64(R32, MASK);
      _mm512_store_si512(pR + 8 * 32, R32);

      R33 = _mm512_add_epi64(R33, T);
      T = _mm512_srli_epi64(R33, DIGIT_SIZE);
      R33 = _mm512_and_epi64(R33, MASK);
      _mm512_store_si512(pR + 8 * 33, R33);

      R34 = _mm512_add_epi64(R34, T);
      T = _mm512_srli_epi64(R34, DIGIT_SIZE);
      R34 = _mm512_and_epi64(R34, MASK);
      _mm512_store_si512(pR + 8 * 34, R34);

      R35 = _mm512_add_epi64(R35, T);
      T = _mm512_srli_epi64(R35, DIGIT_SIZE);
      R35 = _mm512_and_epi64(R35, MASK);
      _mm512_store_si512(pR + 8 * 35, R35);

      R36 = _mm512_add_epi64(R36, T);
      T = _mm512_srli_epi64(R36, DIGIT_SIZE);
      R36 = _mm512_and_epi64(R36, MASK);
      _mm512_store_si512(pR + 8 * 36, R36);

      R37 = _mm512_add_epi64(R37, T);
      T = _mm512_srli_epi64(R37, DIGIT_SIZE);
      R37 = _mm512_and_epi64(R37, MASK);
      _mm512_store_si512(pR + 8 * 37, R37);

      R38 = _mm512_add_epi64(R38, T);
      T = _mm512_srli_epi64(R38, DIGIT_SIZE);
      R38 = _mm512_and_epi64(R38, MASK);
      _mm512_store_si512(pR + 8 * 38, R38);

      R39 = _mm512_add_epi64(R39, T);
      T = _mm512_srli_epi64(R39, DIGIT_SIZE);
      R39 = _mm512_and_epi64(R39, MASK);
      _mm512_store_si512(pR + 8 * 39, R39);
   }
}
//////////////////////////////////////////////////////////////////////

/*
// out[] = inp[] <<nbit
//
// nbits < DIGIT_SIZE
// out occupied enouhg mem (at least ns+1)
*/
static void lshift52x_mb8(int64u pOut[][8], int64u pInp[][8], int ns, __m512i sbiL)
{
   __m512i sbiR = _mm512_sub_epi64(_mm512_set1_epi64(DIGIT_SIZE), sbiL);
   __m512i digMask = _mm512_set1_epi64(DIGIT_MASK);

   __m512i shiftedR = _mm512_setzero_si512();
   int n;
   for(n=0; n<ns; n++) {
      __m512i inp = _mm512_load_si512(pInp[n]);
      __m512i out = _mm512_and_si512(_mm512_or_si512(shiftedR, _mm512_sllv_epi64(inp,sbiL)), digMask);
      _mm512_store_si512(pOut[n], out);
      shiftedR = _mm512_srlv_epi64(inp, sbiR);
   }
}

/* out[] = inp[] >>nbit */
static void rshift52x_mb8(int64u pOut[][8], int64u pInp[][8], int ns, __m512i sbiR)
{
   __m512i sbiL = _mm512_sub_epi64(_mm512_set1_epi64(DIGIT_SIZE), sbiR);
   __m512i digMask = _mm512_set1_epi64(DIGIT_MASK);

   __m512i shiftedL = _mm512_setzero_si512();
   int n;
   for(n=ns; n>0; n--) {
      __m512i inp = _mm512_load_si512(pInp[n-1]);
      __m512i out = _mm512_and_si512(_mm512_or_si512(shiftedL, _mm512_srlv_epi64(inp,sbiR)), digMask);
      _mm512_store_si512(pOut[n-1], out);
      shiftedL = _mm512_sllv_epi64(inp, sbiL);
   }
}

/*
// believe the following works:
//
// right = {r1:r0} - high and low
// left  = {l1:l0} - high and low
//
// if(r1!=l1)
//    return r1>l1;
// else
//    return r0>l0;
*/
static __mmask8 left_gt_right_mb8(__m512i left_hi, __m512i left_lo, __m512i right_hi, __m512i right_lo)
{
   __mmask8 k0 = _mm512_cmpneq_epi64_mask(left_hi, right_hi);
   __mmask8 k1 =  ( k0 & _mm512_cmpgt_epu64_mask(left_hi, right_hi))
                | (~k0 & _mm512_cmpgt_epu64_mask(left_lo, right_lo));
   return k1;
}

__m512i tz;

/* sub multiplied_by_digit
// maskk8 reports whether addition performed inside
*/
static __mmask8 ifma_sub_muldig52x_mb8(__m512i* pRes, const __m512i* pM, int nsM, __m512i dig)
{
   __m512i MASK = _mm512_set1_epi64(DIGIT_MASK);

   __m512i prodLO = _mm512_setzero_si512();
   __m512i prodHI = _mm512_setzero_si512();
   __m512i cf = _mm512_setzero_si512();
   __m512i T;
   __mmask8 k1;

   int n;

   /* multiply dig*pM[] and sub from pRes[] */
   for(n=0; n<nsM; n++) {
      prodLO = _mm512_madd52lo_epu64(prodHI, dig, pM[n]);
      // Nasty performance hack for 2 FMA SKUs
      prodLO = _mm512_add_epi64(prodLO, tz);
      prodHI = _mm512_madd52hi_epu64(_mm512_setzero_si512(), dig, pM[n]);

      T = _mm512_sub_epi64(pRes[n], prodLO);
      T = _mm512_add_epi64(T, cf);

      cf = _mm512_srai_epi64(T, DIGIT_SIZE);
      T = _mm512_and_epi64(T, MASK);

      _mm512_storeu_si512(pRes+n, T);
   }

   T = _mm512_sub_epi64(pRes[n], prodHI);
   T = _mm512_add_epi64(T, cf);
   cf = _mm512_srai_epi64(T, DIGIT_SIZE);
   T = _mm512_and_epi64(T, MASK);
   _mm512_storeu_si512(pRes+n, T);

   /* set mask if borrow */
   k1 = _mm512_cmpneq_epu64_mask(cf, _mm512_setzero_si512());

   /* masked add pM[] */
   cf = _mm512_setzero_si512();
   for(n=0; n<nsM; n++) {
      T = _mm512_mask_add_epi64(pRes[n], k1, pRes[n], pM[n]);
      T = _mm512_add_epi64(T, cf);

      cf = _mm512_srli_epi64(T, DIGIT_SIZE);
      T = _mm512_and_epi64(T, MASK);

      _mm512_storeu_si512(pRes+n, T);
   }

   T = _mm512_add_epi64(cf, pRes[n]);
   _mm512_storeu_si512(pRes+n, T);

   return k1;
}

//EXTERN_C void div128_64_asm_mb8(int64u q[8], int64u r[8], const int64u aH[8], const int64u aL[8], const int64u d[8]);
//from Cristina
EXTERN_C __m512i __div_104_by_52(__m512i Ah, __m512i Al, __m512i B, __m512i* prem);

/* compute estimation of quotient digit (exactq): q-1 <= exactq <= q */
static __m512i estimateq_mb8(const __m512i* ptopX, const __m512i* ptopY)
{
   __m512i base52 =  _mm512_set1_epi64(DIGIT_BASE);
   __m512i one    =  _mm512_set1_epi64(1);

   __m512i y0 = _mm512_load_si512(ptopY);    /* high divisor's digit (B=base52) */
   __m512i y1 = _mm512_load_si512(ptopY-1);

   //__m512i x0 = _mm512_load_si512(ptopX);   /* 3 high divident's digits (B=base52) */
   //__m512i x1 = _mm512_load_si512(ptopX-1);
   __m512i x2 = _mm512_load_si512(ptopX-2);
   __m512i quo, rem;

   __m512i left_lo = _mm512_setzero_si512();
   __m512i left_hi = _mm512_setzero_si512();

   __mmask8 k1, k0;

#if 0 // replaced by Cristina's code
   /* get estimation for (qhat,rhat) = (x0*B + x1)/y0 */
   div128_64_asm_mb8((int64u*)&quo, (int64u*)&rem, (int64u*)(ptopX), (int64u*)(ptopX-1), (int64u*)(ptopY));
   // Avoid store-to-load forwarding block
   quo = _mm512_i32gather_epi64(_mm256_setr_epi32(0,1,2,3,4,5,6,7), &quo, 8);
   rem = _mm512_i32gather_epi64(_mm256_setr_epi32(0,1,2,3,4,5,6,7), &rem, 8);
#endif
   quo = __div_104_by_52(ptopX[0], ptopX[-1], ptopY[0], &rem);

   /*
      compare {y0:y1}*quo and {x0:x1:x2}
      left  = quo*y1
      right = rem*B + x2
   */
   left_lo = _mm512_madd52lo_epu64(left_lo, y1, quo); // left = y1*quo
   left_hi = _mm512_madd52hi_epu64(left_hi, y1, quo);

   // right = rem*B + x2 => pair of digits {r:x2}

   // k1 = left>right
   k1 = left_gt_right_mb8(left_hi, left_lo, rem, x2);

   // if(left>right)
   quo = _mm512_mask_sub_epi64(quo, k1, quo, one);             // quo -= 1
   rem = _mm512_mask_add_epi64(rem, k1, rem,  y0);             // rem += y0 , note "right" increased at the same time

   k0 = _mm512_mask_cmpgt_epu64_mask(k1, y1, left_lo);         // k0 = left_lo < y1
   left_lo = _mm512_mask_sub_epi64(left_lo, k1, left_lo, y1);  // left -= y1
   left_hi = _mm512_mask_sub_epi64(left_hi, (k1&k0), left_hi, one);

   // k0 = rem < B
   k0 = _mm512_cmplt_epu64_mask(rem, base52);
   // k1 = left>right
   k1 = k0 & left_gt_right_mb8(left_hi, left_lo, rem, x2);

   quo = _mm512_mask_sub_epi64(quo, k1, quo, one);             // quo -= 1

   return quo;
}

/* x = x % m */
void ifma_mreduce52x_mb8(int64u pX[][8], int nsX, int64u pM[][8], int nsM)
{
   /* usually divider have to be normilized */
   __m512i* pMtop = (__m512i*)(pM[nsM-1]);     /* top of M */
   __m512i normBits =  _mm512_sub_epi64(
                           _mm512_lzcnt_epi64(pMtop[0]),
                           _mm512_set1_epi64(64-DIGIT_SIZE)
                        );
   tz = _mm512_setzero_si512();
   /* normalize both divisor and shift dividentr */
   lshift52x_mb8(pM, pM, nsM, normBits);
   /* expand and shift X */
   _mm512_store_si512(pX[nsX], _mm512_setzero_si512());
   lshift52x_mb8(pX, pX, nsX+1, normBits);

   // division
   {
      __m512i* pXtop = (__m512i*)(pX[nsX]);       /* top of X (zero value) -- &X[nsX]   */
      __m512i* pXbot = (__m512i*)(pX[nsX-nsM]);   /* bot of X              -- &X[nsQ-1] */

      while(pXbot>=(__m512i*)pX) {
         /* compute estimation of quotient digit q */
         __m512i q = estimateq_mb8(pXtop, pMtop);

         /* multiply and sub */
         ifma_sub_muldig52x_mb8(pXbot, (__m512i*)pM, nsM, q);

         pXtop--;
         pXbot--;
      }
   }

   rshift52x_mb8(pX, pX, nsM, normBits);
   rshift52x_mb8(pM, pM, nsM, normBits);
}

/* bitsize of 2^64 representation => bitsize of 2^52 representation */
#define BASE52_BITSIZE(b64bitsize)     ((b64bitsize) + ((DIGIT_SIZE - ((b64bitsize) % DIGIT_SIZE)) % DIGIT_SIZE))

/* rr = 2^(2*ifmaBitLen) mod m */
void ifma_montRR52x_mb8 (int64u pRR[][8], int64u pM[][8], int convBitLen)
{
   #define MAX_IFMA_MODULUS_BITLEN  BASE52_BITSIZE(RSA_4K)

   /* buffer to hold 2^(2*MAX_IFMA_MODULUS_BITLEN) */
   __ALIGN64 int64u pwr2_mb8[(NUMBER_OF_DIGITS(2 * MAX_IFMA_MODULUS_BITLEN + 1, DIGIT_SIZE)) + 1][8]; /* +1 is necessary extension for ifma_mreduce52x_mb8() purpose */

   int ifmaBitLen = BASE52_BITSIZE(convBitLen);
   int ifmaLen = NUMBER_OF_DIGITS(ifmaBitLen, DIGIT_SIZE);

   int pwr = 2*ifmaBitLen;
   int s = pwr - ((pwr/DIGIT_SIZE) * DIGIT_SIZE);
   int pwrLen = NUMBER_OF_DIGITS(pwr + 1, DIGIT_SIZE);

   /* set 2^(ifmaBitLen*2) */
   zero_mb8(pwr2_mb8, pwrLen);
   _mm512_store_si512(pwr2_mb8[pwrLen-1], _mm512_slli_epi64(_mm512_set1_epi64(1), s) );

   /* 2^(ifmaBitLen*2) mod M */
   ifma_mreduce52x_mb8(pwr2_mb8, pwrLen, pM, ifmaLen);

   /* copy result */
   for(s=0; s<ifmaLen; s++)
      _mm512_store_si512(pRR[s], _mm512_load_si512(pwr2_mb8[s]) );

   #undef MAX_IFMA_MODULUS_BITLEN
}
