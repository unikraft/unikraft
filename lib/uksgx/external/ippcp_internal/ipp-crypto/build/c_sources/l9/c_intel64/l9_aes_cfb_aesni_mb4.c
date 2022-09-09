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
#include "aes_cfb_aesni_mb.h"

#if (_IPP32E>=_IPP32E_Y8)

static inline void aes_encrypt4_aesni_mb4(__m128i blocks[4], __m128i enc_keys[4][15], int cipherRounds)
{

    blocks[0] = _mm_xor_si128(blocks[0], enc_keys[0][0]);
    blocks[1] = _mm_xor_si128(blocks[1], enc_keys[1][0]);
    blocks[2] = _mm_xor_si128(blocks[2], enc_keys[2][0]);
    blocks[3] = _mm_xor_si128(blocks[3], enc_keys[3][0]);

    int nr;

    for (nr = 1; nr < cipherRounds; nr += 1) {

        blocks[0] = _mm_aesenc_si128(blocks[0], enc_keys[0][nr]);
        blocks[1] = _mm_aesenc_si128(blocks[1], enc_keys[1][nr]);
        blocks[2] = _mm_aesenc_si128(blocks[2], enc_keys[2][nr]);
        blocks[3] = _mm_aesenc_si128(blocks[3], enc_keys[3][nr]);

    }

    blocks[0] = _mm_aesenclast_si128(blocks[0], enc_keys[0][nr]);
    blocks[1] = _mm_aesenclast_si128(blocks[1], enc_keys[1][nr]);
    blocks[2] = _mm_aesenclast_si128(blocks[2], enc_keys[2][nr]);
    blocks[3] = _mm_aesenclast_si128(blocks[3], enc_keys[3][nr]);
}


IPP_OWN_DEFN (void, aes_cfb16_enc_aesni_mb4, (const Ipp8u* const source_pa[4], Ipp8u* const dst_pa[4], const int len[4], const int cipherRounds, const Ipp32u* enc_keys[4], const Ipp8u* pIV[4]))
{
    __m128i* pSrc[4];
    __m128i* pDst[4];

    __m128i blocks[4];
    __m128i plainBlocks[4];

    int nBlocks[4];

    int maxBlocks = 0;

    __m128i keySchedule[4][15];

    for (int i = 0; i < 4; i++) {
        pSrc[i] = (__m128i*)source_pa[i];
        pDst[i] = (__m128i*)dst_pa[i];

        nBlocks[i] = len[i] / CFB16_BLOCK_SIZE;

        if(nBlocks[i] > 0) {
            blocks[i] = _mm_loadu_si128((__m128i const*)(pIV[i]));

            for (int j = 0; j <= cipherRounds; j++) {
                keySchedule[i][j] = _mm_loadu_si128((__m128i const*)enc_keys[i] + j);
            }
        }

        if (nBlocks[i] > maxBlocks) {
            maxBlocks = nBlocks[i];
        }
    }

    for (int block = 0; block < maxBlocks; block++) {
        aes_encrypt4_aesni_mb4(blocks, keySchedule, cipherRounds);

        for (int i = 0; i < 4; i++) {
            if (nBlocks[i] > 0) {
                plainBlocks[i] = _mm_loadu_si128(pSrc[i]);
                blocks[i] = _mm_xor_si128(blocks[i], plainBlocks[i]);
                _mm_storeu_si128(pDst[i], blocks[i]);

                pSrc[i]+= 1;
                pDst[i]+= 1;
                nBlocks[i] -= 1;
            }
        }
    }
}

#endif
