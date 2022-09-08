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

#include <assert.h>

#if defined(_MSC_VER) && (_MSC_VER < 1920)
  // Disable optimization for VS2017 due to AVX512 masking bug
  #define DISABLE_OPTIMIZATION __pragma(optimize( "", off ))
#else
  #define DISABLE_OPTIMIZATION
#endif

#define PROC_LEN (52)

#define BYTES_REV (1)
#define RADIX_CVT (2)

#define MIN(a, b) ( ((a) < (b)) ? a : b )

__INLINE __mmask8 MB_MASK(int L) {
   return (L > 0) ? (__mmask8)0xFF : (__mmask8)0;
}

__INLINE __mmask64 SB_MASK1(int L, int REV)
{
   if (L <= 0)
      return (__mmask64)0x0;
   if (L > PROC_LEN)
      L = PROC_LEN;
   if (REV)
      return (__mmask64)(0xFFFFFFFFFFFFFFFFULL << ((int)sizeof(__m512i) - L));
   return (__mmask64)(0xFFFFFFFFFFFFFFFFULL >> ((int)sizeof(__m512i) - L));
}


   #ifndef BN_OPENSSL_DISABLE
   #include <openssl/bn.h>
   #if BN_OPENSSL_PATCH
   extern BN_ULONG* bn_get_words(const BIGNUM* bn);
   #endif
   #endif /* BN_OPENSSL_DISABLE */

