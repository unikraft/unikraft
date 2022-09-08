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
 * File: sgx_create_report.cpp
 * Description:
 *     Wrapper for EREPORT instruction
 */

#include "sgx_utils.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
#include "se_memcpy.h"
#include "sgx_trts.h"
#include "trts_inst.h"
#include "se_cdefs.h"
#include "sgx_spinlock.h"

// add a version to tservice.
SGX_ACCESS_VERSION(tservice, 1)

extern "C" void * __memset(void *dst, int c, size_t n);

sgx_status_t sgx_create_report(const sgx_target_info_t *target_info, const sgx_report_data_t *report_data, sgx_report_t *report)
{
    static_assert(sizeof(*target_info) == 512, "sgx_target_info_t");
    static_assert(sizeof(*report_data) == 64, "sgx_report_data_t");
    static_assert(sizeof(*report) == 432, "sgx_report_t");

    alignas(REPORT_DATA_ALIGN_SIZE) sgx_report_data_t tmp_report_data;
    __memset((void *)&tmp_report_data, 0, sizeof(sgx_report_data_t));
    alignas(TARGET_INFO_ALIGN_SIZE) sgx_target_info_t tmp_target_info;
    __memset((void *)&tmp_target_info, 0, sizeof(sgx_target_info_t));    
    alignas(REPORT_ALIGN_SIZE)sgx_report_t tmp_report;
    __memset((void *)&tmp_report, 0, sizeof(sgx_report_t));

    // check parameters
    //
    // target_info is allowed to be NULL, but if it is not NULL, it must be within the enclave
    if(target_info)
    {
        if (!sgx_is_within_enclave(target_info, sizeof(*target_info)))
            return SGX_ERROR_INVALID_PARAMETER;
        tmp_target_info = *target_info;
    }
    // report_data is allowed to be NULL, but if it is not NULL, it must be within the enclave
    if(report_data)
    {
        if(!sgx_is_within_enclave(report_data, sizeof(*report_data)))
            return SGX_ERROR_INVALID_PARAMETER;
        tmp_report_data = *report_data;
    }
    // report must be within the enclave
    if(!report || !sgx_is_within_enclave(report, sizeof(*report)))
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }


    // Do EREPORT
    auto failed = do_ereport(&tmp_target_info, &tmp_report_data, &tmp_report);
    
    // Copy data to the user buffer: *report = tmp_report; 
    // Use a loop to avoid compiler to call memcpy, 
    // which cannot be used during enclave initialization.
    // No need to cleanup the tmp_report as it is not secret.
    if (!failed)
    {
        static_assert(sizeof(*report) % sizeof(uint64_t) == 0, "sizeof(sgx_report_t) should be multiple of 8");
        for(size_t i = 0; i < sizeof(*report)/sizeof(uint64_t); i++)
        {
            ((uint64_t*)report)[i] = ((uint64_t*)&tmp_report)[i];
        }
    }


    return failed ? SGX_ERROR_UNEXPECTED : SGX_SUCCESS;
}

const sgx_report_t *sgx_self_report(void)
{
    static sgx_report_t _report = 
    {
        .body = {
                .cpu_svn         = {0}, 
                .misc_select     = 0, 
                .reserved1       = {0}, 
                .isv_ext_prod_id = {0}, 
                .attributes      = {0, 0},
                .mr_enclave      = {0}, 
                .reserved2       = {0}, 
                .mr_signer       = {0}, 
                .reserved3       = {0},
                .config_id       = {0},
                .isv_prod_id     = 0,
                .isv_svn        = 0, 
                .config_svn      = 0, 
                .reserved4       = {0}, 
                .isv_family_id   = {0},
                .report_data     = {0}
            },
        .key_id = {0},
        .mac = {0}
    };
    static sgx_spinlock_t report_lock = SGX_SPINLOCK_INITIALIZER;

    if (0 == _report.body.attributes.flags)
    {
        // sgx_create_report() only needs to be called once to get self report.
        sgx_spin_lock(&report_lock);
        if (0 == _report.body.attributes.flags)
        {
            sgx_create_report(nullptr, nullptr, &_report);
        }
        sgx_spin_unlock(&report_lock);
    }

    return &_report;
}

