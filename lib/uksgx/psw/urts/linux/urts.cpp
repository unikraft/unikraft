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


#include "sgx_error.h"
#include "sgx_urts.h"
#include "sgx_uswitchless.h"
#include "se_types.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>

#include "urts_com.h"

static bool inline _check_ex_params_(const uint32_t ex_features, const void* ex_features_p[32])
{
    //update last feature index if it fails here
    se_static_assert(_SGX_LAST_EX_FEATURE_IDX_ == SGX_CREATE_ENCLAVE_EX_KSS_BIT_IDX);
    
    uint32_t i;

    if (ex_features_p != NULL)
    {
        for (i = 0; i <= _SGX_LAST_EX_FEATURE_IDX_; i++)
        {
            if (((ex_features & (1<<i)) == 0) && (ex_features_p[i] != NULL))
                return false;
        }

        for (i = _SGX_LAST_EX_FEATURE_IDX_ + 1; i < MAX_EX_FEATURES_COUNT; i++)
        {
            if (ex_features_p[i] != NULL)
                return false;
        }
    }
    
    return ((ex_features | _SGX_EX_FEATURES_MASK_) == _SGX_EX_FEATURES_MASK_);
}

extern "C" sgx_status_t __sgx_create_enclave_ex(const char *file_name, 
                                                const int debug, 
                                                sgx_launch_token_t *launch_token, 
                                                int *launch_token_updated, 
                                                sgx_enclave_id_t *enclave_id, 
                                                sgx_misc_attribute_t *misc_attr,
                                                const uint32_t ex_features,
                                                const void* ex_features_p[32])
{
    sgx_status_t ret = SGX_SUCCESS;

    //Only true or false is valid
    if(TRUE != debug &&  FALSE != debug)
        return SGX_ERROR_INVALID_PARAMETER;

    if (!_check_ex_params_(ex_features, ex_features_p))
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    int fd = open(file_name, O_RDONLY);
    if(-1 == fd)
    {
        SE_TRACE(SE_TRACE_ERROR, "Couldn't open the enclave file, error = %d\n", errno);
        return SGX_ERROR_ENCLAVE_FILE_ACCESS;
    }
    se_file_t file = {NULL, 0, false};
    char resolved_path[PATH_MAX] = {0};
    file.name = realpath(file_name, resolved_path);
    file.name_len = (uint32_t)strnlen_s(resolved_path, PATH_MAX);

    ret = _create_enclave_ex(!!debug, fd, file, NULL, launch_token, launch_token_updated, enclave_id, misc_attr, ex_features, ex_features_p);
    if(SGX_SUCCESS != ret && misc_attr)
    {
        sgx_misc_attribute_t plat_cap;
        memset(&plat_cap, 0, sizeof(plat_cap));
        get_enclave_creator()->get_plat_cap(&plat_cap);
        memcpy_s(misc_attr, sizeof(sgx_misc_attribute_t), &plat_cap, sizeof(sgx_misc_attribute_t));
    }

    close(fd);

    return ret;
}

extern "C" sgx_status_t sgx_create_enclave(const char *file_name, 
                                           const int debug, 
                                           sgx_launch_token_t *launch_token, 
                                           int *launch_token_updated, 
                                           sgx_enclave_id_t *enclave_id, 
                                           sgx_misc_attribute_t *misc_attr) 
{
    return __sgx_create_enclave_ex(file_name, debug, launch_token, launch_token_updated, enclave_id, misc_attr, 0, NULL);
}

extern "C"  sgx_status_t sgx_create_enclave_ex(const char *file_name,
                                               const int debug,
                                               sgx_launch_token_t *launch_token,
                                               int *launch_token_updated,
                                               sgx_enclave_id_t *enclave_id,
                                               sgx_misc_attribute_t *misc_attr,
                                               const uint32_t ex_features,
                                               const void* ex_features_p[32])
{

    return __sgx_create_enclave_ex(file_name, debug, launch_token,
        launch_token_updated, enclave_id, misc_attr, ex_features, ex_features_p);
}

extern "C" sgx_status_t sgx_get_target_info(
	const sgx_enclave_id_t enclave_id,
	sgx_target_info_t* target_info)
{
    if (!target_info)
        return SGX_ERROR_INVALID_PARAMETER;

    CEnclave* enclave = CEnclavePool::instance()->get_enclave(enclave_id);
    if (!enclave) {
        return SGX_ERROR_INVALID_ENCLAVE_ID;
    }
    *target_info = enclave->get_target_info();
    return SGX_SUCCESS;
}