/*
// transpose 8 SB into MB including (reverse bytes and) radix 2^64 => 2^52 conversion
//
// covers:
//    - 8 BIGNUM     -> mb8
//    - 8 BNU        -> mb8
//    - 8 hex strings -> mb8
*/
DISABLE_OPTIMIZATION
__INLINE void transform_8sb_to_mb8(U64 out_mb8[], int bitLen, int8u *inp[8], int inpLen[8], int flag) {
   // inverse bytes (reverse=1)
   const __m512i bswap_mask = _mm512_set_epi64(
                     0x0001020304050607, 0x08090a0b0c0d0e0f,
                     0x1011121314151617, 0x18191a1b1c1d1e1f,
                     0x2021222324252627, 0x28292a2b2c2d2e2f,
                     0x3031323334353637, 0x38393a3b3c3d3e3f);
   // repeat words
   const __m512i idx16 = _mm512_set_epi64(
                     0x0019001800170016,
                     0x0016001500140013,
                     0x0013001200110010,
                     0x0010000f000e000d,
                     0x000c000b000a0009,
                     0x0009000800070006,
                     0x0006000500040003,
                     0x0003000200010000);
   // shift right
   const __m512i shiftR = _mm512_set_epi64(
      12, 8, 4, 0, 12, 8, 4, 0);
   // radix 2^52 mask of digits
   __m512i digMask = _mm512_set1_epi64(DIGIT_MASK);

   int bytesRev = flag & BYTES_REV; /* reverse flag */
   int radixCvt = flag & RADIX_CVT; /* radix (64->52) conversion assumed*/

   int inpBytes = NUMBER_OF_DIGITS(bitLen, 8); /* bytes */
   int outDigits = NUMBER_OF_DIGITS(bitLen, DIGIT_SIZE); /* digits */

   int i;
   for (i = 0; inpBytes > 0; i += PROC_LEN, inpBytes -= PROC_LEN, out_mb8 += 8) {
      int sbidx = bytesRev ? inpBytes - (int)sizeof(__m512i) : i;

      __m512i X0 = _mm512_maskz_loadu_epi8(SB_MASK1(inpLen[0] - i, bytesRev), (__m512i*)&inp[0][sbidx]);
      __m512i X1 = _mm512_maskz_loadu_epi8(SB_MASK1(inpLen[1] - i, bytesRev), (__m512i*)&inp[1][sbidx]);
      __m512i X2 = _mm512_maskz_loadu_epi8(SB_MASK1(inpLen[2] - i, bytesRev), (__m512i*)&inp[2][sbidx]);
      __m512i X3 = _mm512_maskz_loadu_epi8(SB_MASK1(inpLen[3] - i, bytesRev), (__m512i*)&inp[3][sbidx]);
      __m512i X4 = _mm512_maskz_loadu_epi8(SB_MASK1(inpLen[4] - i, bytesRev), (__m512i*)&inp[4][sbidx]);
      __m512i X5 = _mm512_maskz_loadu_epi8(SB_MASK1(inpLen[5] - i, bytesRev), (__m512i*)&inp[5][sbidx]);
      __m512i X6 = _mm512_maskz_loadu_epi8(SB_MASK1(inpLen[6] - i, bytesRev), (__m512i*)&inp[6][sbidx]);
      __m512i X7 = _mm512_maskz_loadu_epi8(SB_MASK1(inpLen[7] - i, bytesRev), (__m512i*)&inp[7][sbidx]);

      if (bytesRev) {
         X0 = _mm512_permutexvar_epi8(bswap_mask, X0);
         X1 = _mm512_permutexvar_epi8(bswap_mask, X1);
         X2 = _mm512_permutexvar_epi8(bswap_mask, X2);
         X3 = _mm512_permutexvar_epi8(bswap_mask, X3);
         X4 = _mm512_permutexvar_epi8(bswap_mask, X4);
         X5 = _mm512_permutexvar_epi8(bswap_mask, X5);
         X6 = _mm512_permutexvar_epi8(bswap_mask, X6);
         X7 = _mm512_permutexvar_epi8(bswap_mask, X7);
      }

      if (radixCvt) {
         X0 = _mm512_permutexvar_epi16(idx16, X0);
         X0 = _mm512_srlv_epi64(X0, shiftR);
         X0 = _mm512_and_si512(X0, digMask); /* probably exceeded instruction */

         X1 = _mm512_permutexvar_epi16(idx16, X1);
         X1 = _mm512_srlv_epi64(X1, shiftR);
         X1 = _mm512_and_si512(X1, digMask);

         X2 = _mm512_permutexvar_epi16(idx16, X2);
         X2 = _mm512_srlv_epi64(X2, shiftR);
         X2 = _mm512_and_si512(X2, digMask);

         X3 = _mm512_permutexvar_epi16(idx16, X3);
         X3 = _mm512_srlv_epi64(X3, shiftR);
         X3 = _mm512_and_si512(X3, digMask);

         X4 = _mm512_permutexvar_epi16(idx16, X4);
         X4 = _mm512_srlv_epi64(X4, shiftR);
         X4 = _mm512_and_si512(X4, digMask);

         X5 = _mm512_permutexvar_epi16(idx16, X5);
         X5 = _mm512_srlv_epi64(X5, shiftR);
         X5 = _mm512_and_si512(X5, digMask);

         X6 = _mm512_permutexvar_epi16(idx16, X6);
         X6 = _mm512_srlv_epi64(X6, shiftR);
         X6 = _mm512_and_si512(X6, digMask);

         X7 = _mm512_permutexvar_epi16(idx16, X7);
         X7 = _mm512_srlv_epi64(X7, shiftR);
         X7 = _mm512_and_si512(X7, digMask);
      }

      // transpose 8 digits at a time
      TRANSPOSE_8xI64x8(X0, X1, X2, X3, X4, X5, X6, X7);

      // store transposed digits
      _mm512_mask_storeu_epi64(&out_mb8[0], MB_MASK(outDigits--), X0);
      _mm512_mask_storeu_epi64(&out_mb8[1], MB_MASK(outDigits--), X1);
      _mm512_mask_storeu_epi64(&out_mb8[2], MB_MASK(outDigits--), X2);
      _mm512_mask_storeu_epi64(&out_mb8[3], MB_MASK(outDigits--), X3);
      _mm512_mask_storeu_epi64(&out_mb8[4], MB_MASK(outDigits--), X4);
      _mm512_mask_storeu_epi64(&out_mb8[5], MB_MASK(outDigits--), X5);
      _mm512_mask_storeu_epi64(&out_mb8[6], MB_MASK(outDigits--), X6);
      _mm512_mask_storeu_epi64(&out_mb8[7], MB_MASK(outDigits--), X7);
   }
}

