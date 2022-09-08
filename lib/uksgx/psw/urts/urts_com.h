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

#ifndef _URTS_COM_H_
#define _URTS_COM_H_


#include "arch.h"
#include "sgx_error.h"
#include "se_error_internal.h"
#include "se_trace.h"
#include "file.h"
#include "sgx_eid.h"
#include "se_map.h"
#include "launch_checker.h"
#include "debugger_support.h"
#include "loader.h"
#include "binparser.h"
#include "cpuid.h"
#include "se_macro.h"
#include "prd_css_util.h"
#include "se_detect.h"
#include "rts.h"
#include "enclave_creator_hw.h"
#include "edmm_utility.h"
#include <sys/mman.h>
#ifndef PARSER
#include "elfparser.h"
#define PARSER ElfParser
#endif
#include "xsave.h"

#include "ittnotify.h"
#include "ittnotify_config.h"
#include "ittnotify_types.h"

extern "C" int __itt_init_ittlib(const char*, __itt_group_id);
extern "C" __itt_global* __itt_get_ittapi_global();



#define GET_FEATURE_POINTER(feature_name, ex_features_p)    ex_features_p[feature_name##_BIT_IDX]
// Get the corresponding feature pointer for the input feature name
// This function should only be called for the features which have the corresponding feature structs
// Return value: 
//    -1 - invalid input
//    0  - no such feature request
//    1  - has such feature request
static int get_ex_feature_pointer(uint32_t feature_name, const uint32_t ex_features, const void *ex_features_p[MAX_EX_FEATURES_COUNT], void **op)
{
    bool fbit_set = (feature_name & ex_features) ? true : false;
    void *pointer = NULL;
    int ret = -1;
    switch(feature_name)
    {
    case SGX_CREATE_ENCLAVE_EX_PCL:
        if (ex_features_p != NULL)
            pointer = const_cast<void *>(GET_FEATURE_POINTER(SGX_CREATE_ENCLAVE_EX_PCL, ex_features_p));
        if (fbit_set && pointer)
            ret = 1;
        else if (!fbit_set && !pointer)
            ret = 0;
        break;
    case SGX_CREATE_ENCLAVE_EX_SWITCHLESS:
        if (ex_features_p != NULL)
            pointer = const_cast<void *>(GET_FEATURE_POINTER(SGX_CREATE_ENCLAVE_EX_SWITCHLESS, ex_features_p));
        if (fbit_set && pointer)
            ret = 1;
        else if (!fbit_set && !pointer)
            ret = 0;
        break;
    case SGX_CREATE_ENCLAVE_EX_KSS:
        if (ex_features_p != NULL)
            pointer = const_cast<void *>(GET_FEATURE_POINTER(SGX_CREATE_ENCLAVE_EX_KSS, ex_features_p));
        if (fbit_set && pointer)
            ret = 1;
        else if (!fbit_set && !pointer)
            ret = 0;
        break;
    default:
        break;
    }
    if (ret == 1)
        *op = pointer;
    return ret;
}


#define HSW_C0  0x306c3
#define GPR_A0  0x406e0
#define GPR_B0  0x406e1
#define GPR_P0  0x506e0

#ifndef SE_SIM
static int validate_platform()
{
    int cpu_info[4] = {0, 0, 0, 0};

    __cpuid(cpu_info, 1);

    // The compatibility between SDK and PSW is checked by the metadata version.
    // Below check the compatibility between the platform and uRTS only.

    
    // It is HSW users' responsibility to make the uRTS version to consistent with the HSW patch.
    if(cpu_info[0] == HSW_C0)
    {
        return SGX_SUCCESS;
    }

    // GPR region
    else if(cpu_info[0] == GPR_A0 || cpu_info[0] == GPR_B0 || cpu_info[0] == GPR_P0)
    {
        SE_TRACE(SE_TRACE_ERROR, "ERROR: The enclave cannot be launched on current platform.\n");
        return SGX_ERROR_INVALID_VERSION;
    }
    
    return SGX_SUCCESS;
}
#endif

