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

#include <crypto_mb/status.h>
#include <crypto_mb/sm3.h>

#include <internal/sm3/sm3_avx512.h>
#include <internal/common/ifma_defs.h>

// Disable optimization for VS17
#if defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__INTEL_COMPILER)
    #pragma optimize( "", off )
#endif

DLL_PUBLIC
mbx_status16 mbx_sm3_update_mb16(const int8u* msg_pa[16],
    int len[16],
    SM3_CTX_mb16 * p_state)
{
    int i;
    mbx_status16 status = 0;

    /* test input pointers */
    if (NULL == msg_pa || NULL == len || NULL == p_state) {
        status = MBX_SET_STS16_ALL(MBX_STATUS_NULL_PARAM_ERR);
        return status;
    }

    __m512i loc_len = _mm512_loadu_si512(len);
    int* p_loc_len = (int*)&loc_len;

    /* generate mask based on array with messages lengths */
    __m512i zero_buffer = _mm512_setzero_si512();
    __mmask16 mb_mask = _mm512_cmp_epi32_mask(loc_len, zero_buffer, 4);

    /* generate mask based on array with pointers to messages */
    __mmask8 mb_mask8[2];
    mb_mask8[0] = _mm512_cmp_epi64_mask(_mm512_loadu_si512(msg_pa), zero_buffer, 4);
    mb_mask8[1] = _mm512_cmp_epi64_mask(_mm512_loadu_si512(msg_pa + 8), zero_buffer, 4);

    /* don't process the data from i buffer if in msg_pa[i] == 0 or len[i] == 0 */
    mb_mask &= *(__mmask16*)mb_mask8;
    mb_mask8[0] = (__mmask8)mb_mask;
    mb_mask8[1] = *((__mmask8*)&mb_mask + 1);

    int8u* loc_src[SM3_NUM_BUFFERS];

    /* handle non empty message */
    if (mb_mask) {
        _mm512_storeu_si512(loc_src, _mm512_mask_loadu_epi64(_mm512_set1_epi64((long long)&zero_buffer), mb_mask8[0], msg_pa));
        _mm512_storeu_si512(loc_src + 8, _mm512_mask_loadu_epi64(_mm512_set1_epi64((long long)&zero_buffer), mb_mask8[1], msg_pa + 8));

        __m512i proc_len;
        __m512i idx = _mm512_loadu_si512(HAHS_BUFFIDX(p_state));

        int* p_proc_len = (int*)&proc_len;
        int* p_idx = (int*)&idx;

        int64u sum_msg_len[SM3_NUM_BUFFERS] = { (int64u)p_loc_len[0],  (int64u)p_loc_len[1],  (int64u)p_loc_len[2],  (int64u)p_loc_len[3],
                                                    (int64u)p_loc_len[4],  (int64u)p_loc_len[5],  (int64u)p_loc_len[6],  (int64u)p_loc_len[7],
                                                    (int64u)p_loc_len[8],  (int64u)p_loc_len[9],  (int64u)p_loc_len[10], (int64u)p_loc_len[11],
                                                    (int64u)p_loc_len[12], (int64u)p_loc_len[13], (int64u)p_loc_len[14], (int64u)p_loc_len[15] };

        int8u* p_buffer[SM3_NUM_BUFFERS] = { HASH_BUFF(p_state)[0],  HASH_BUFF(p_state)[1],  HASH_BUFF(p_state)[2],  HASH_BUFF(p_state)[3],
                                                  HASH_BUFF(p_state)[4],  HASH_BUFF(p_state)[5],  HASH_BUFF(p_state)[6],  HASH_BUFF(p_state)[7],
                                                  HASH_BUFF(p_state)[8],  HASH_BUFF(p_state)[9],  HASH_BUFF(p_state)[10], HASH_BUFF(p_state)[11],
                                                  HASH_BUFF(p_state)[12], HASH_BUFF(p_state)[13], HASH_BUFF(p_state)[14], HASH_BUFF(p_state)[15] };

        __mmask16 processed_mask = _mm512_cmp_epi32_mask(idx, zero_buffer, 4);

        M512(sum_msg_len) = _mm512_mask_add_epi64(M512(sum_msg_len), mb_mask8[0], _mm512_loadu_si512(MSG_LEN(p_state)), M512(sum_msg_len));
        M512(sum_msg_len + 8) = _mm512_mask_add_epi64(M512(sum_msg_len + 8), mb_mask8[1], _mm512_loadu_si512(MSG_LEN(p_state) + 8), M512(sum_msg_len + 8));

        /* if non empty internal buffer filling */
        if (processed_mask) {

            __m512i tmp = _mm512_sub_epi32(_mm512_set1_epi32(SM3_MSG_BLOCK_SIZE), idx);
            processed_mask = _mm512_cmp_epi32_mask(_mm512_sub_epi32(loc_len, tmp), zero_buffer, 1);

            /* p_proc_len[i] = MIN(p_loc_len[i], (SM3_MSG_BLOCK_SIZE - p_idx[i])) */
            proc_len = _mm512_mask_loadu_epi32(tmp, processed_mask, p_loc_len);
            
            /* copy from input stream to the internal buffer as match as possible */
            for (i = 0; i < SM3_NUM_BUFFERS; i++) {
                /* copy from input stream to the internal buffer as match as possible */
                __mmask64 mb_mask64 = ~(0xFFFFFFFFFFFFFFFF << p_proc_len[i]);
                _mm512_storeu_si512(p_buffer[i] + p_idx[i], _mm512_mask_loadu_epi8(_mm512_loadu_si512(p_buffer[i] + p_idx[i]), mb_mask64, loc_src[i]));
            }

            idx = _mm512_add_epi32(idx, proc_len);
            loc_len = _mm512_sub_epi32(loc_len, proc_len);

            for (i = 0; i < SM3_NUM_BUFFERS; i++) {
                loc_src[i] += p_proc_len[i];
            }

            processed_mask = _mm512_cmp_epi32_mask(idx, _mm512_set1_epi32(SM3_MSG_BLOCK_SIZE), 0);
            proc_len = _mm512_mask_set1_epi32(proc_len, processed_mask, SM3_MSG_BLOCK_SIZE);

            /* update digest if at least one buffer is full */
            if (processed_mask) {
                sm3_avx512_mb16((int32u**)HASH_VALUE(p_state), (const int8u**)p_buffer, p_proc_len);
                idx = _mm512_mask_set1_epi32(idx, ~_mm512_cmp_epi32_mask(proc_len, zero_buffer, 2), 0);
            }
        }

        /* main message part processing */
        proc_len = _mm512_and_epi32(loc_len, _mm512_set1_epi32(-SM3_MSG_BLOCK_SIZE));
        processed_mask = _mm512_cmp_epi32_mask(proc_len, zero_buffer, 5);

        if (processed_mask)
            sm3_avx512_mb16((int32u**)HASH_VALUE(p_state), (const int8u**)loc_src, p_proc_len);

        loc_len = _mm512_sub_epi32(loc_len, proc_len);

        for (i = 0; i < SM3_NUM_BUFFERS; i++) {
            loc_src[i] += p_proc_len[i];
        }

        processed_mask = _mm512_cmp_epi32_mask(loc_len, zero_buffer, 6);

        /* store rest of message into the internal buffer */
        if (processed_mask) {
            for (i = 0; i < SM3_NUM_BUFFERS; i++) {
                /* copy from input stream to the internal buffer as match as possible */
                __mmask64 mb_mask64 = ~(0xFFFFFFFFFFFFFFFF << *(p_loc_len + i));
                _mm512_storeu_si512(p_buffer[i], _mm512_maskz_loadu_epi8(mb_mask64, loc_src[i]));
            }

            idx = _mm512_add_epi32(idx, loc_len);
        }

        /* Update length of processed message */
        _mm512_storeu_si512(MSG_LEN(p_state), _mm512_mask_loadu_epi64(_mm512_loadu_si512(MSG_LEN(p_state)), mb_mask8[0], sum_msg_len));
        _mm512_storeu_si512(MSG_LEN(p_state) + 8, _mm512_mask_loadu_epi64(_mm512_loadu_si512(MSG_LEN(p_state) + 8), mb_mask8[1], sum_msg_len + 8));
        _mm512_storeu_si512(HAHS_BUFFIDX(p_state), _mm512_mask_loadu_epi32(_mm512_loadu_si512(HAHS_BUFFIDX(p_state)), mb_mask, p_idx));
    }

    return status;
}
