/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "stdlib.h"
#include "string.h"
#include "pve_qe_common.h"
#include "assert.h"
#include "util.h"
#include "sgx_trts.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "epid/common/src/memory.h"
#include "epid/member/software_member.h"
#include "epid/member/src/write_precomp.h"
#include "epid/member/src/signbasic.h"
#include "epid/member/src/nrprove.h"
#ifdef __cplusplus
}
#endif


int epid_random_func(
    unsigned int *p_random_data,
    int bits,
    void* p_user_data)
{
    UNUSED(p_user_data);
    assert(!(bits % 8));

    if(SGX_SUCCESS != sgx_read_rand((uint8_t *)p_random_data,
                                    ROUND_TO(bits, 8) / 8))
        return 1;
    return 0;
}

EpidStatus epid_member_create(BitSupplier rnd_func, void* rnd_param, FpElemStr* f, MemberCtx** ctx)
{
    MemberParams params;
    size_t member_size = 0;
    EpidStatus epid_ret = kEpidNoErr;

    memset(&params, 0, sizeof(params));

    params.rnd_func = rnd_func;
    params.rnd_param = rnd_param;
    params.f = f;

    epid_ret = EpidMemberGetSize(&params, &member_size);
    if (kEpidNoErr != epid_ret) {
        return epid_ret;
    }

    *ctx = (MemberCtx*)SAFE_ALLOC(member_size);
    if (!(*ctx)) {
        return kEpidNoMemErr;
    }

    epid_ret = EpidMemberInit(&params, *ctx);
    if (kEpidNoErr != epid_ret) {
        SAFE_FREE(*ctx);
        *ctx = NULL;
        return epid_ret;
    }

    epid_ret = EpidMemberSetHashAlg(*ctx, kSha256);
    if (kEpidNoErr != epid_ret) {
        goto ret_point;
    }

ret_point:
    if (kEpidNoErr != epid_ret) {
        epid_member_delete(ctx);
    }
    return epid_ret;
}

void epid_member_delete(MemberCtx** ctx)
{
    if (!ctx) {
        return;
    }
    EpidMemberDeinit(*ctx);
    SAFE_FREE(*ctx);
    *ctx = NULL;
}