#ifndef BN_OPENSSL_DISABLE
// Convert BIGNUM into MB8(Radix=2^52) format
// Returns bitmask of succesfully converted values
// Accepts NULLs as BIGNUM inputs
//    Null or wrong length
int8u ifma_BN_to_mb8(int64u out_mb8[][8], const BIGNUM* const bn[8], int bitLen)
{
   // check input input length
   assert((0<bitLen) && (bitLen<=IFMA_MAX_BITSIZE));

   int byteLen = NUMBER_OF_DIGITS(bitLen, 8);
   int byteLens[8];

   int8u *d[8];
   #ifndef BN_OPENSSL_PATCH
   __ALIGN64 int8u buffer[8][NUMBER_OF_DIGITS(IFMA_MAX_BITSIZE,8)];
   #endif

   int i;
   for (i = 0; i < 8; ++i) {
      if(NULL != bn[i]) {
         byteLens[i] = BN_num_bytes(bn[i]);
         assert(byteLens[i] <= byteLen);

         #ifndef BN_OPENSSL_PATCH
         d[i] = buffer[i];
         BN_bn2lebinpad(bn[i], d[i], byteLen);
         #else
         d[i] = (int8u*)bn_get_words(bn[i]);
         #endif
      }
      else {
         // no input in that bucket
         d[i] = NULL;
         byteLens[i] = 0;
      }
   }

   transform_8sb_to_mb8((U64*)out_mb8, bitLen, (int8u**)d, byteLens, RADIX_CVT);

   return _mm512_cmpneq_epi64_mask(_mm512_loadu_si512((__m512i*)&bn), _mm512_setzero_si512());
}
#endif /* BN_OPENSSL_DISABLE */

// Simlilar to ifma_BN_to_mb8(), but converts array of int64u instead of BIGNUM
// Assumed that each converted values has bitLen length
int8u ifma_BNU_to_mb8(int64u out_mb8[][8], const int64u* const bn[8], int bitLen)
{
   // Check input parameters
   assert(bitLen > 0);

   int byteLens[8];
   int byteLen = NUMBER_OF_DIGITS(bitLen, 8);
   int i;
   for (i = 0; i < 8; ++i)
       byteLens[i] = (NULL != bn[i]) ? byteLen : 0;

   transform_8sb_to_mb8((U64*)out_mb8, bitLen, (int8u**)bn, byteLens, RADIX_CVT);

   return _mm512_cmpneq_epi64_mask(_mm512_loadu_si512((__m512i*)bn), _mm512_setzero_si512());
}

int8u ifma_HexStr8_to_mb8(int64u out_mb8[][8], const int8u* const pStr[8], int bitLen)
{
    // check input parameters
    assert(bitLen > 0);

    int byteLens[8];
    int byteLen = NUMBER_OF_DIGITS(bitLen, 8);
    int i;
    for (i = 0; i < 8; i++)
        byteLens[i] = (NULL != pStr[i]) ? byteLen : 0;

   transform_8sb_to_mb8((U64*)out_mb8, bitLen, (int8u**)pStr, byteLens, RADIX_CVT | BYTES_REV);

   return _mm512_cmpneq_epi64_mask(_mm512_loadu_si512((__m512i*)pStr), _mm512_setzero_si512());
}
////////////////////////////////////////////////////////////////////////////////////////////////////

