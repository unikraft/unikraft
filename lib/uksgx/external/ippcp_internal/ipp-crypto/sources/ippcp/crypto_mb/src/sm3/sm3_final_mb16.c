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

__ALIGN64 static const int8u swapBytes[] = { 7,6,5,4, 3,2,1,0, 15,14,13,12, 11,10,9,8,
                                             7,6,5,4, 3,2,1,0, 15,14,13,12, 11,10,9,8,
                                             7,6,5,4, 3,2,1,0, 15,14,13,12, 11,10,9,8,
                                             7,6,5,4, 3,2,1,0, 15,14,13,12, 11,10,9,8 };

__INLINE void TRANSPOSE_8X8_I32(__m256i *v0, __m256i *v1, __m256i *v2, __m256i *v3,
                                __m256i *v4, __m256i *v5, __m256i *v6, __m256i *v7) 
{
    __m256i w0, w1, w2, w3, w4, w5, w6, w7;
    __m256i x0, x1, x2, x3, x4, x5, x6, x7;
    __m256i t1, t2;

    x0 = _mm256_permute4x64_epi64(*v0, 0b11011000);
    x1 = _mm256_permute4x64_epi64(*v1, 0b11011000);
    w0 = _mm256_unpacklo_epi32(x0, x1);
    w1 = _mm256_unpackhi_epi32(x0, x1);

    x2 = _mm256_permute4x64_epi64(*v2, 0b11011000);
    x3 = _mm256_permute4x64_epi64(*v3, 0b11011000);
    w2 = _mm256_unpacklo_epi32(x2, x3);
    w3 = _mm256_unpackhi_epi32(x2, x3);

    x4 = _mm256_permute4x64_epi64(*v4, 0b11011000);
    x5 = _mm256_permute4x64_epi64(*v5, 0b11011000);
    w4 = _mm256_unpacklo_epi32(x4, x5);
    w5 = _mm256_unpackhi_epi32(x4, x5);

    x6 = _mm256_permute4x64_epi64(*v6, 0b11011000);
    x7 = _mm256_permute4x64_epi64(*v7, 0b11011000);
    w6 = _mm256_unpacklo_epi32(x6, x7);
    w7 = _mm256_unpackhi_epi32(x6, x7);

    t1 = _mm256_permute4x64_epi64(w0, 0b11011000);
    t2 = _mm256_permute4x64_epi64(w2, 0b11011000);
    x0 = _mm256_unpacklo_epi64(t1, t2);
    x1 = _mm256_unpackhi_epi64(t1, t2);

    t1 = _mm256_permute4x64_epi64(w1, 0b11011000);
    t2 = _mm256_permute4x64_epi64(w3, 0b11011000);
    x2 = _mm256_unpacklo_epi64(t1, t2);
    x3 = _mm256_unpackhi_epi64(t1, t2);

    t1 = _mm256_permute4x64_epi64(w4, 0b11011000);
    t2 = _mm256_permute4x64_epi64(w6, 0b11011000);
    x4 = _mm256_unpacklo_epi64(t1, t2);
    x5 = _mm256_unpackhi_epi64(t1, t2);

    t1 = _mm256_permute4x64_epi64(w5, 0b11011000);
    t2 = _mm256_permute4x64_epi64(w7, 0b11011000);
    x6 = _mm256_unpacklo_epi64(t1, t2);
    x7 = _mm256_unpackhi_epi64(t1, t2);
    
    *v0 = _mm256_permute2x128_si256(x0, x4, 0b100000);
    *v1 = _mm256_permute2x128_si256(x0, x4, 0b110001);
    *v2 = _mm256_permute2x128_si256(x1, x5, 0b100000);
    *v3 = _mm256_permute2x128_si256(x1, x5, 0b110001);
    *v4 = _mm256_permute2x128_si256(x2, x6, 0b100000);
    *v5 = _mm256_permute2x128_si256(x2, x6, 0b110001);
    *v6 = _mm256_permute2x128_si256(x3, x7, 0b100000);
    *v7 = _mm256_permute2x128_si256(x3, x7, 0b110001);
}


