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

#if !defined(_SM3_HASH_H)
#define _SM3_HASH_H

#include <crypto_mb/defs.h>
#include <crypto_mb/sm3.h>

#include <immintrin.h>

#define SM3_MSG_LEN_REPR                   (sizeof(int64u))               /* size of processed message length representation (bytes) */


#ifndef M512
    #define M512(mem)                   (*((__m512i*)(mem)))
#endif

#ifndef MIN
    #define MIN(a, b) ( ((a) < (b)) ? a : b )
#endif

/*
// change endian
*/

static __ALIGN64 const int8u swapBytesCtx[] = { 3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12,
                                                3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12,
                                                3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12,
                                                3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12 };
#  define SIMD_ENDIANNESS32(x)  _mm512_shuffle_epi8((x), M512(swapBytesCtx));

/*
// accessors to context's fields
*/

#define MSG_LEN(ctx)                ((ctx)->msg_len)
#define HASH_VALUE(ctx)             ((ctx)->msg_hash)
#define HAHS_BUFFIDX(ctx)           ((ctx)->msg_buff_idx)
#define HASH_BUFF(ctx)              ((ctx)->msg_buffer)


/*
// internal functions 
*/

EXTERN_C void sm3_avx512_mb16(int32u* hash_pa[16], const int8u* msg_pa[16], int len[16]);
EXTERN_C void sm3_mask_init_mb16(SM3_CTX_mb16* p_state, unsigned short mb_mask);

__INLINE void pad_block(int8u padding_byte, void* dst_p, int num_bytes)
{
   int8u* d  = (int8u*)dst_p;
   int k;
   for(k = 0; k < num_bytes; k++ )
      d[k] = padding_byte;
}

#endif /* _SM3_HASH_H */