/*
// transpose MB into 8 SB including (reverse bytes and) radix 2^52 => 2^64 conversion
//
// covers:
//    - mb8 -> 8 BNU
//    - mb8 -> 8 hex strings
*/
DISABLE_OPTIMIZATION
__INLINE void transform_mb8_to_8sb(int8u* out[8], int outLen[8], const U64 inp_mb8[], int bitLen, int flag)
{
   // inverse bytes (reverse=1)
   const __m512i bswap_mask = _mm512_set_epi64(
                     0x0001020304050607, 0x08090a0b0c0d0e0f,
                     0x1011121314151617, 0x18191a1b1c1d1e1f,
                     0x2021222324252627, 0x28292a2b2c2d2e2f,
                     0x3031323334353637, 0x38393a3b3c3d3e3f);

   const __m512i shiftL = _mm512_set_epi64(4, 0, 4, 0, 4, 0, 4, 0);

   const __m512i permutation1 = _mm512_set_epi64(0x3f3f3f3f3f3f3f3f, // {63,63,63,63,63,63,63,63}
                                                0x3f3f3f3f3e3d3c3b,  // {63,63,63,63,62,61,60,59}
                                                0x3737363534333231,  // {55,55,54,53,52,51,50,49}
                                                0x302e2d2c2b2a2928,  // {48,46,45,44,43,42,41,40}
                                                0x1f1f1f1f1f1f1e1d,  // {31,31,31,31,31,31,30,29}
                                                0x1717171716151413,  // {23,23,23,23,22,21,20,19}
                                                0x0f0f0f0e0d0c0b0a,  // {15,15,15,14,13,12,11,10}
                                                0x0706050403020100); // { 7, 6, 5, 4, 3, 2, 1, 0}

   const __m512i permutation2 = _mm512_set_epi64(0x3f3f3f3f3f3f3f3f, // {63,63,63,63,63,63,63,63}
                                                0x3f3f3f3f3f3f3f3f,  // {63,63,63,63,63,63,63,63}
                                                0x3a39383737373737,  // {58,57,56,55,55,55,55,55}
                                                0x2727272727272726,  // {39,39,39,39,39,39,39,38}
                                                0x2524232221201f1f,  // {37,36,35,34,33,32,31,31}
                                                0x1c1b1a1918171717,  // {28,27,26,25,24,23,23,23}
                                                0x1211100f0f0f0f0f,  // {18,17,16,15,15,15,15,15}
                                                0x0908070707070707); // { 9, 8, 7, 7, 7, 7, 7, 7}
   int bytesRev = flag & BYTES_REV; /* reverse flag */
   int radixCvt = flag & RADIX_CVT; /* radix (52->64) conversion assumed */

   int inpDigits = NUMBER_OF_DIGITS(bitLen, DIGIT_SIZE); /* digits */
   int outBytes = NUMBER_OF_DIGITS(bitLen, 8); /* bytes */

   int i;
   for (i = 0; outBytes > 0; i += PROC_LEN, outBytes -= PROC_LEN, inp_mb8 += 8) {
      int sbidx = bytesRev ? outBytes - (int)sizeof(__m512i) : i;

      __m512i X0 = _mm512_maskz_loadu_epi64(MB_MASK(inpDigits--), &inp_mb8[0]);
      __m512i X1 = _mm512_maskz_loadu_epi64(MB_MASK(inpDigits--), &inp_mb8[1]);
      __m512i X2 = _mm512_maskz_loadu_epi64(MB_MASK(inpDigits--), &inp_mb8[2]);
      __m512i X3 = _mm512_maskz_loadu_epi64(MB_MASK(inpDigits--), &inp_mb8[3]);
      __m512i X4 = _mm512_maskz_loadu_epi64(MB_MASK(inpDigits--), &inp_mb8[4]);
      __m512i X5 = _mm512_maskz_loadu_epi64(MB_MASK(inpDigits--), &inp_mb8[5]);
      __m512i X6 = _mm512_maskz_loadu_epi64(MB_MASK(inpDigits--), &inp_mb8[6]);
      __m512i X7 = _mm512_maskz_loadu_epi64(MB_MASK(inpDigits--), &inp_mb8[7]);

      // transpose 8 digits at a time
      TRANSPOSE_8xI64x8(X0, X1, X2, X3, X4, X5, X6, X7);

      if (radixCvt) {
         __m512i T;
         X0 = _mm512_sllv_epi64(X0, shiftL);
         T = _mm512_permutexvar_epi8(permutation1, X0);
         X0 = _mm512_permutexvar_epi8(permutation2, X0);
         X0 = _mm512_or_si512(X0, T);

         X1 = _mm512_sllv_epi64(X1, shiftL);
         T = _mm512_permutexvar_epi8(permutation1, X1);
         X1 = _mm512_permutexvar_epi8(permutation2, X1);
         X1 = _mm512_or_si512(X1, T);

         X2 = _mm512_sllv_epi64(X2, shiftL);
         T = _mm512_permutexvar_epi8(permutation1, X2);
         X2 = _mm512_permutexvar_epi8(permutation2, X2);
         X2 = _mm512_or_si512(X2, T);

         X3 = _mm512_sllv_epi64(X3, shiftL);
         T = _mm512_permutexvar_epi8(permutation1, X3);
         X3 = _mm512_permutexvar_epi8(permutation2, X3);
         X3 = _mm512_or_si512(X3, T);

         X4 = _mm512_sllv_epi64(X4, shiftL);
         T = _mm512_permutexvar_epi8(permutation1, X4);
         X4 = _mm512_permutexvar_epi8(permutation2, X4);
         X4 = _mm512_or_si512(X4, T);

         X5 = _mm512_sllv_epi64(X5, shiftL);
         T = _mm512_permutexvar_epi8(permutation1, X5);
         X5 = _mm512_permutexvar_epi8(permutation2, X5);
         X5 = _mm512_or_si512(X5, T);

         X6 = _mm512_sllv_epi64(X6, shiftL);
         T = _mm512_permutexvar_epi8(permutation1, X6);
         X6 = _mm512_permutexvar_epi8(permutation2, X6);
         X6 = _mm512_or_si512(X6, T);

         X7 = _mm512_sllv_epi64(X7, shiftL);
         T = _mm512_permutexvar_epi8(permutation1, X7);
         X7 = _mm512_permutexvar_epi8(permutation2, X7);
         X7 = _mm512_or_si512(X7, T);
      }

      if (bytesRev) {
         X0 = _mm512_permutexvar_epi8(bswap_mask, X0);
         X1 = _mm512_permutexvar_epi8(bswap_mask, X1);
         X2 = _mm512_permutexvar_epi8(bswap_mask, X2);
         X3 = _mm512_permutexvar_epi8(bswap_mask, X3);
         X4 = _mm512_permutexvar_epi8(bswap_mask, X4);
         X5 = _mm512_permutexvar_epi8(bswap_mask, X5);
         X6 = _mm512_permutexvar_epi8(bswap_mask, X6);
         X7 = _mm512_permutexvar_epi8(bswap_mask, X7);
      }

      // store transposed digits
      _mm512_mask_storeu_epi8(out[0] + sbidx, SB_MASK1(outLen[0] - i, bytesRev), X0);
      _mm512_mask_storeu_epi8(out[1] + sbidx, SB_MASK1(outLen[1] - i, bytesRev), X1);
      _mm512_mask_storeu_epi8(out[2] + sbidx, SB_MASK1(outLen[2] - i, bytesRev), X2);
      _mm512_mask_storeu_epi8(out[3] + sbidx, SB_MASK1(outLen[3] - i, bytesRev), X3);
      _mm512_mask_storeu_epi8(out[4] + sbidx, SB_MASK1(outLen[4] - i, bytesRev), X4);
      _mm512_mask_storeu_epi8(out[5] + sbidx, SB_MASK1(outLen[5] - i, bytesRev), X5);
      _mm512_mask_storeu_epi8(out[6] + sbidx, SB_MASK1(outLen[6] - i, bytesRev), X6);
      _mm512_mask_storeu_epi8(out[7] + sbidx, SB_MASK1(outLen[7] - i, bytesRev), X7);
   }

}

