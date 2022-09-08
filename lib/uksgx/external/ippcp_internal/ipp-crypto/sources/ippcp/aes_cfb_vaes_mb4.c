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

#define AES_ENCRYPT_VAES_MB16(b0, pRkey, num_rounds) { \
   __m512i (*tkeys) =  &pRkey[num_rounds-9]; \
   b0 = _mm512_xor_si512(b0, pRkey[0]); \
   switch(num_rounds) { \
   case 14: \
      b0 = _mm512_aesenc_epi128(b0, tkeys[-4]); \
      b0 = _mm512_aesenc_epi128(b0, tkeys[-3]); \
   case 12: \
      b0 = _mm512_aesenc_epi128(b0, tkeys[-2]); \
      b0 = _mm512_aesenc_epi128(b0, tkeys[-1]); \
   default: \
      b0 = _mm512_aesenc_epi128(b0, tkeys[0]); \
      b0 = _mm512_aesenc_epi128(b0, tkeys[1]); \
      b0 = _mm512_aesenc_epi128(b0, tkeys[2]); \
      b0 = _mm512_aesenc_epi128(b0, tkeys[3]); \
      b0 = _mm512_aesenc_epi128(b0, tkeys[4]); \
      b0 = _mm512_aesenc_epi128(b0, tkeys[5]); \
      b0 = _mm512_aesenc_epi128(b0, tkeys[6]); \
      b0 = _mm512_aesenc_epi128(b0, tkeys[7]); \
      b0 = _mm512_aesenc_epi128(b0, tkeys[8]); \
      b0 = _mm512_aesenclast_epi128(b0, tkeys[9]); \
   } \
}

// Disable optimization for MSVC
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
    #pragma optimize( "", off )
#endif

IPP_OWN_DEFN (void, aes_cfb16_enc_vaes_mb4, (const Ipp8u* const source_pa[4], Ipp8u* const dst_pa[4], const int arr_len[4], const int num_rounds, const Ipp32u* enc_keys[4], const Ipp8u* iv_pa[4]))
{
    int i;
	int maxLen = 0;
	int loc_len64[4];
	Ipp8u* loc_src[4];
	Ipp8u* loc_dst[4];

	__mmask8 mbMask128[4] = { 0x03, 0x0C, 0x30, 0xC0 };
	__mmask8 mbMask[4]    = { 0xFF, 0xFF, 0xFF, 0xFF };

	for (i = 0; i < 4; i++) {
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
    __m512i iv512 = _mm512_setzero_si512();
    iv512 = _mm512_mask_expandloadu_epi64(iv512, mbMask128[0], iv_pa[0]); //  0   0   0  IV1
    iv512 = _mm512_mask_expandloadu_epi64(iv512, mbMask128[1], iv_pa[1]); //  0   0  IV2  0
    iv512 = _mm512_mask_expandloadu_epi64(iv512, mbMask128[2], iv_pa[2]); //  0  IV3  0   0
    iv512 = _mm512_mask_expandloadu_epi64(iv512, mbMask128[3], iv_pa[3]); // IV4  0   0   0

    // Temporary block to left IV unchanged to use it on the next round 
	__m512i chip0 = iv512;

    // Prepare array with key schedule
    __m512i keySchedule[15];
    __m512i tmpKeyMb = _mm512_setzero_si512();
    for (i = 0; i <= num_rounds; i++)
    {
        tmpKeyMb = _mm512_mask_expandloadu_epi64(tmpKeyMb, mbMask128[0], (const void *)(enc_keys[0] + (Ipp32u)i * sizeof(Ipp32u)));
        tmpKeyMb = _mm512_mask_expandloadu_epi64(tmpKeyMb, mbMask128[1], (const void *)(enc_keys[1] + (Ipp32u)i * sizeof(Ipp32u)));
        tmpKeyMb = _mm512_mask_expandloadu_epi64(tmpKeyMb, mbMask128[2], (const void *)(enc_keys[2] + (Ipp32u)i * sizeof(Ipp32u)));
        tmpKeyMb = _mm512_mask_expandloadu_epi64(tmpKeyMb, mbMask128[3], (const void *)(enc_keys[3] + (Ipp32u)i * sizeof(Ipp32u)));

        keySchedule[i] = _mm512_loadu_si512(&tmpKeyMb);
    }

	for (; maxLen >= 0; maxLen -= 8)
	{
		// Load plain text from the different buffers
		__m512i b0 = _mm512_maskz_loadu_epi64(mbMask[0], loc_src[0]);  loc_src[0] += CFB16_BLOCK_SIZE * 4;
		__m512i b1 = _mm512_maskz_loadu_epi64(mbMask[1], loc_src[1]);  loc_src[1] += CFB16_BLOCK_SIZE * 4;
		__m512i b2 = _mm512_maskz_loadu_epi64(mbMask[2], loc_src[2]);  loc_src[2] += CFB16_BLOCK_SIZE * 4;
		__m512i b3 = _mm512_maskz_loadu_epi64(mbMask[3], loc_src[3]);  loc_src[3] += CFB16_BLOCK_SIZE * 4;

		TRANSPOSE_4x4_I128(b0, b1, b2, b3);       // {0,0,0,0}, {1,1,1,1}, {2,2,2,2}, {3,3,3,3}

		AES_ENCRYPT_VAES_MB16(chip0, keySchedule, num_rounds);
		chip0 = b0 = _mm512_xor_si512(b0, chip0);

		AES_ENCRYPT_VAES_MB16(chip0, keySchedule, num_rounds);
		chip0 = b1 = _mm512_xor_si512(b1, chip0);

		AES_ENCRYPT_VAES_MB16(chip0, keySchedule, num_rounds);
		chip0 = b2 = _mm512_xor_si512(b2, chip0);

		AES_ENCRYPT_VAES_MB16(chip0, keySchedule, num_rounds);
		chip0 = b3 = _mm512_xor_si512(b3, chip0);

		TRANSPOSE_4x4_I128(b0, b1, b2, b3);

		_mm512_mask_storeu_epi64(loc_dst[0], mbMask[0], b0);  loc_dst[0] += CFB16_BLOCK_SIZE * 4;  loc_len64[0] -= 2 * 4; UPDATE_MASK(loc_len64[0], mbMask[0]);
		_mm512_mask_storeu_epi64(loc_dst[1], mbMask[1], b1);  loc_dst[1] += CFB16_BLOCK_SIZE * 4;  loc_len64[1] -= 2 * 4; UPDATE_MASK(loc_len64[1], mbMask[1]);
		_mm512_mask_storeu_epi64(loc_dst[2], mbMask[2], b2);  loc_dst[2] += CFB16_BLOCK_SIZE * 4;  loc_len64[2] -= 2 * 4; UPDATE_MASK(loc_len64[2], mbMask[2]);
		_mm512_mask_storeu_epi64(loc_dst[3], mbMask[3], b3);  loc_dst[3] += CFB16_BLOCK_SIZE * 4;  loc_len64[3] -= 2 * 4; UPDATE_MASK(loc_len64[3], mbMask[3]);
    }
}

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
    #pragma optimize( "", on )
#endif

#endif
