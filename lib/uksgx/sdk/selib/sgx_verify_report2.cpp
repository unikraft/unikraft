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
 * File: sgx_verify_report2.cpp
 * Description:
 *     API for the mac structure of the cryptographic report verification
 */

#include "sgx_utils.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
#include "se_memcpy.h"
#include "sgx_trts.h"
#include "se_cdefs.h"
#include "sgx_report2.h"
#include "trts_inst.h"

// add a version to tservice.
SGX_ACCESS_VERSION(tservice, 4)


sgx_status_t sgx_verify_report2(const sgx_report2_mac_struct_t *report_mac_struct)
{
    sgx_status_t err = SGX_ERROR_UNEXPECTED;
    void *buffer = NULL;
    size_t size = 0, buf_ptr = 0;
    sgx_report2_mac_struct_t *tmp_report2_mac_struct = NULL;
    everifyreport2_status_t everifyreport2_status = EVERIFYREPORT2_SUCCESS;
    size_t i = 0;
    // check parameters
    //
    // report must be within the enclave
    if (!report_mac_struct || !sgx_is_within_enclave(report_mac_struct, sizeof(*report_mac_struct)))
    {
        err = SGX_ERROR_INVALID_PARAMETER;
        goto CLEANUP;
    }

    if (report_mac_struct->report_type.type != TEE_REPORT2_TYPE)
    {
        err = SGX_ERROR_INVALID_PARAMETER;
        goto CLEANUP;
    }

    if (report_mac_struct->report_type.subtype != TEE_REPORT2_SUBTYPE
        || report_mac_struct->report_type.version != TEE_REPORT2_VERSION)
    {
        err = SGX_ERROR_INVALID_PARAMETER;
        goto CLEANUP;
    }

    //reserved bytes must be zero
    if (report_mac_struct->report_type.reserved != 0)
    {
        err = SGX_ERROR_INVALID_PARAMETER;
        goto CLEANUP;
    }

    for (i = 0; i < sizeof(report_mac_struct->reserved1); ++i)
    {
        if (report_mac_struct->reserved1[i] != 0)
        {
            err = SGX_ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }
    }
    for (i = 0; i < sizeof(report_mac_struct->reserved2); ++i)
    {
        if (report_mac_struct->reserved2[i] != 0)
        {
            err = SGX_ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }
    }

    // allocate memory
    //
    size = ROUND_TO(sizeof(sgx_report2_mac_struct_t), REPORT2_MAC_STRUCT_ALIGN_SIZE);
    size += (REPORT2_MAC_STRUCT_ALIGN_SIZE - 1);

    buffer = malloc(size);
    if (buffer == NULL)
    {
        err = SGX_ERROR_OUT_OF_MEMORY;
        goto CLEANUP;
    }
    memset(buffer, 0, size);
    buf_ptr = reinterpret_cast<size_t>(buffer);

    buf_ptr = ROUND_TO(buf_ptr, REPORT2_MAC_STRUCT_ALIGN_SIZE);
    tmp_report2_mac_struct = reinterpret_cast<sgx_report2_mac_struct_t *>(buf_ptr);

    // Copy data from user buffer to the aligned memory
    if (0 != memcpy_s(tmp_report2_mac_struct, sizeof(*tmp_report2_mac_struct), report_mac_struct, sizeof(*report_mac_struct))) {
        err = SGX_ERROR_UNEXPECTED;
        goto CLEANUP;
    }

    // Do EVERIFYREPORT2
    everifyreport2_status = (everifyreport2_status_t)do_everifyreport2(tmp_report2_mac_struct);
    switch (everifyreport2_status)
    {
        case EVERIFYREPORT2_SUCCESS:
            err = SGX_SUCCESS;
            break;
        case EVERIFYREPORT2_INVALID_REPORTMACSTRUCT:
            err = SGX_ERROR_MAC_MISMATCH;
            break;
        case EVERIFYREPORT2_INVALID_CPUSVN:
            err = SGX_ERROR_INVALID_CPUSVN;
            break;
        case EVERIFYREPORT2_INVALID_LEAF:
            err = SGX_ERROR_FEATURE_NOT_SUPPORTED;
            break;
        default:
            err = SGX_ERROR_UNEXPECTED;
            break;
    }

CLEANUP:
    if (buffer)
    {
        free(buffer);
        buffer = NULL;
    }
    return err;
}