int8u ifma_mb8_to_BNU(int64u* const out_bn[8], const int64u inp_mb8[][8], const int bitLen)
{
    // Check input parameters
    assert(bitLen > 0);

    int bnu_bitlen = NUMBER_OF_DIGITS(bitLen, 64) * 64; // gres: output length is multiple 64
    int byteLens[8];
    int i;
    for (i = 0; i < 8; ++i)
        //gres: byteLens[i] = (NULL != out_bn[i]) ? NUMBER_OF_DIGITS(bitLen, 8) : 0;
         byteLens[i] = (NULL != out_bn[i]) ? NUMBER_OF_DIGITS(bnu_bitlen, 8) : 0;

    transform_mb8_to_8sb((int8u**)out_bn, byteLens, (U64*)inp_mb8, bitLen, RADIX_CVT);

   return _mm512_cmpneq_epi64_mask(_mm512_loadu_si512((__m512i*)out_bn), _mm512_setzero_si512());
}

int8u ifma_mb8_to_HexStr8(int8u* const pStr[8], const int64u inp_mb8[][8], int bitLen)
{
    // check input parameters
    assert(bitLen > 0);

    int byteLens[8];
    int byteLen = NUMBER_OF_DIGITS(bitLen, 8);
    int i;
    for (i = 0; i < 8; i++)
        byteLens[i] = (NULL != pStr[i]) ? byteLen : 0;

    transform_mb8_to_8sb((int8u**)pStr, byteLens, (U64*)inp_mb8, bitLen, RADIX_CVT | BYTES_REV);

    return _mm512_cmpneq_epi64_mask(_mm512_loadu_si512((__m512i*)pStr), _mm512_setzero_si512());
}
////////////////////////////////////////////////////////////////////////////////////////////////////