void TRANSPOSE_8X16_I32(const int32u* out[16], int32u inp[][16], __mmask16 mb_mask) {
    __m256i* v0 = (__m256i*)&inp[0];
    __m256i* v1 = (__m256i*)&inp[1];
    __m256i* v2 = (__m256i*)&inp[2];
    __m256i* v3 = (__m256i*)&inp[3];
    __m256i* v4 = (__m256i*)&inp[4];
    __m256i* v5 = (__m256i*)&inp[5];
    __m256i* v6 = (__m256i*)&inp[6];
    __m256i* v7 = (__m256i*)&inp[7];

    TRANSPOSE_8X8_I32(v0, v1, v2, v3, v4, v5, v6, v7);

    /* mask store hashes to the first 8 buffers */
    _mm256_mask_storeu_epi32((void*)out[0], (__mmask8)(((mb_mask >> 0) & 1)) * 0xFF, *v0);
    _mm256_mask_storeu_epi32((void*)out[1], (__mmask8)(((mb_mask >> 1) & 1)) * 0xFF, *v1);
    _mm256_mask_storeu_epi32((void*)out[2], (__mmask8)(((mb_mask >> 2) & 1)) * 0xFF, *v2);
    _mm256_mask_storeu_epi32((void*)out[3], (__mmask8)(((mb_mask >> 3) & 1)) * 0xFF, *v3);
    _mm256_mask_storeu_epi32((void*)out[4], (__mmask8)(((mb_mask >> 4) & 1)) * 0xFF, *v4);
    _mm256_mask_storeu_epi32((void*)out[5], (__mmask8)(((mb_mask >> 5) & 1)) * 0xFF, *v5);
    _mm256_mask_storeu_epi32((void*)out[6], (__mmask8)(((mb_mask >> 6) & 1)) * 0xFF, *v6);
    _mm256_mask_storeu_epi32((void*)out[7], (__mmask8)(((mb_mask >> 7) & 1)) * 0xFF, *v7);

    v0 = (__m256i*)&inp[0]+1;
    v1 = (__m256i*)&inp[1]+1;
    v2 = (__m256i*)&inp[2]+1;
    v3 = (__m256i*)&inp[3]+1;
    v4 = (__m256i*)&inp[4]+1;
    v5 = (__m256i*)&inp[5]+1;
    v6 = (__m256i*)&inp[6]+1;
    v7 = (__m256i*)&inp[7]+1;

    TRANSPOSE_8X8_I32(v0, v1, v2, v3, v4, v5, v6, v7);

    /* mask store hashes to the last 8 buffers */
    _mm256_mask_storeu_epi32((void*)out[8],  (__mmask8)(((mb_mask >> 8) & 1)) * 0xFF, *v0);
    _mm256_mask_storeu_epi32((void*)out[9],  (__mmask8)(((mb_mask >> 9) & 1)) * 0xFF, *v1);
    _mm256_mask_storeu_epi32((void*)out[10], (__mmask8)(((mb_mask >> 10) & 1)) * 0xFF, *v2);
    _mm256_mask_storeu_epi32((void*)out[11], (__mmask8)(((mb_mask >> 11) & 1)) * 0xFF, *v3);
    _mm256_mask_storeu_epi32((void*)out[12], (__mmask8)(((mb_mask >> 12) & 1)) * 0xFF, *v4);
    _mm256_mask_storeu_epi32((void*)out[13], (__mmask8)(((mb_mask >> 13) & 1)) * 0xFF, *v5);
    _mm256_mask_storeu_epi32((void*)out[14], (__mmask8)(((mb_mask >> 14) & 1)) * 0xFF, *v6);
    _mm256_mask_storeu_epi32((void*)out[15], (__mmask8)(((mb_mask >> 15) & 1)) * 0xFF, *v7);
}

