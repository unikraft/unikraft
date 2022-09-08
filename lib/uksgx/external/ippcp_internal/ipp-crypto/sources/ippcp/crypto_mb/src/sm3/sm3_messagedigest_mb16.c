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

DLL_PUBLIC
mbx_status16 mbx_sm3_msg_digest_mb16(const int8u* msg_pa[16],
                                           int len[16],
                                           int8u* hash_pa[16])
{
    int buf_no;
    mbx_status16 status = 0;

    /* test input pointers */
    if(NULL==msg_pa || NULL==len || NULL==hash_pa) {
        status = MBX_SET_STS16_ALL(MBX_STATUS_NULL_PARAM_ERR);
        return status;
    }

    for (buf_no = 0; buf_no < SM3_NUM_BUFFERS; buf_no++) {
        if ((len[buf_no] && !hash_pa[buf_no]) || (len[buf_no] && !msg_pa[buf_no])) {
            status = MBX_SET_STS16(status, buf_no, MBX_STATUS_NULL_PARAM_ERR);
            return status;
        }    
    }

    /* initialize the context of SM3 hash */
    SM3_CTX_mb16  p_state;
    mbx_sm3_init_mb16(&p_state);

    /* process main part of the message */
    status = mbx_sm3_update_mb16((const int8u**)msg_pa, len, &p_state);

    if(MBX_IS_ANY_OK_STS16(status)) {
        /* finalize message processing */
        status = mbx_sm3_final_mb16(hash_pa, &p_state);
    }
    
    return status;
}