/*
// transpose 8 SB into MB without radix conversion
//
// covers:
//    - mb8 -> 8 BNU
//    - mb8 -> 8 hex strings
*/
DISABLE_OPTIMIZATION
int8u ifma_BNU_transpose_copy(int64u out_mb8[][8], const int64u* const bn[8], int bitLen)
{
    // Check input parameters
    assert(bitLen > 0);

    __mmask8 kbn[8];
    int i;
    for (i = 0; i < 8; ++i)
        kbn[i] = (NULL == bn[i]) ? (__mmask8)0 : (__mmask8)0xFF;

    int len = NUMBER_OF_DIGITS(bitLen, 64);
    int n;
    for (n = 0; len > 0; n += 8, out_mb8 += 8) {
        __mmask8 kread = (len >= 8) ? 0xFF : (__mmask8)((1 << len) - 1);

        __m512i X0 = _mm512_maskz_loadu_epi64(kread & kbn[0], bn[0] + n);
        __m512i X1 = _mm512_maskz_loadu_epi64(kread & kbn[1], bn[1] + n);
        __m512i X2 = _mm512_maskz_loadu_epi64(kread & kbn[2], bn[2] + n);
        __m512i X3 = _mm512_maskz_loadu_epi64(kread & kbn[3], bn[3] + n);
        __m512i X4 = _mm512_maskz_loadu_epi64(kread & kbn[4], bn[4] + n);
        __m512i X5 = _mm512_maskz_loadu_epi64(kread & kbn[5], bn[5] + n);
        __m512i X6 = _mm512_maskz_loadu_epi64(kread & kbn[6], bn[6] + n);
        __m512i X7 = _mm512_maskz_loadu_epi64(kread & kbn[7], bn[7] + n);

        TRANSPOSE_8xI64x8(X0, X1, X2, X3, X4, X5, X6, X7);

        _mm512_mask_storeu_epi64(&out_mb8[0], MB_MASK(len--), X0);
        _mm512_mask_storeu_epi64(&out_mb8[1], MB_MASK(len--), X1);
        _mm512_mask_storeu_epi64(&out_mb8[2], MB_MASK(len--), X2);
        _mm512_mask_storeu_epi64(&out_mb8[3], MB_MASK(len--), X3);
        _mm512_mask_storeu_epi64(&out_mb8[4], MB_MASK(len--), X4);
        _mm512_mask_storeu_epi64(&out_mb8[5], MB_MASK(len--), X5);
        _mm512_mask_storeu_epi64(&out_mb8[6], MB_MASK(len--), X6);
        _mm512_mask_storeu_epi64(&out_mb8[7], MB_MASK(len--), X7);
    }

    return _mm512_cmpneq_epi64_mask(_mm512_loadu_si512((__m512i*)bn), _mm512_setzero_si512());
}