extern "C" sgx_status_t sgx_create_enclave_from_buffer_ex(uint8_t *buffer,
                                                          uint64_t buffer_size,
                                                          const int debug,
                                                          sgx_enclave_id_t *enclave_id,
                                                          sgx_misc_attribute_t *misc_attr,
                                                          const uint32_t ex_features,
                                                          const void* ex_features_p[32])
{
    sgx_status_t ret = SGX_SUCCESS;

    // Only true or false is valid
    if (TRUE != debug &&  FALSE != debug)
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    if (!_check_ex_params_(ex_features, ex_features_p))
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    se_file_t file = {NULL, 0, false};
    ret = _create_enclave_from_buffer_ex(!!debug, buffer, buffer_size, file, NULL, enclave_id,
		                         misc_attr, ex_features, ex_features_p);
    if (SGX_SUCCESS != ret && misc_attr)
    {
        sgx_misc_attribute_t plat_cap;
        memset(&plat_cap, 0, sizeof(plat_cap));
        get_enclave_creator()->get_plat_cap(&plat_cap);
        memcpy_s(misc_attr, sizeof(sgx_misc_attribute_t), &plat_cap,
                 sizeof(sgx_misc_attribute_t));
    }

    return ret;
}


extern "C" sgx_status_t
sgx_create_encrypted_enclave(
    const char *file_name,
    const int debug,
    sgx_launch_token_t *launch_token,
    int *launch_token_updated,
    sgx_enclave_id_t *enclave_id,
    sgx_misc_attribute_t *misc_attr,
    uint8_t* sealed_key)
{
    uint32_t ex_features = SGX_CREATE_ENCLAVE_EX_PCL;
    const void* ex_features_p[32] = { 0 };
    ex_features_p[SGX_CREATE_ENCLAVE_EX_PCL_BIT_IDX] = (void*)sealed_key;

    return __sgx_create_enclave_ex(file_name, debug, launch_token,
        launch_token_updated, enclave_id, misc_attr, ex_features, ex_features_p);
}

static bool get_metadata_internal(BinParser *parser, metadata_t **metadata)
{
    if (parser == NULL || metadata == NULL)
        return false;
    uint64_t meta_rva = parser->get_metadata_offset();
    const uint8_t *base_addr = parser->get_start_addr();
    uint64_t urts_version = META_DATA_MAKE_VERSION(MAJOR_VERSION,MINOR_VERSION);
    metadata_t *target_metadata = NULL;

    //assume AE only contains one metadata
    *metadata = GET_PTR(metadata_t, base_addr, meta_rva);

    if(metadata == NULL)
    {
        return false;
    }
    if((*metadata)->magic_num != METADATA_MAGIC)
    {
        return false;
    }
    if(0 == (*metadata)->size)
    {
        SE_TRACE(SE_TRACE_ERROR, "ERROR: metadata's size can't be zero.\n");
        return false;
    }
    //check metadata version
    if(check_metadata_version(urts_version, (*metadata)->version) == true)
    {
        if(target_metadata == NULL ||
           target_metadata->version < (*metadata)->version)
        {
            target_metadata = *metadata;
        }
    }
    if(target_metadata == NULL )
    {
        return false;
    }
    else
    {
        *metadata = target_metadata;
    }
    return true;
}

extern "C" sgx_status_t sgx_get_metadata(const char* enclave_file, metadata_t *metadata)
{
    map_handle_t* mh = NULL;
    metadata_t *p_metadata;
    sgx_status_t ret;

    off_t file_size = 0;
    int fd = open(enclave_file, O_RDONLY);
    if(-1 == fd)
    {
        SE_TRACE(SE_TRACE_ERROR, "Couldn't open the enclave file, error = %d\n", errno);
        return SGX_ERROR_INVALID_PARAMETER;
    }


    mh = map_file(fd, &file_size);
    if (!mh)
    {
        close(fd);
        return SGX_ERROR_INVALID_ENCLAVE;
    }

    PARSER parser(const_cast<uint8_t *>(mh->base_addr), (uint64_t)(file_size));
    if(SGX_SUCCESS != (ret = parser.run_parser()))
    {
        unmap_file(mh);
        close(fd);
        return ret;
    }

    if(true != get_metadata_internal(&parser, &p_metadata))
    {
        unmap_file(mh);
        close(fd);
        return SGX_ERROR_INVALID_METADATA;
    }
    
    if(memcpy_s(metadata, sizeof(metadata_t), p_metadata, sizeof(metadata_t)))
    {
        unmap_file(mh);
        close(fd);
        return SGX_ERROR_UNEXPECTED;
    }
    
    unmap_file(mh);
    close(fd);
    return SGX_SUCCESS;
}