static bool check_metadata_version(uint64_t urts_version, uint64_t metadata_version)
{
    //for metadata change, we have updated the metadata major version
    if(MAJOR_VERSION_OF_METADATA(urts_version)%SGX_MAJOR_VERSION_GAP < MAJOR_VERSION_OF_METADATA(metadata_version)%SGX_MAJOR_VERSION_GAP)
    {
        return false;
    }
    
    return true;
}


static sgx_status_t get_metadata(BinParser *parser, const int debug, metadata_t **metadata, sgx_misc_attribute_t *sgx_misc_attr)
{
    assert(parser != NULL && metadata != NULL && sgx_misc_attr != NULL);
    uint64_t meta_rva = parser->get_metadata_offset();
    const uint8_t *base_addr = parser->get_start_addr();
    uint64_t urts_version = META_DATA_MAKE_VERSION(MAJOR_VERSION,MINOR_VERSION);
    metadata_t *target_metadata = NULL;

#ifndef SE_SIM
    EnclaveCreatorHW *enclave_creator = static_cast<EnclaveCreatorHW *>(get_enclave_creator());
    if (!is_cpu_support_edmm() || !(enclave_creator->is_driver_compatible()))
    {
        // cannot support EDMM, adjust the possibly highest metadata version supported
        urts_version = META_DATA_MAKE_VERSION(SGX_1_9_MAJOR_VERSION,SGX_1_9_MINOR_VERSION);
    }
#else
    //for simulation, use the metadata of 1.9
    urts_version = META_DATA_MAKE_VERSION(SGX_1_9_MAJOR_VERSION,SGX_1_9_MINOR_VERSION);
#endif

    //scan multiple metadata list in sgx_metadata section
    meta_rva = parser->get_metadata_offset();
    do {
        *metadata = GET_PTR(metadata_t, base_addr, meta_rva);
        if(*metadata == NULL)
        {
            return SGX_ERROR_INVALID_METADATA;
        }
        if((*metadata)->magic_num != METADATA_MAGIC)
        {
            break;
        }
        if(0 == (*metadata)->size)
        {
            SE_TRACE(SE_TRACE_ERROR, "ERROR: metadata's size can't be zero.\n");
            return SGX_ERROR_INVALID_METADATA;
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
        meta_rva += (*metadata)->size; /*goto next metadata offset*/
    }while(1);

    if(target_metadata == NULL )
    {
        return SGX_ERROR_INVALID_METADATA;
    }
    else
    {
        *metadata = target_metadata;
    }

    return (sgx_status_t)get_enclave_creator()->get_misc_attr(sgx_misc_attr, *metadata, NULL, debug);
}


#define MAX_LEN 256
static bool is_SGX_DBG_OPTIN_variable_set()
{
    const char sgx_dbg_optin[] = "SGX_DBG_OPTIN";
    const char sgx_dbg_optin_expect_val[] = "1";
    char *sgx_dbg_optin_val = getenv(sgx_dbg_optin);

    if(sgx_dbg_optin_val == NULL)
    {
        return false;
    }
    size_t expect_len = strnlen_s(sgx_dbg_optin_expect_val, MAX_LEN);
    size_t len = strnlen_s(sgx_dbg_optin_val, MAX_LEN);
    if(len != expect_len || strncmp(sgx_dbg_optin_expect_val, sgx_dbg_optin_val, expect_len) != 0)
    {
        return false;
    }
    return true;
}


static int __create_enclave(BinParser &parser, 
                            uint8_t* base_addr, 
                            const metadata_t *metadata, 
                            se_file_t& file, 
                            const bool debug, 
                            SGXLaunchToken *lc, 
                            le_prd_css_file_t *prd_css_file, 
                            sgx_enclave_id_t *enclave_id, 
                            sgx_misc_attribute_t *misc_attr,
                            const uint32_t ex_features,
                            const void* ex_features_p[32])
{
    debug_enclave_info_t *debug_info = NULL;
    int ret = SGX_SUCCESS;
    sgx_config_id_t *config_id = NULL;
    sgx_config_svn_t config_svn = 0;
    sgx_kss_config_t *kss_config = NULL;
    sgx_uswitchless_config_t* us_config = NULL;
    
    CLoader loader(base_addr, parser);

    if (get_ex_feature_pointer(SGX_CREATE_ENCLAVE_EX_KSS, ex_features, ex_features_p, (void **)&kss_config) == -1)
        return SGX_ERROR_INVALID_PARAMETER;
    if (kss_config)
    {
        config_id = &(kss_config->config_id);
        config_svn = kss_config->config_svn;
    }

    ret = loader.load_enclave_ex(lc, debug, metadata, config_id, config_svn, prd_css_file, misc_attr);
    if (ret != SGX_SUCCESS)
    {
        return ret;
    }

    CEnclave* enclave = new CEnclave();
    uint32_t enclave_version = SDK_VERSION_1_5;
    uint64_t urts_version = META_DATA_MAKE_VERSION(MAJOR_VERSION,MINOR_VERSION);
    // metadata->version has already been validated during load_encalve_ex()
    if (MAJOR_VERSION_OF_METADATA(metadata->version) % SGX_MAJOR_VERSION_GAP == MAJOR_VERSION_OF_METADATA(urts_version)% SGX_MAJOR_VERSION_GAP &&
        MINOR_VERSION_OF_METADATA(metadata->version) >= MINOR_VERSION_OF_METADATA(urts_version))
    {
        enclave_version = SDK_VERSION_2_3;
    }
    else if (MAJOR_VERSION_OF_METADATA(metadata->version) % SGX_MAJOR_VERSION_GAP == MAJOR_VERSION_OF_METADATA(urts_version)% SGX_MAJOR_VERSION_GAP &&
             MINOR_VERSION_OF_METADATA(metadata->version) < MINOR_VERSION_OF_METADATA(urts_version))
    {
        enclave_version = SDK_VERSION_2_0;
    }

    // initialize the enclave object
    ret = enclave->initialize(file,
                              loader,
                              metadata->enclave_size,
                              metadata->tcs_policy,
                              enclave_version,
                              metadata->tcs_min_pool);

    if (ret != SGX_SUCCESS)
    {
        loader.destroy_enclave();
        delete enclave; // The `enclave' object owns the `loader' object.
        return ret;
    }
    
    //set sealed key for encrypted enclave
    uint8_t *sealed_key = NULL;
    if (get_ex_feature_pointer(SGX_CREATE_ENCLAVE_EX_PCL, ex_features, ex_features_p, (void **)&sealed_key) == -1)
    {
        loader.destroy_enclave();
        delete enclave; // The `enclave' object owns the `loader' object.
        return SGX_ERROR_INVALID_PARAMETER;
    }
    if (sealed_key != NULL)
    {
        enclave->set_sealed_key(sealed_key);
    }
    
    // It is accurate to get debug flag from secs
    enclave->set_dbg_flag(!!(loader.get_secs().attributes.flags & SGX_FLAGS_DEBUG));

    debug_info = const_cast<debug_enclave_info_t *>(enclave->get_debug_info());

    enclave->set_extra_debug_info(const_cast<secs_t &>(loader.get_secs()), loader);

    //add enclave to enclave pool before init_enclave because in simualtion
    //mode init_enclave will rely on CEnclavePool to get Enclave instance.
    if (FALSE == CEnclavePool::instance()->add_enclave(enclave))
    {
        loader.destroy_enclave();
        delete enclave;
        return SGX_ERROR_UNEXPECTED;
    }

    std::vector<std::pair<tcs_t *, bool>> tcs_list = loader.get_tcs_list();
    for (unsigned idx = 0; idx < tcs_list.size(); ++idx)
    {
        enclave->add_thread(tcs_list[idx].first, tcs_list[idx].second);
        SE_TRACE(SE_TRACE_DEBUG, "add tcs %p\n", tcs_list[idx].first);
    }
    
    if(debug)
        debug_info->enclave_type |= ET_DEBUG;
    if (!(get_enclave_creator()->use_se_hw()))
        debug_info->enclave_type |= ET_SIM;

    if(debug || !(get_enclave_creator()->use_se_hw()))
    {
        SE_TRACE(SE_TRACE_DEBUG, "Debug enclave. Checking if VTune is profiling or SGX_DBG_OPTIN is set\n");

        __itt_init_ittlib(NULL, __itt_group_none);
        bool isVTuneProfiling;
        if(__itt_get_ittapi_global()->api_initialized && __itt_get_ittapi_global()->lib)
            isVTuneProfiling = true;
        else
            isVTuneProfiling = false;

        bool is_SGX_DBG_OPTIN_set = false;
        is_SGX_DBG_OPTIN_set = is_SGX_DBG_OPTIN_variable_set();
        if (isVTuneProfiling || is_SGX_DBG_OPTIN_set)
        {
            SE_TRACE(SE_TRACE_DEBUG, "VTune is profiling or SGX_DBG_OPTIN is set\n");

            bool thread_updated;
            thread_updated = enclave->update_debug_flag(1);

            if(thread_updated == false)
            {
                SE_TRACE(SE_TRACE_DEBUG, "Failed to update debug OPTIN bit\n");
            }
            else
            {
                SE_TRACE(SE_TRACE_DEBUG, "Updated debug OPTIN bit\n");
            }

            if (isVTuneProfiling)
            {
                uint64_t enclave_start_addr;
                uint64_t enclave_end_addr;
                const char* enclave_path;
                enclave_start_addr = (uint64_t) loader.get_start_addr();
                enclave_end_addr = enclave_start_addr + (uint64_t) metadata->enclave_size -1;

                SE_TRACE(SE_TRACE_DEBUG, "Invoking VTune's module mapping API __itt_module_load \n");
                SE_TRACE(SE_TRACE_DEBUG, "Enclave_start_addr==0x%llx\n", enclave_start_addr);
                SE_TRACE(SE_TRACE_DEBUG, "Enclave_end_addr==0x%llx\n", enclave_end_addr);

                enclave_path = (const char*)file.name;
                SE_TRACE(SE_TRACE_DEBUG, "Enclave_path==%s\n",  enclave_path);
                __itt_module_load((void*)enclave_start_addr, (void*) enclave_end_addr, enclave_path);
            }
        }
        else
        {
            SE_TRACE(SE_TRACE_DEBUG, "VTune is not profiling and SGX_DBG_OPTIN is not set. TCS Debug OPTIN bit not set and API to do module mapping not invoked\n");
        }
    }

    //send debug event to debugger when enclave is debug mode or release mode
    //set struct version
    debug_info->struct_version = enclave->get_debug_info()->struct_version;
    //generate load debug event after EINIT
    generate_enclave_debug_event(URTS_EXCEPTION_POSTINITENCLAVE, debug_info);

    if (get_enclave_creator()->is_EDMM_supported(loader.get_enclave_id()))
    {
        layout_t *layout_start = GET_PTR(layout_t, metadata, metadata->dirs[DIR_LAYOUT].offset);
        layout_t *layout_end = GET_PTR(layout_t, metadata, metadata->dirs[DIR_LAYOUT].offset + metadata->dirs[DIR_LAYOUT].size);
        if (SGX_SUCCESS != (ret = loader.post_init_action(layout_start, layout_end, 0)))
        {
            SE_TRACE(SE_TRACE_ERROR, "trim range error.\n");
            sgx_status_t status = SGX_SUCCESS;
            generate_enclave_debug_event(URTS_EXCEPTION_PREREMOVEENCLAVE, debug_info);
            CEnclavePool::instance()->remove_enclave(loader.get_enclave_id(), status);
            goto fail;
        }
    }

    //call trts to do some initialization
    if(SGX_SUCCESS != (ret = get_enclave_creator()->initialize(loader.get_enclave_id())))
    {
        sgx_status_t status = SGX_SUCCESS;
        generate_enclave_debug_event(URTS_EXCEPTION_PREREMOVEENCLAVE, debug_info);
        CEnclavePool::instance()->remove_enclave(loader.get_enclave_id(), status);
        goto fail;
    }

    if (get_enclave_creator()->is_EDMM_supported(loader.get_enclave_id()))
    {
        
        layout_t *layout_start = GET_PTR(layout_t, metadata, metadata->dirs[DIR_LAYOUT].offset);
        layout_t *layout_end = GET_PTR(layout_t, metadata, metadata->dirs[DIR_LAYOUT].offset + metadata->dirs[DIR_LAYOUT].size);
        if (SGX_SUCCESS != (ret = loader.post_init_action_commit(layout_start, layout_end, 0)))
        {
            SE_TRACE(SE_TRACE_ERROR, "trim page commit error.\n");
            sgx_status_t status = SGX_SUCCESS;
            generate_enclave_debug_event(URTS_EXCEPTION_PREREMOVEENCLAVE, debug_info);
            CEnclavePool::instance()->remove_enclave(loader.get_enclave_id(), status);
            goto fail;
        }
    }

    if(SGX_SUCCESS != (ret = loader.set_memory_protection()))
    {
        sgx_status_t status = SGX_SUCCESS;
        generate_enclave_debug_event(URTS_EXCEPTION_PREREMOVEENCLAVE, debug_info);
        CEnclavePool::instance()->remove_enclave(loader.get_enclave_id(), status);
        goto fail;
    }

    //fill tcs mini pool
    if (get_enclave_creator()->is_EDMM_supported(loader.get_enclave_id()))
    {
        ret = enclave->fill_tcs_mini_pool_fn();
        if (ret != SGX_SUCCESS)
        {
            SE_TRACE(SE_TRACE_ERROR, "fill_tcs_mini_pool error.\n");
            sgx_status_t status = SGX_SUCCESS;
            generate_enclave_debug_event(URTS_EXCEPTION_PREREMOVEENCLAVE, debug_info);
            CEnclavePool::instance()->remove_enclave(loader.get_enclave_id(), status);
            goto fail;
        }
    }
        
    if (get_ex_feature_pointer(SGX_CREATE_ENCLAVE_EX_SWITCHLESS, ex_features, ex_features_p, (void **)&us_config) == -1)
    {
        ret = SGX_ERROR_INVALID_PARAMETER;
        sgx_status_t status = SGX_SUCCESS;
        generate_enclave_debug_event(URTS_EXCEPTION_PREREMOVEENCLAVE, debug_info);
        CEnclavePool::instance()->remove_enclave(loader.get_enclave_id(), status);
        goto fail;

    }
    if (us_config)
    {
         if (SGX_SUCCESS != (ret = enclave->init_uswitchless(us_config)))
        {
            sgx_status_t status = SGX_SUCCESS;
            generate_enclave_debug_event(URTS_EXCEPTION_PREREMOVEENCLAVE, debug_info);
            CEnclavePool::instance()->remove_enclave(loader.get_enclave_id(), status);
            goto fail;
        }

    }

    *enclave_id = loader.get_enclave_id();
    return SGX_SUCCESS;

fail:
    loader.destroy_enclave();
    delete enclave;
    return ret;
}


sgx_status_t _create_enclave_from_buffer_ex(const bool debug, uint8_t *base_addr, uint64_t file_size, se_file_t& file, 
                                            le_prd_css_file_t *prd_css_file, sgx_enclave_id_t *enclave_id, sgx_misc_attribute_t *misc_attr,
                                            const uint32_t ex_features, const void* ex_features_p[32])
{
    unsigned int ret = SGX_SUCCESS;
    sgx_misc_attribute_t sgx_misc_attr;
    metadata_t *metadata = NULL;
    SGXLaunchToken *lc = NULL;
    memset(&sgx_misc_attr, 0, sizeof(sgx_misc_attribute_t));
    sgx_isvfamily_id_t isvf;
    sgx_isvext_prod_id_t isvp;
    memset(&isvf, 0, sizeof(sgx_isvfamily_id_t));
    memset(&isvp, 0, sizeof(sgx_isvext_prod_id_t));
    sgx_kss_config_t* kss_config = NULL;
    void *ex_fp = NULL;
    int res = 0;

    if(NULL == base_addr || NULL == enclave_id)
        return SGX_ERROR_INVALID_PARAMETER;
#ifndef SE_SIM
    ret = validate_platform();
    if(ret != SGX_SUCCESS)
        return (sgx_status_t)ret;
#endif

    PARSER parser(base_addr, file_size);
    if(SGX_SUCCESS != (ret = parser.run_parser()))
    {
        goto clean_return;
    }
    //Make sure HW uRTS won't load simulation enclave and vice verse.
    if(get_enclave_creator()->use_se_hw() != (!parser.get_symbol_rva("g_global_data_sim")))
    {
        SE_TRACE_WARNING("HW and Simulation mode incompatibility detected. The enclave is linked with the incorrect tRTS library.\n");
        ret = SGX_ERROR_MODE_INCOMPATIBLE;
        goto clean_return;
    }
 
    res = get_ex_feature_pointer(SGX_CREATE_ENCLAVE_EX_PCL, ex_features, ex_features_p, &ex_fp);
    if (res == -1)
    {
        ret = SGX_ERROR_INVALID_PARAMETER;
        goto clean_return;

    }
    else if (res == 0 && parser.is_enclave_encrypted() != false)
    {
    
        // If no PCL feature request is input, the enclave should not be encrypted.
        ret = SGX_ERROR_PCL_ENCRYPTED;
        goto clean_return;
    }
    else if (res == 1 && parser.is_enclave_encrypted() != true)
    {
        // If PCL feature is requested, the enclave should be encrypted
        ret = SGX_ERROR_PCL_NOT_ENCRYPTED;
        goto clean_return;
    }

    if(SGX_SUCCESS != (ret = get_metadata(&parser, debug,  &metadata, &sgx_misc_attr)))
    {
        goto clean_return;
    }

    // Check KSS
    if (!(sgx_misc_attr.secs_attr.flags & SGX_FLAGS_KSS))
    {
        // KSS flag is not set, then we should not configure config_id, config_svn, family_id and ext_prod_id
        res = get_ex_feature_pointer(SGX_CREATE_ENCLAVE_EX_KSS, ex_features, ex_features_p, (void **)&kss_config);
        if (res == -1)
        {
            ret = SGX_ERROR_INVALID_PARAMETER;
            goto clean_return;
        }
        else if (res == 1)
        {
            ret = SGX_ERROR_FEATURE_NOT_SUPPORTED;
            goto clean_return;
        }
        if (memcmp(metadata->enclave_css.body.isvext_prod_id, &isvp, sizeof(sgx_isvext_prod_id_t)) ||
            memcmp(metadata->enclave_css.body.isv_family_id, &isvf, sizeof(sgx_isvfamily_id_t))) 
        {
            ret = SGX_ERROR_FEATURE_NOT_SUPPORTED;
            goto clean_return;
        }
    }

    lc = new SGXLaunchToken(&metadata->enclave_css, &sgx_misc_attr.secs_attr, NULL);
#ifndef SE_SIM
    // Only LE allows the prd_css_file
    if(is_le(&metadata->enclave_css) == false && prd_css_file != NULL)
    {
        ret = SGX_ERROR_INVALID_PARAMETER;
        goto clean_return;
    }
#endif

    // init xave global variables for xsave/xrstor
    init_xsave_info();


    //Need to set the whole misc_attr instead of just secs_attr.
    do {
        ret = __create_enclave(parser, base_addr, metadata, file, debug, lc, prd_css_file, enclave_id, misc_attr, ex_features, ex_features_p);
        //SGX_ERROR_ENCLAVE_LOST caused by initializing enclave while power transition occurs
    } while(SGX_ERROR_ENCLAVE_LOST == ret);

    if(SE_ERROR_INVALID_LAUNCH_TOKEN == ret)
        ret = SGX_ERROR_INVALID_LAUNCH_TOKEN;
        
    // The launch token is updated, so the SE_INVALID_MEASUREMENT is only caused by signature.
    if(SE_ERROR_INVALID_MEASUREMENT == ret)
        ret = SGX_ERROR_INVALID_SIGNATURE;

    // The launch token is updated, so the SE_ERROR_INVALID_ISVSVNLE means user needs to update the LE image
    if (SE_ERROR_INVALID_ISVSVNLE == ret)
        ret = SGX_ERROR_UPDATE_NEEDED;

    if(SGX_SUCCESS != ret)
        goto clean_return;


clean_return:
    if(lc != NULL)
        delete lc;
    return (sgx_status_t)ret;
}

sgx_status_t _create_enclave_ex(const bool debug, se_file_handle_t pfile, se_file_t& file, le_prd_css_file_t *prd_css_file,
                                sgx_launch_token_t *launch, int *launch_updated, sgx_enclave_id_t *enclave_id, 
                                sgx_misc_attribute_t *misc_attr, const uint32_t ex_features, const void* ex_features_p[32])
{
    UNUSED(launch);
    UNUSED(launch_updated);

    unsigned int ret = SGX_SUCCESS;
    off_t file_size = 0;
    map_handle_t* mh = NULL;

    mh = map_file(pfile, &file_size);
    if (!mh)
        return SGX_ERROR_OUT_OF_MEMORY;

    ret = _create_enclave_from_buffer_ex(debug, mh->base_addr, (uint64_t)(file_size), file, prd_css_file,
                                         enclave_id, misc_attr, ex_features, ex_features_p);

    unmap_file(mh);
    return (sgx_status_t)ret;
}

sgx_status_t _create_enclave(const bool debug, se_file_handle_t pfile, se_file_t& file, le_prd_css_file_t *prd_css_file, 
                             sgx_launch_token_t *launch, int *launch_updated, sgx_enclave_id_t *enclave_id, sgx_misc_attribute_t *misc_attr) 
{
    return _create_enclave_ex(debug, pfile, file, prd_css_file, launch, launch_updated, enclave_id, misc_attr, 0, NULL);
}


extern "C" sgx_status_t sgx_destroy_enclave(const sgx_enclave_id_t enclave_id)
{
    {
        CEnclave* enclave = CEnclavePool::instance()->ref_enclave(enclave_id);

        if(enclave)
        {
            debug_enclave_info_t *debug_info = const_cast<debug_enclave_info_t *>(enclave->get_debug_info());
            generate_enclave_debug_event(URTS_EXCEPTION_PREREMOVEENCLAVE, debug_info);
            enclave->destroy_uswitchless();
            if (get_enclave_creator()->is_EDMM_supported(enclave->get_enclave_id()))
                enclave->ecall(ECMD_UNINIT_ENCLAVE, NULL, NULL);
            CEnclavePool::instance()->unref_enclave(enclave);
        }
    }

    sgx_status_t status = SGX_SUCCESS;
    CEnclave* enclave = CEnclavePool::instance()->remove_enclave(enclave_id, status);

    if (enclave)
    {
        delete enclave;
    }

    return status;
}
#endif