#ifndef BN_OPENSSL_DISABLE

DISABLE_OPTIMIZATION
int8u ifma_BN_transpose_copy(int64u out_mb8[][8], const BIGNUM* const bn[8], int bitLen)
{
   // check input length
   assert((0<bitLen) && (bitLen<=IFMA_MAX_BITSIZE));

   int byteLen = NUMBER_OF_DIGITS(bitLen, 64) * 8;

   int64u *inp[8];
   #ifndef BN_OPENSSL_PATCH
   __ALIGN64 int64u buffer[8][NUMBER_OF_DIGITS(IFMA_MAX_BITSIZE,64)];
   #endif

   __mmask8 kbn[8];

   int i;
   for (i = 0; i < 8; ++i) {
      if (NULL == bn[i]) {
         kbn[i] = 0;
         inp[i] = NULL;
      }
      else {
         kbn[i] = 0xFF;

         #ifndef BN_OPENSSL_PATCH
         inp[i] = buffer[i];
         BN_bn2lebinpad(bn[i], (unsigned char *)inp[i], byteLen);
         #else
         inp[i] = (int64u*)bn_get_words(bn[i]);
         #endif
      }
   }

   int len = NUMBER_OF_DIGITS(bitLen,64);
   int n;
   for (n = 0; len > 0; n += 8, out_mb8 += 8) {
      __mmask8 k = (len >= 8) ? 0xFF : (1 << len) - 1;

      __m512i X0 = _mm512_maskz_loadu_epi64(k & kbn[0], inp[0]+n);
      __m512i X1 = _mm512_maskz_loadu_epi64(k & kbn[1], inp[1]+n);
      __m512i X2 = _mm512_maskz_loadu_epi64(k & kbn[2], inp[2]+n);
      __m512i X3 = _mm512_maskz_loadu_epi64(k & kbn[3], inp[3]+n);
      __m512i X4 = _mm512_maskz_loadu_epi64(k & kbn[4], inp[4]+n);
      __m512i X5 = _mm512_maskz_loadu_epi64(k & kbn[5], inp[5]+n);
      __m512i X6 = _mm512_maskz_loadu_epi64(k & kbn[6], inp[6]+n);
      __m512i X7 = _mm512_maskz_loadu_epi64(k & kbn[7], inp[7]+n);

      TRANSPOSE_8xI64x8(X0, X1, X2, X3, X4, X5, X6, X7);

      _mm512_mask_storeu_epi64(&out_mb8[0], MB_MASK(len--), X0);
      _mm512_mask_storeu_epi64(&out_mb8[1], MB_MASK(len--), X1);
      _mm512_mask_storeu_epi64(&out_mb8[2], MB_MASK(len--), X2);
      _mm512_mask_storeu_epi64(&out_mb8[3], MB_MASK(len--), X3);
      _mm512_mask_storeu_epi64(&out_mb8[4], MB_MASK(len--), X4);
      _mm512_mask_storeu_epi64(&out_mb8[5], MB_MASK(len--), X5);
      _mm512_mask_storeu_epi64(&out_mb8[6], MB_MASK(len--), X6);
      _mm512_mask_storeu_epi64(&out_mb8[7], MB_MASK(len--), X7);
   }

   return _mm512_cmpneq_epi64_mask(_mm512_loadu_si512((__m512i*)&bn), _mm512_setzero_si512());
}
#endif /* BN_OPENSSL_DISABLE */
