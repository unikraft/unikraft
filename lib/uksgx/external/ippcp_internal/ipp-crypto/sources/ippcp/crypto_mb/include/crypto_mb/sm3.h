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


#ifndef SM3_H
#define SM3_H

#include <crypto_mb/defs.h>
#include <crypto_mb/status.h>

#define SM3_SIZE_IN_BITS                       (256)                      /*                  sm3 size in bits                       */
#define SM3_SIZE_IN_WORDS       (SM3_SIZE_IN_BITS/(sizeof(int32u)*8))     /*                sm3 hash size in words                   */
#define SM3_MSG_BLOCK_SIZE                     (64)                       /*                  messge block size                      */

#define SM3_NUM_BUFFERS                        (16)                       /*         max number of buffers in sm3 multi buffer       */

/*
// sm3 context
*/

typedef int32u sm3_hash_mb[SM3_SIZE_IN_WORDS][SM3_NUM_BUFFERS];         /* sm3 hash value in multi-buffer format */

struct _sm3_context_mb16 {
    int             msg_buff_idx[SM3_NUM_BUFFERS];                      /*              buffer entry             */
    int64u          msg_len[SM3_NUM_BUFFERS];                           /*              message length           */
    int8u           msg_buffer[SM3_NUM_BUFFERS][SM3_MSG_BLOCK_SIZE];    /*                  buffer               */
    sm3_hash_mb     msg_hash;                                           /*             intermediate hash         */
};

typedef struct _sm3_context_mb16 SM3_CTX_mb16;


EXTERN_C mbx_status16 mbx_sm3_init_mb16(SM3_CTX_mb16 * p_state);

EXTERN_C mbx_status16 mbx_sm3_update_mb16(const int8u* msg_pa[16], 
                                                int len[16], 
                                                SM3_CTX_mb16 * p_state);

EXTERN_C mbx_status16 mbx_sm3_final_mb16(int8u* hash_pa[16], 
                                         SM3_CTX_mb16 * p_state);

EXTERN_C mbx_status16 mbx_sm3_msg_digest_mb16(const int8u* msg_pa[16],
                                                    int len[16],
                                                    int8u* hash_pa[16]);



#endif /* SM3_H */