DLL_PUBLIC
mbx_status16 mbx_sm3_final_mb16(int8u* hash_pa[16], 
                                       SM3_CTX_mb16 * p_state)
{
    int i;
    mbx_status16 status = 0;

    /* test input pointers */
    if(NULL==hash_pa || NULL==p_state) {
        status = MBX_SET_STS16_ALL(MBX_STATUS_NULL_PARAM_ERR);
        return status;
    }

    int input_len[SM3_NUM_BUFFERS];
    int buffer_len[SM3_NUM_BUFFERS];
    int64u sum_msg_len[SM3_NUM_BUFFERS];

    /* allocate local buffer */
    __ALIGN64 int8u loc_buffer[SM3_NUM_BUFFERS][SM3_MSG_BLOCK_SIZE*2];
    int8u* buffer_pa[SM3_NUM_BUFFERS] = { loc_buffer[0],  loc_buffer[1],  loc_buffer[2],  loc_buffer[3], 
                                               loc_buffer[4],  loc_buffer[5],  loc_buffer[6],  loc_buffer[7],
                                               loc_buffer[8],  loc_buffer[9],  loc_buffer[10], loc_buffer[11],
                                               loc_buffer[12], loc_buffer[13], loc_buffer[14], loc_buffer[15] };

    __m512i zero_buffer = _mm512_setzero_si512();

    /* 
    // create  __mmask8 and __mmask16 based on input hash_pa 
    // corresponding element in mask = 0 if hash_pa[i] = 0 
    */
    __mmask8 mb_mask8[2];
    mb_mask8[0] = _mm512_cmp_epi64_mask(_mm512_loadu_si512(hash_pa), zero_buffer, 4);
    mb_mask8[1] = _mm512_cmp_epi64_mask(_mm512_loadu_si512(hash_pa + 8), zero_buffer, 4);
    __mmask16 mb_mask16 = *(__mmask16*)mb_mask8;

    M512(sum_msg_len) = _mm512_maskz_loadu_epi64(mb_mask8[0], MSG_LEN(p_state));
    M512(sum_msg_len + 8) = _mm512_maskz_loadu_epi64(mb_mask8[1], MSG_LEN(p_state) + 8);
    
    /* put processed message length in bits */
    M512(sum_msg_len) = _mm512_rol_epi64(M512(sum_msg_len), 3);
    M512(sum_msg_len + 8) = _mm512_rol_epi64(M512(sum_msg_len+8), 3);
    M512(sum_msg_len) = _mm512_shuffle_epi8(M512(sum_msg_len), M512(swapBytes));
    M512(sum_msg_len +8) = _mm512_shuffle_epi8(M512(sum_msg_len +8), M512(swapBytes));

    M512(input_len) = _mm512_maskz_loadu_epi32(mb_mask16, HAHS_BUFFIDX(p_state));

    __mmask16 tmp_mask = _mm512_cmplt_epi32_mask(M512(input_len), _mm512_set1_epi32(SM3_MSG_BLOCK_SIZE - (int)SM3_MSG_LEN_REPR));
    M512(buffer_len) = _mm512_mask_set1_epi32(_mm512_set1_epi32(SM3_MSG_BLOCK_SIZE * 2), tmp_mask, SM3_MSG_BLOCK_SIZE);
    M512(buffer_len) = _mm512_mask_set1_epi32(M512(buffer_len), ~mb_mask16, 0);

    M512(buffer_len) = _mm512_mask_set1_epi32(M512(buffer_len), ~mb_mask16, 0);
   
    __mmask64 mb_mask64;
    for (i = 0; i < SM3_NUM_BUFFERS; i++) {
        /* Copy rest of message into internal buffer */
        mb_mask64 = ~(0xFFFFFFFFFFFFFFFF << input_len[i]);
        M512(loc_buffer[i]) = _mm512_maskz_loadu_epi8(mb_mask64, HASH_BUFF(p_state)[i]);

        /* Padd message */
        loc_buffer[i][input_len[i]++] = 0x80;
        pad_block(0, loc_buffer[i] + input_len[i], (int)(buffer_len[i] - input_len[i] - (int)SM3_MSG_LEN_REPR));
        ((int64u*)(loc_buffer[i] + buffer_len[i]))[-1] = sum_msg_len[i];
    }

    /* Copmplete hash computation */
    sm3_avx512_mb16((int32u**)HASH_VALUE(p_state), (const int8u**)buffer_pa, buffer_len);
    
    /* Convert hash into big endian */
    __m512i T[8];
    T[0]  = SIMD_ENDIANNESS32(_mm512_loadu_si512(HASH_VALUE(p_state)[0]));
    T[1]  = SIMD_ENDIANNESS32(_mm512_loadu_si512(HASH_VALUE(p_state)[1]));
    T[2]  = SIMD_ENDIANNESS32(_mm512_loadu_si512(HASH_VALUE(p_state)[2]));
    T[3]  = SIMD_ENDIANNESS32(_mm512_loadu_si512(HASH_VALUE(p_state)[3]));
    T[4]  = SIMD_ENDIANNESS32(_mm512_loadu_si512(HASH_VALUE(p_state)[4]));
    T[5]  = SIMD_ENDIANNESS32(_mm512_loadu_si512(HASH_VALUE(p_state)[5]));
    T[6]  = SIMD_ENDIANNESS32(_mm512_loadu_si512(HASH_VALUE(p_state)[6]));
    T[7]  = SIMD_ENDIANNESS32(_mm512_loadu_si512(HASH_VALUE(p_state)[7]));
    
    /* Transpose hash and store in array with pointers to hash values */
    TRANSPOSE_8X16_I32((const int32u**)hash_pa, (int32u(*)[16])T, mb_mask16);

    /* re-init hash value using mb masks */
    _mm512_storeu_si512(MSG_LEN(p_state), _mm512_mask_set1_epi64(_mm512_loadu_si512(MSG_LEN(p_state)), mb_mask8[0], 0));
    _mm512_storeu_si512(MSG_LEN(p_state)+8, _mm512_mask_set1_epi64(_mm512_loadu_si512(MSG_LEN(p_state)+8), mb_mask8[1], 0));
    _mm512_storeu_si512(HAHS_BUFFIDX(p_state), _mm512_mask_set1_epi32(_mm512_loadu_si512(HAHS_BUFFIDX(p_state)), mb_mask16, 0));

    sm3_mask_init_mb16(p_state, mb_mask16);
    
    return status;
}
