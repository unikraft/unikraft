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


/**
 * File: init_enclave.cpp
 * Description:
 *     Initialize enclave by rebasing the image to the enclave base 
 */

#include <string.h>
#include "thread_data.h"
#include "global_data.h"
#include "util.h"
#include "xsave.h"
#include "sgx_trts.h"
#include "sgx_lfence.h"
#include "init_optimized_lib.h"
#include "trts_internal.h"
#include "linux/elf_parser.h"
#include "rts.h"
#include "trts_util.h"
#include "se_memcpy.h"
#include "se_cpu_feature.h"
#include "se_version.h"

// The global cpu feature bits from uRTS
uint64_t g_cpu_feature_indicator __attribute__((section(RELRO_SECTION_NAME))) = 0;
int EDMM_supported __attribute__((section(RELRO_SECTION_NAME))) = 0;
sdk_version_t g_sdk_version __attribute__((section(RELRO_SECTION_NAME))) = SDK_VERSION_1_5;
uint64_t g_enclave_base __attribute__((section(RELRO_SECTION_NAME))) = 0;
uint64_t g_enclave_size __attribute__((section(RELRO_SECTION_NAME))) = 0;


const volatile global_data_t g_global_data __attribute__((section(".niprod"))) = {VERSION_UINT, 1, 2, 3, 4, 5, 6, 0, 0, 0,
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0}, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, 0, {{{0, 0, 0, 0, 0, 0, 0}}}, 0, 0, 0};
uint32_t g_enclave_state __attribute__((section(".nipd"))) = ENCLAVE_INIT_NOT_STARTED;
uint32_t g_cpu_core_num __attribute__((section(RELRO_SECTION_NAME))) = 0;

extern "C" {
uintptr_t __stack_chk_guard __attribute__((section(RELRO_SECTION_NAME)))= 0;
#define __weak_alias(alias,sym)                 \
    __asm__(".weak " __STRING(alias) " ; "      \
        __STRING(alias) " = " __STRING(sym))
__weak_alias(__intel_security_cookie, __stack_chk_guard);
}

extern sgx_status_t pcl_entry(void* enclave_base,void* ms) __attribute__((weak));
extern "C" int init_enclave(void *enclave_base, void *ms) __attribute__((section(".nipx")));

extern "C" int rsrv_mem_init(void *_rsrv_mem_base, size_t _rsrv_mem_size, size_t _rsrv_mem_min_size);

