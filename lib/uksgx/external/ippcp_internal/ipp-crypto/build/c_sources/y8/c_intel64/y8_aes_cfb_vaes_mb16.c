/*******************************************************************************
* Copyright 2020-2021 Intel Corporation
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

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesm.h"
#include "pcptool.h"
#include "aes_cfb_vaes_mb.h"

#if(_IPP32E>=_IPP32E_K1)

#define AES_ENCRYPT_VAES_MB16(b0, b1, b2, b3, pRkey, num_rounds) { \
   __m512i (*tkeys)[4] =  &pRkey[num_rounds-9]; \
   b0 = _mm512_xor_si512(b0, pRkey[0][0]); \
   b1 = _mm512_xor_si512(b1, pRkey[0][1]); \
   b2 = _mm512_xor_si512(b2, pRkey[0][2]); \
   b3 = _mm512_xor_si512(b3, pRkey[0][3]); \
   switch(num_rounds) { \
   case 14: \
      b0 = _mm512_aesenc_epi128(b0, tkeys[-4][0]); \
      b1 = _mm512_aesenc_epi128(b1, tkeys[-4][1]); \
      b2 = _mm512_aesenc_epi128(b2, tkeys[-4][2]); \
      b3 = _mm512_aesenc_epi128(b3, tkeys[-4][3]); \
     \
      b0 = _mm512_aesenc_epi128(b0, tkeys[-3][0]); \
      b1 = _mm512_aesenc_epi128(b1, tkeys[-3][1]); \
      b2 = _mm512_aesenc_epi128(b2, tkeys[-3][2]); \
      b3 = _mm512_aesenc_epi128(b3, tkeys[-3][3]); \
   case 12: \
      b0 = _mm512_aesenc_epi128(b0, tkeys[-2][0]); \
      b1 = _mm512_aesenc_epi128(b1, tkeys[-2][1]); \
      b2 = _mm512_aesenc_epi128(b2, tkeys[-2][2]); \
      b3 = _mm512_aesenc_epi128(b3, tkeys[-2][3]); \
     \
      b0 = _mm512_aesenc_epi128(b0, tkeys[-1][0]); \
      b1 = _mm512_aesenc_epi128(b1, tkeys[-1][1]); \
      b2 = _mm512_aesenc_epi128(b2, tkeys[-1][2]); \
      b3 = _mm512_aesenc_epi128(b3, tkeys[-1][3]); \
   default: \
      b0 = _mm512_aesenc_epi128(b0, tkeys[0][0]); \
      b1 = _mm512_aesenc_epi128(b1, tkeys[0][1]); \
      b2 = _mm512_aesenc_epi128(b2, tkeys[0][2]); \
      b3 = _mm512_aesenc_epi128(b3, tkeys[0][3]); \
     \
      b0 = _mm512_aesenc_epi128(b0, tkeys[1][0]); \
      b1 = _mm512_aesenc_epi128(b1, tkeys[1][1]); \
      b2 = _mm512_aesenc_epi128(b2, tkeys[1][2]); \
      b3 = _mm512_aesenc_epi128(b3, tkeys[1][3]); \
     \
      b0 = _mm512_aesenc_epi128(b0, tkeys[2][0]); \
      b1 = _mm512_aesenc_epi128(b1, tkeys[2][1]); \
      b2 = _mm512_aesenc_epi128(b2, tkeys[2][2]); \
      b3 = _mm512_aesenc_epi128(b3, tkeys[2][3]); \
     \
      b0 = _mm512_aesenc_epi128(b0, tkeys[3][0]); \
      b1 = _mm512_aesenc_epi128(b1, tkeys[3][1]); \
      b2 = _mm512_aesenc_epi128(b2, tkeys[3][2]); \
      b3 = _mm512_aesenc_epi128(b3, tkeys[3][3]); \
     \
      b0 = _mm512_aesenc_epi128(b0, tkeys[4][0]); \
      b1 = _mm512_aesenc_epi128(b1, tkeys[4][1]); \
      b2 = _mm512_aesenc_epi128(b2, tkeys[4][2]); \
      b3 = _mm512_aesenc_epi128(b3, tkeys[4][3]); \
     \
      b0 = _mm512_aesenc_epi128(b0, tkeys[5][0]); \
      b1 = _mm512_aesenc_epi128(b1, tkeys[5][1]); \
      b2 = _mm512_aesenc_epi128(b2, tkeys[5][2]); \
      b3 = _mm512_aesenc_epi128(b3, tkeys[5][3]); \
     \
      b0 = _mm512_aesenc_epi128(b0, tkeys[6][0]); \
      b1 = _mm512_aesenc_epi128(b1, tkeys[6][1]); \
      b2 = _mm512_aesenc_epi128(b2, tkeys[6][2]); \
      b3 = _mm512_aesenc_epi128(b3, tkeys[6][3]); \
     \
      b0 = _mm512_aesenc_epi128(b0, tkeys[7][0]); \
      b1 = _mm512_aesenc_epi128(b1, tkeys[7][1]); \
      b2 = _mm512_aesenc_epi128(b2, tkeys[7][2]); \
      b3 = _mm512_aesenc_epi128(b3, tkeys[7][3]); \
     \
      b0 = _mm512_aesenc_epi128(b0, tkeys[8][0]); \
      b1 = _mm512_aesenc_epi128(b1, tkeys[8][1]); \
      b2 = _mm512_aesenc_epi128(b2, tkeys[8][2]); \
      b3 = _mm512_aesenc_epi128(b3, tkeys[8][3]); \
     \
      b0 = _mm512_aesenclast_epi128(b0, tkeys[9][0]); \
      b1 = _mm512_aesenclast_epi128(b1, tkeys[9][1]); \
      b2 = _mm512_aesenclast_epi128(b2, tkeys[9][2]); \
      b3 = _mm512_aesenclast_epi128(b3, tkeys[9][3]); \
   } \
}

// Disable optimization for MSVC
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
    #pragma optimize( "", off )
#endif

IPP_OWN_DEFN(void, aes_cfb16_enc_vaes_mb16, (const Ipp8u* const source_pa[16], Ipp8u* const dst_pa[16], const int arr_len[16], const int num_rounds, const Ipp32u* enc_keys[16], const Ipp8u* iv_pa[16]))
{
	int i, j, k;
	int maxLen = 0;
	int loc_len64[16];
	Ipp8u* loc_src[16];
	Ipp8u* loc_dst[16];
	__m512i iv512[4];

	__mmask8 mbMask128[16] = { 0x03, 0x0C, 0x30, 0xC0, 0x03, 0x0C, 0x30, 0xC0, 0x03, 0x0C, 0x30, 0xC0, 0x03, 0x0C, 0x30, 0xC0 };
	__mmask8 mbMask[16]    = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

	// - Local copy of length, soure and target pointers, maxLen calculation
	for (i = 0; i < 16; i++) {
		loc_src[i] = (Ipp8u*)source_pa[i];
		loc_dst[i] = (Ipp8u*)dst_pa[i];
		int len64 = arr_len[i] / (Ipp32s)sizeof(Ipp64u); // legth in 64-bit chunks
		loc_len64[i] = len64;

		// The case of the empty input buffer
		if (arr_len[i] == 0)
		{
			mbMask128[i] = 0;
			mbMask[i] = 0;
			continue;
		}

		if (len64 < 8)
			mbMask[i] = (__mmask8)(((1 << len64) - 1) & 0xFF);
		if (len64 > maxLen)
			maxLen = len64;
	}

	// Load the necessary number of 128-bit IV
	j = 0;
	for (i = 0; i < 16; i += 4) {
		iv512[j] = _mm512_setzero_si512();

		iv512[j] = _mm512_mask_expandloadu_epi64(iv512[j], mbMask128[i], iv_pa[i]);
		iv512[j] = _mm512_mask_expandloadu_epi64(iv512[j], mbMask128[i + 1], iv_pa[i + 1]);
		iv512[j] = _mm512_mask_expandloadu_epi64(iv512[j], mbMask128[i + 2], iv_pa[i + 2]);
		iv512[j] = _mm512_mask_expandloadu_epi64(iv512[j], mbMask128[i + 3], iv_pa[i + 3]);
		j += 1;
	}

	// Temporary block to left IV unchanged to use it on the next round
	__m512i chip0 = iv512[0];
	__m512i chip1 = iv512[1];
	__m512i chip2 = iv512[2];
	__m512i chip3 = iv512[3];

	// Prepare array with key schedule
	__m512i keySchedule[15][4];
	__m512i tmpKeyMb = _mm512_setzero_si512();
	for (i = 0; i <= num_rounds; i++)
	{
		k = 0;
		for (j = 0; j < 16; j += 4) {
			tmpKeyMb = _mm512_mask_expandloadu_epi64(tmpKeyMb, mbMask128[j], (const void *)(enc_keys[j] + (Ipp32u)i * sizeof(Ipp32u)));
			tmpKeyMb = _mm512_mask_expandloadu_epi64(tmpKeyMb, mbMask128[j + 1], (const void *)(enc_keys[j + 1] + (Ipp32u)i * sizeof(Ipp32u)));
			tmpKeyMb = _mm512_mask_expandloadu_epi64(tmpKeyMb, mbMask128[j + 2], (const void *)(enc_keys[j + 2] + (Ipp32u)i * sizeof(Ipp32u)));
			tmpKeyMb = _mm512_mask_expandloadu_epi64(tmpKeyMb, mbMask128[j + 3], (const void *)(enc_keys[j + 3] + (Ipp32u)i * sizeof(Ipp32u)));

			keySchedule[i][k] = _mm512_loadu_si512(&tmpKeyMb);
			k += 1;
		}
	}
	
	for (; maxLen >= 0; maxLen -= 8) 
	{
		__m512i b0 = _mm512_maskz_loadu_epi64(mbMask[0], loc_src[0]);  loc_src[0] += CFB16_BLOCK_SIZE * 4;
		__m512i b1 = _mm512_maskz_loadu_epi64(mbMask[1], loc_src[1]);  loc_src[1] += CFB16_BLOCK_SIZE * 4;
		__m512i b2 = _mm512_maskz_loadu_epi64(mbMask[2], loc_src[2]);  loc_src[2] += CFB16_BLOCK_SIZE * 4;
		__m512i b3 = _mm512_maskz_loadu_epi64(mbMask[3], loc_src[3]);  loc_src[3] += CFB16_BLOCK_SIZE * 4;
		__m512i b4 = _mm512_maskz_loadu_epi64(mbMask[4], loc_src[4]);  loc_src[4] += CFB16_BLOCK_SIZE * 4;
		__m512i b5 = _mm512_maskz_loadu_epi64(mbMask[5], loc_src[5]);  loc_src[5] += CFB16_BLOCK_SIZE * 4;
		__m512i b6 = _mm512_maskz_loadu_epi64(mbMask[6], loc_src[6]);  loc_src[6] += CFB16_BLOCK_SIZE * 4;
		__m512i b7 = _mm512_maskz_loadu_epi64(mbMask[7], loc_src[7]);  loc_src[7] += CFB16_BLOCK_SIZE * 4;
		__m512i b8 = _mm512_maskz_loadu_epi64(mbMask[8], loc_src[8]);  loc_src[8] += CFB16_BLOCK_SIZE * 4;
		__m512i b9 = _mm512_maskz_loadu_epi64(mbMask[9], loc_src[9]);  loc_src[9] += CFB16_BLOCK_SIZE * 4;
		__m512i b10 = _mm512_maskz_loadu_epi64(mbMask[10], loc_src[10]); loc_src[10] += CFB16_BLOCK_SIZE * 4;
		__m512i b11 = _mm512_maskz_loadu_epi64(mbMask[11], loc_src[11]); loc_src[11] += CFB16_BLOCK_SIZE * 4;
		__m512i b12 = _mm512_maskz_loadu_epi64(mbMask[12], loc_src[12]); loc_src[12] += CFB16_BLOCK_SIZE * 4;
		__m512i b13 = _mm512_maskz_loadu_epi64(mbMask[13], loc_src[13]); loc_src[13] += CFB16_BLOCK_SIZE * 4;
		__m512i b14 = _mm512_maskz_loadu_epi64(mbMask[14], loc_src[14]); loc_src[14] += CFB16_BLOCK_SIZE * 4;
		__m512i b15 = _mm512_maskz_loadu_epi64(mbMask[15], loc_src[15]); loc_src[15] += CFB16_BLOCK_SIZE * 4;

		TRANSPOSE_4x4_I128(b0, b1, b2, b3);       // {0,0,0,0}, {1,1,1,1}, {2,2,2,2}, {3,3,3,3}
		TRANSPOSE_4x4_I128(b4, b5, b6, b7);       // {0,0,0,0}, {1,1,1,1}, {2,2,2,2}, {3,3,3,3}
		TRANSPOSE_4x4_I128(b8, b9, b10, b11);     // {0,0,0,0}, {1,1,1,1}, {2,2,2,2}, {3,3,3,3}
		TRANSPOSE_4x4_I128(b12, b13, b14, b15);   // {0,0,0,0}, {1,1,1,1}, {2,2,2,2}, {3,3,3,3}

		AES_ENCRYPT_VAES_MB16(chip0, chip1, chip2, chip3, keySchedule, num_rounds);
		chip0 = b0 = _mm512_xor_si512(b0, chip0);
		chip1 = b4 = _mm512_xor_si512(b4, chip1);
		chip2 = b8 = _mm512_xor_si512(b8, chip2);
		chip3 = b12 = _mm512_xor_si512(b12, chip3);

		AES_ENCRYPT_VAES_MB16(chip0, chip1, chip2, chip3, keySchedule, num_rounds);
		chip0 = b1 = _mm512_xor_si512(b1, chip0);
		chip1 = b5 = _mm512_xor_si512(b5, chip1);
		chip2 = b9 = _mm512_xor_si512(b9, chip2);
		chip3 = b13 = _mm512_xor_si512(b13, chip3);

		AES_ENCRYPT_VAES_MB16(chip0, chip1, chip2, chip3, keySchedule, num_rounds);
		chip0 = b2 = _mm512_xor_si512(b2, chip0);
		chip1 = b6 = _mm512_xor_si512(b6, chip1);
		chip2 = b10 = _mm512_xor_si512(b10, chip2);
		chip3 = b14 = _mm512_xor_si512(b14, chip3);

		AES_ENCRYPT_VAES_MB16(chip0, chip1, chip2, chip3, keySchedule, num_rounds);
		chip0 = b3 = _mm512_xor_si512(b3, chip0);
		chip1 = b7 = _mm512_xor_si512(b7, chip1);
		chip2 = b11 = _mm512_xor_si512(b11, chip2);
		chip3 = b15 = _mm512_xor_si512(b15, chip3);

		TRANSPOSE_4x4_I128(b0, b1, b2, b3);
		TRANSPOSE_4x4_I128(b4, b5, b6, b7);
		TRANSPOSE_4x4_I128(b8, b9, b10, b11);
		TRANSPOSE_4x4_I128(b12, b13, b14, b15);

		_mm512_mask_storeu_epi64(loc_dst[0], mbMask[0], b0);  loc_dst[0] += CFB16_BLOCK_SIZE * 4;  loc_len64[0] -= 2 * 4; UPDATE_MASK(loc_len64[0], mbMask[0]);
		_mm512_mask_storeu_epi64(loc_dst[1], mbMask[1], b1);  loc_dst[1] += CFB16_BLOCK_SIZE * 4;  loc_len64[1] -= 2 * 4; UPDATE_MASK(loc_len64[1], mbMask[1]);
		_mm512_mask_storeu_epi64(loc_dst[2], mbMask[2], b2);  loc_dst[2] += CFB16_BLOCK_SIZE * 4;  loc_len64[2] -= 2 * 4; UPDATE_MASK(loc_len64[2], mbMask[2]);
		_mm512_mask_storeu_epi64(loc_dst[3], mbMask[3], b3);  loc_dst[3] += CFB16_BLOCK_SIZE * 4;  loc_len64[3] -= 2 * 4; UPDATE_MASK(loc_len64[3], mbMask[3]);
		_mm512_mask_storeu_epi64(loc_dst[4], mbMask[4], b4);  loc_dst[4] += CFB16_BLOCK_SIZE * 4;  loc_len64[4] -= 2 * 4; UPDATE_MASK(loc_len64[4], mbMask[4]);
		_mm512_mask_storeu_epi64(loc_dst[5], mbMask[5], b5);  loc_dst[5] += CFB16_BLOCK_SIZE * 4;  loc_len64[5] -= 2 * 4; UPDATE_MASK(loc_len64[5], mbMask[5]);
		_mm512_mask_storeu_epi64(loc_dst[6], mbMask[6], b6);  loc_dst[6] += CFB16_BLOCK_SIZE * 4;  loc_len64[6] -= 2 * 4; UPDATE_MASK(loc_len64[6], mbMask[6]);
		_mm512_mask_storeu_epi64(loc_dst[7], mbMask[7], b7);  loc_dst[7] += CFB16_BLOCK_SIZE * 4;  loc_len64[7] -= 2 * 4; UPDATE_MASK(loc_len64[7], mbMask[7]);
		_mm512_mask_storeu_epi64(loc_dst[8], mbMask[8], b8);  loc_dst[8] += CFB16_BLOCK_SIZE * 4;  loc_len64[8] -= 2 * 4; UPDATE_MASK(loc_len64[8], mbMask[8]);
		_mm512_mask_storeu_epi64(loc_dst[9], mbMask[9], b9);  loc_dst[9] += CFB16_BLOCK_SIZE * 4;  loc_len64[9] -= 2 * 4; UPDATE_MASK(loc_len64[9], mbMask[9]);
		_mm512_mask_storeu_epi64(loc_dst[10], mbMask[10], b10); loc_dst[10] += CFB16_BLOCK_SIZE * 4;  loc_len64[10] -= 2 * 4; UPDATE_MASK(loc_len64[10], mbMask[10]);
		_mm512_mask_storeu_epi64(loc_dst[11], mbMask[11], b11); loc_dst[11] += CFB16_BLOCK_SIZE * 4;  loc_len64[11] -= 2 * 4; UPDATE_MASK(loc_len64[11], mbMask[11]);
		_mm512_mask_storeu_epi64(loc_dst[12], mbMask[12], b12); loc_dst[12] += CFB16_BLOCK_SIZE * 4;  loc_len64[12] -= 2 * 4; UPDATE_MASK(loc_len64[12], mbMask[12]);
		_mm512_mask_storeu_epi64(loc_dst[13], mbMask[13], b13); loc_dst[13] += CFB16_BLOCK_SIZE * 4;  loc_len64[13] -= 2 * 4; UPDATE_MASK(loc_len64[13], mbMask[13]);
		_mm512_mask_storeu_epi64(loc_dst[14], mbMask[14], b14); loc_dst[14] += CFB16_BLOCK_SIZE * 4;  loc_len64[14] -= 2 * 4; UPDATE_MASK(loc_len64[14], mbMask[14]);
		_mm512_mask_storeu_epi64(loc_dst[15], mbMask[15], b15); loc_dst[15] += CFB16_BLOCK_SIZE * 4;  loc_len64[15] -= 2 * 4; UPDATE_MASK(loc_len64[15], mbMask[15]);
	}
}

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
    #pragma optimize( "", on )
#endif

#endif
