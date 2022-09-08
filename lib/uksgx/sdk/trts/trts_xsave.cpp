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


#include "arch.h"
#include "xsave.h"
#include "trts_inst.h"
#include "util.h"
#include "global_data.h"
#include "stdlib.h"
#include "sgx_utils.h"

#define SYNTHETIC_STATE_SIZE   (512 + 64)  // 512 for legacy regs, 64 for xsave header
//FXRSTOR only cares about the first 512 bytes, while
//XRSTOR in compacted mode will ignore the first 512 bytes.
extern "C" SE_DECLSPEC_ALIGN(XSAVE_ALIGN_SIZE) const uint32_t
SYNTHETIC_STATE[SYNTHETIC_STATE_SIZE/sizeof(uint32_t)] __attribute__((section(".niprod"))) = {
    0x037F, 0, 0, 0, 0, 0, 0x1FBF, 0xFFFF, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 0, 2, 0x80000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // XCOMP_BV[63] = 1, compaction mode
};

int g_xsave_enabled __attribute__((section(".nipd"))) = 0;         // flag to indicate whether xsave is enabled or not
#ifdef SE_SIM
uint32_t g_xsave_mask_high __attribute__((section(".nipd"))) = 0xFFFFFFFF;
uint32_t g_xsave_mask_low __attribute__((section(".nipd"))) = 0xFFFFFFFF;
#include "rts_sim.h"
#endif

// EENTER will set xcr0 with secs.attr.xfrm, 
// So use the xfeature mask from report instead of calling xgetbv
#ifdef __clang__
#define SE_OPTIMIZE_OFF [[clang::optnone]]
#else
#define SE_OPTIMIZE_OFF
#endif

SE_OPTIMIZE_OFF
uint64_t get_xfeature_state()
{
#ifndef SE_SIM
    auto *report = sgx_self_report();
    g_xsave_enabled = (report->body.attributes.xfrm == SGX_XFRM_LEGACY) ? 0 : 1;
    uint64_t xfrm = report->body.attributes.xfrm;
#else
    uint64_t xfrm = g_global_data_sim.secs_ptr->attributes.xfrm;
    g_xsave_enabled = (xfrm == SGX_XFRM_LEGACY) ? 0 : 1;
    g_xsave_mask_high = (uint32_t)(xfrm >> 32);
    g_xsave_mask_low = (uint32_t)(xfrm & 0xFFFFFFFF);
#endif

    // no secrets in target_info, report_data, and report. no need to clear them before return
    // tlibc functions cannot be used before calling init_optimized_libs().

    return xfrm;
}