// init_enclave()
//      Initialize enclave.
// Parameters:
//      [IN] enclave_base - the enclave base address
//      [IN] ms - the marshalling structure passed by uRTS
// Return Value:
//       0 - success
//      -1 - fail
//
extern "C" int init_enclave(void *enclave_base, void *ms)
{
    if(enclave_base == NULL || ms == NULL)
    {
        return -1;
    }

    if(NULL != pcl_entry)
    {
        // LFENCE before pcl_entry
        sgx_lfence();
        sgx_status_t ret = pcl_entry(enclave_base, ms);
        if(SGX_SUCCESS != ret)
        {
            return -1;
        }
    }

    // relocation
    if(0 != relocate_enclave(enclave_base))
    {
        return -1;
    }

    g_enclave_base = (uint64_t)get_enclave_base();
    g_enclave_size = g_global_data.elrange_size;
    //we are not allowed to set enclave_image_address to 0 if elrange is set
    //so if enclave_image_address is 0, it means elrange is not set
    if(g_global_data.enclave_image_address != 0)
    {
        //__ImageBase should be the same as enclave_start_address
        if(g_global_data.enclave_image_address != g_enclave_base)
        {
            abort();
        }
        //if elrange is set, we should set enclave_base to correct value
        g_enclave_base = g_global_data.elrange_start_address;
    }

    // Check if the ms is outside the enclave.
    // sgx_is_outside_enclave() should be placed after relocate_enclave()
    system_features_t *info = (system_features_t *)ms;
    if(!sgx_is_outside_enclave(info, sizeof(system_features_t)))
    {
        return -1;
    }
    sgx_lfence();

    system_features_t sys_features = *info;
    size_t offset = 0;
    if(sys_features.system_feature_set[0] & (1ULL<< SYS_FEATURE_EXTEND))
    {
        offset = (sys_features.size < sizeof(sys_features)) ? sys_features.size : sizeof(sys_features);
    }
    else
    {
        // old urts is used to load the enclave and size is not set by urts.
        // So clear these new fields including 'size'
        //
        offset = offsetof(system_features_t, size);
    }
    for(size_t i = 0; i < sizeof(sys_features) - offset; i++)
    {
        // Clean the fields that cannot be recognized by trts
        *((uint8_t *)&sys_features + offset + i) = 0;
    }


    g_cpu_core_num = sys_features.cpu_core_num;
    g_sdk_version = sys_features.version;
    if (g_sdk_version == SDK_VERSION_1_5)
    {
        EDMM_supported = 0;
    }
    else if (g_sdk_version >= SDK_VERSION_2_0)
    {
        EDMM_supported = feature_supported((const uint64_t *)sys_features.system_feature_set, 0);
    }
    else
    {
        return -1;
    }
    
    if (heap_init(get_heap_base(), get_heap_size(), get_heap_min_size(), EDMM_supported) != SGX_SUCCESS)
        return -1;

#ifdef SE_SIM
    memset_s(GET_PTR(void, enclave_base, g_global_data.heap_offset), g_global_data.heap_size, 0, g_global_data.heap_size);
    if(g_global_data.rsrv_size != 0)
    {
        memset_s(GET_PTR(void, enclave_base, g_global_data.rsrv_offset), g_global_data.rsrv_size, 0, g_global_data.rsrv_size);
    }
#endif
    // xsave
    uint64_t xfrm = get_xfeature_state();

    // Unset conflict cpu feature bits for legacy cpu features.
    uint64_t cpu_features = (sys_features.cpu_features & ~(INCOMPAT_FEATURE_BIT));

    if (sys_features.system_feature_set[0] & ((uint64_t)(1ULL << SYS_FEATURE_EXTEND)))
    {
        // The sys_features structure is collected by updated uRTS, so use the updated cpu features instead.
        cpu_features = sys_features.cpu_features_ext;
    }

    // optimized libs
    if (SDK_VERSION_2_0 < g_sdk_version || sys_features.size != 0)
    {
        if (0 != init_optimized_libs(cpu_features, (uint32_t*)sys_features.cpuinfo_table, xfrm))
        {
            return -1;
        }
    }
    else 
    {
        if (0 != init_optimized_libs(cpu_features, NULL, xfrm))
        {
            return -1;
        }
    }

    if ( get_rsrv_size() != 0)
    {
        if(rsrv_mem_init(get_rsrv_base(), get_rsrv_size(), get_rsrv_min_size()) != SGX_SUCCESS)
            return -1;
    }

    
    if(SGX_SUCCESS != sgx_read_rand((unsigned char*)&__stack_chk_guard,
                                     sizeof(__stack_chk_guard)))
    {
        return -1;
    }

    return 0;
}

#ifndef SE_SIM
int accept_post_remove(const volatile layout_t *layout_start, const volatile layout_t *layout_end, size_t offset);
#endif

extern size_t rsrv_mem_min_size;

sgx_status_t do_init_enclave(void *ms, void *tcs)
{
#ifdef SE_SIM
    UNUSED(tcs);
#endif
    void *enclave_base = get_enclave_base();
    if(ENCLAVE_INIT_NOT_STARTED != lock_enclave())
    {
        return SGX_ERROR_UNEXPECTED;
    }
    if(0 != init_enclave(enclave_base, ms))
    {
        return SGX_ERROR_UNEXPECTED;
    }

#ifndef SE_SIM
    if (SGX_SUCCESS != do_init_thread(tcs, true))
    {
        return SGX_ERROR_UNEXPECTED;
    }

    /* for EDMM, we need to accept the trimming of the POST_REMOVE pages. */
    if (EDMM_supported)
    {
        if (0 != accept_post_remove(&g_global_data.layout_table[0], &g_global_data.layout_table[0] + g_global_data.layout_entry_num, 0))
            return SGX_ERROR_UNEXPECTED;

        size_t heap_min_size = get_heap_min_size();
        memset_s(GET_PTR(void, enclave_base, g_global_data.heap_offset), heap_min_size, 0, heap_min_size);

        memset_s(GET_PTR(void, enclave_base, g_global_data.rsrv_offset), rsrv_mem_min_size, 0, rsrv_mem_min_size);
    }
    else
    {
        memset_s(GET_PTR(void, enclave_base, g_global_data.heap_offset), g_global_data.heap_size, 0, g_global_data.heap_size);
        memset_s(GET_PTR(void, enclave_base, g_global_data.rsrv_offset), g_global_data.rsrv_size, 0, g_global_data.rsrv_size);
    }
#endif

    g_enclave_state = ENCLAVE_INIT_DONE;
    return SGX_SUCCESS;
}

