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

  
#include "enclave.h"
#include "enclave_creator_hw.h"
#include "edmm_utility.h"
#include "sgx_enclave_common.h"
#include "se_trace.h"
#include "se_page_attr.h"
#include "isgx_user.h"
#include "sig_handler.h"
#include "se_error_internal.h"
#include "se_memcpy.h"
#include "se_atomic.h"
#include "se_detect.h"
#include "cpuid.h"
#include "rts.h"
#include <assert.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdlib.h>

#define POINTER_TO_U64(A) ((__u64)((uintptr_t)(A)))
  
static EnclaveCreatorHW g_enclave_creator_hw;

EnclaveCreator* g_enclave_creator = &g_enclave_creator_hw;
static uint64_t g_eid = 0x1;


EnclaveCreatorHW::EnclaveCreatorHW():
    m_hdevice(-1),
    m_sig_registered(false)
{
    se_mutex_init(&m_sig_mutex);
    memset(&m_enclave_elrange, 0, sizeof(m_enclave_elrange));
}

EnclaveCreatorHW::~EnclaveCreatorHW()
{
    close_device();
}

int EnclaveCreatorHW::error_driver2urts(int driver_error, int err_no)
{
    int ret = SGX_ERROR_UNEXPECTED;\
    
    if(driver_error == -1){
        switch(err_no) {
        case (int)ENOMEM:
            ret = SGX_ERROR_OUT_OF_MEMORY;
            break;
        case (int)EINVAL:
            ret = SGX_ERROR_INVALID_PARAMETER;
            break;
        case (int)ENOSYS:
            ret = SGX_ERROR_FEATURE_NOT_SUPPORTED;
            break;
        default:
            SE_TRACE(SE_TRACE_WARNING, "unexpected errno %#x from driver, might be a driver bug\n", err_no);
            ret = ENCLAVE_UNEXPECTED;
            break;
        }
    }
    else{
        switch(driver_error){
        case SGX_INVALID_ATTRIBUTE:
            ret = SGX_ERROR_INVALID_ATTRIBUTE;
            break;
        case SGX_INVALID_PRIVILEGE:
            ret = SGX_ERROR_SERVICE_INVALID_PRIVILEGE;
            break;
        case SGX_INVALID_MEASUREMENT:
            ret = SE_ERROR_INVALID_MEASUREMENT;
            break;
        case SGX_INVALID_SIG_STRUCT:
        case SGX_INVALID_SIGNATURE:
            ret = SGX_ERROR_INVALID_SIGNATURE;
            break;
        case SGX_INVALID_LICENSE:
            ret = SE_ERROR_INVALID_LAUNCH_TOKEN;
            break;
        case SGX_INVALID_CPUSVN:
            ret = SGX_ERROR_INVALID_CPUSVN;
            break;
        case SGX_INVALID_ISVSVN:
            ret = SGX_ERROR_INVALID_ISVSVN;
            break;
        case SGX_UNMASKED_EVENT:
            ret = SGX_ERROR_DEVICE_BUSY;
            break;
        case (int)SGX_POWER_LOST_ENCLAVE: // [-Wc++11-narrowing]
            ret = SGX_ERROR_ENCLAVE_LOST;
            break;
        case (int)SGX_LE_ROLLBACK:
            ret = SE_ERROR_INVALID_ISVSVNLE;
            break;
        default:
            SE_TRACE(SE_TRACE_WARNING, "unexpected error %#x from driver, should be uRTS/driver bug\n", driver_error);
            ret = SGX_ERROR_UNEXPECTED;
            break;
        }
    }

    return ret;
}

int EnclaveCreatorHW::error_api2urts(uint32_t api_error)
{
    int ret = SGX_ERROR_UNEXPECTED;

    switch(api_error)
    {
     case ENCLAVE_ERROR_SUCCESS:
         ret = SGX_SUCCESS;
         break;
     case ENCLAVE_NOT_SUPPORTED:
         ret = SGX_ERROR_NO_DEVICE;
         break;
     case ENCLAVE_INVALID_SIG_STRUCT:
     case ENCLAVE_INVALID_SIGNATURE:
         ret = SGX_ERROR_INVALID_SIGNATURE;
         break;
     case ENCLAVE_INVALID_ATTRIBUTE:
         ret = SGX_ERROR_INVALID_ATTRIBUTE;
         break;
     case ENCLAVE_NOT_AUTHORIZED:
         ret = SGX_ERROR_SERVICE_INVALID_PRIVILEGE;
         break;
     case ENCLAVE_INVALID_MEASUREMENT:
         ret = SE_ERROR_INVALID_MEASUREMENT;
         break;
     case ENCLAVE_INVALID_ENCLAVE:
         ret = SGX_ERROR_INVALID_ENCLAVE;
         break;
     case ENCLAVE_LOST:
         ret = SGX_ERROR_ENCLAVE_LOST;
         break;
     case ENCLAVE_INVALID_PARAMETER:
         ret = SGX_ERROR_INVALID_PARAMETER;
         break;
     case ENCLAVE_OUT_OF_MEMORY:
         ret = SGX_ERROR_OUT_OF_MEMORY;
         break;
     case ENCLAVE_DEVICE_NO_RESOURCES:
         ret = SGX_ERROR_OUT_OF_EPC;
         break;
     case ENCLAVE_SERVICE_TIMEOUT:
         ret = SGX_ERROR_SERVICE_TIMEOUT;
         break;
     case ENCLAVE_SERVICE_NOT_AVAILABLE:
         ret = SGX_ERROR_SERVICE_UNAVAILABLE;
         break; 
     case ENCLAVE_MEMORY_MAP_FAILURE:
         ret = SGX_ERROR_MEMORY_MAP_FAILURE;
         break;
     default:
         SE_TRACE(SE_TRACE_WARNING, "unexpected error %#x from enclave common api, should be uRTS/driver bug\n", api_error);
         ret = SGX_ERROR_UNEXPECTED;
         break;
     }

     return ret;
}
 
int EnclaveCreatorHW::create_enclave(secs_t *secs, sgx_enclave_id_t *enclave_id, void **start_addr, const uint32_t ex_features, const void* ex_features_p[32])
{
    assert(secs != NULL && enclave_id != NULL && start_addr != NULL);

    enclave_create_sgx_t enclave_create_sgx = {0};
    if (0 != memcpy_s(enclave_create_sgx.secs, SECS_SIZE, secs, SECS_SIZE))
        return SGX_ERROR_UNEXPECTED;

    uint32_t enclave_error = ENCLAVE_ERROR_SUCCESS;
    void* enclave_base = enclave_create_ex(*start_addr, (size_t)secs->size, 0, ENCLAVE_TYPE_SGX2, &enclave_create_sgx, sizeof(enclave_create_sgx_t), ex_features, ex_features_p, &enclave_error);

    if (enclave_error)
        return error_api2urts(enclave_error);

    secs->base = enclave_base;
    *start_addr = enclave_base;
    *enclave_id = se_atomic_inc64(&g_eid);

    return error_api2urts(enclave_error);
}

int EnclaveCreatorHW::add_enclave_page(sgx_enclave_id_t enclave_id, void *src, uint64_t rva, const sec_info_t &sinfo, uint32_t attr)
{
    assert((rva & ((1<<SE_PAGE_SHIFT)-1)) == 0);
    UNUSED(attr);

    uint32_t enclave_error = ENCLAVE_ERROR_SUCCESS;
    uint32_t data_properties = (uint32_t)(sinfo.flags);
    if(!((1<<DoEEXTEND) & attr))
    {
        data_properties |= ENCLAVE_PAGE_UNVALIDATED;
    }
    enclave_load_data((void*)(enclave_id + rva), SE_PAGE_SIZE, src, data_properties, &enclave_error);

    return error_api2urts(enclave_error);
}

int EnclaveCreatorHW::try_init_enclave(sgx_enclave_id_t enclave_id, enclave_css_t *enclave_css, token_t *launch)
{
    UNUSED(launch);

    enclave_init_sgx_t enclave_init_sgx = {0};
    if (0 != memcpy_s(enclave_init_sgx.sigstruct, SIGSTRUCT_SIZE, enclave_css, SIGSTRUCT_SIZE))
        return SGX_ERROR_UNEXPECTED;

    uint32_t enclave_error = ENCLAVE_ERROR_SUCCESS;
    enclave_initialize((void*)enclave_id, &enclave_init_sgx, sizeof(enclave_init_sgx), &enclave_error);

    if (enclave_error)
        return error_api2urts(enclave_error);

    //register signal handler
    se_mutex_lock(&m_sig_mutex);
    if(false == m_sig_registered)
    {
        reg_sig_handler();
        m_sig_registered = true;
    }
    se_mutex_unlock(&m_sig_mutex);

    return SGX_SUCCESS;
}

int EnclaveCreatorHW::destroy_enclave(sgx_enclave_id_t enclave_id, uint64_t enclave_size)
{
    UNUSED(enclave_size);

    uint32_t enclave_error = ENCLAVE_ERROR_SUCCESS;
    enclave_delete((void*)enclave_id, &enclave_error);

    return error_api2urts(enclave_error);
}

bool EnclaveCreatorHW::get_plat_cap(sgx_misc_attribute_t *misc_attr)
{
    // need to update code to support HyperV ECO
    return get_plat_cap_by_cpuid(misc_attr);
}

bool EnclaveCreatorHW::open_device()
{
    LockGuard lock(&m_dev_mutex);

    if(-1 != m_hdevice)
        return true;

    //todo: probably should add an interface to the enclave common loader so that edmm_utility does not need to:
    //  - provide the driver type - the enclave common loader can provide properties
    //  - open the device - if needed, the enclave common loader can share the file handle
    int driver_type = 0;
    if (!::get_driver_type(&driver_type))
    {
        SE_TRACE(SE_TRACE_ERROR, "open_device() - could not get driver typed\n");
        return false;
    }

    if(driver_type == SGX_DRIVER_OUT_OF_TREE)
    {
        return ::open_se_device(driver_type, &m_hdevice);
    }
    
    return true;
    
}

void EnclaveCreatorHW::close_device()
{
    LockGuard lock(&m_dev_mutex);

    ::close_se_device(&m_hdevice);
    m_hdevice = -1;
}

int EnclaveCreatorHW::emodpr(uint64_t addr, uint64_t size, uint64_t flag)
{
    sgx_modification_param params;
    memset(&params, 0 ,sizeof(sgx_modification_param));
    params.range.start_addr = (unsigned long)addr;
    params.range.nr_pages = (unsigned int)(size/SE_PAGE_SIZE);
    params.flags = (unsigned long)flag;

    int ret = ioctl(m_hdevice, SGX_IOC_ENCLAVE_EMODPR, &params);
    if (ret)
    {
        SE_TRACE(SE_TRACE_ERROR, "SGX_IOC_ENCLAVE_EMODPR failed %d\n", errno);
        return error_driver2urts(ret, errno);
    }

    return SGX_SUCCESS;
}
 
int EnclaveCreatorHW::mktcs(uint64_t tcs_addr)
{
    sgx_range params;
    memset(&params, 0 ,sizeof(sgx_range));
    params.start_addr = (unsigned long)tcs_addr;
    params.nr_pages = 1;

    int ret = ioctl(m_hdevice, SGX_IOC_ENCLAVE_MKTCS, &params);
    if (ret)
    {
        SE_TRACE(SE_TRACE_ERROR, "MODIFY_TYPE failed %d\n", errno);
        return error_driver2urts(ret, errno);
    }

    return SGX_SUCCESS;
}
 
int EnclaveCreatorHW::trim_range(uint64_t fromaddr, uint64_t toaddr)
{
    sgx_range params;
    memset(&params, 0 ,sizeof(sgx_range));
    params.start_addr = (unsigned long)fromaddr;
    params.nr_pages = (unsigned int)((toaddr - fromaddr)/SE_PAGE_SIZE);

    int ret= ioctl(m_hdevice, SGX_IOC_ENCLAVE_TRIM, &params);
    if (ret)
    {
        SE_TRACE(SE_TRACE_ERROR, "SGX_IOC_ENCLAVE_TRIM failed %d\n", errno);
        return error_driver2urts(ret, errno);
    }

    return SGX_SUCCESS;

}
 
int EnclaveCreatorHW::trim_accept(uint64_t addr)
{
    sgx_range params;
    memset(&params, 0 ,sizeof(sgx_range));
    params.start_addr = (unsigned long)addr;
    params.nr_pages = 1;

    int ret = ioctl(m_hdevice, SGX_IOC_ENCLAVE_NOTIFY_ACCEPT, &params);


    if (ret)
    {
        SE_TRACE(SE_TRACE_ERROR, "TRIM_RANGE_COMMIT failed %d\n", errno);
        return error_driver2urts(ret, errno);
    }

    return SGX_SUCCESS;
}
 
int EnclaveCreatorHW::remove_range(uint64_t fromaddr, uint64_t numpages)
{
    int ret = -1;
    uint64_t i;
    unsigned long start;

    for (i = 0; i < numpages; i++)
    {
        start = (unsigned long)fromaddr + (unsigned long)(i << SE_PAGE_SHIFT);
        ret = ioctl(m_hdevice, SGX_IOC_ENCLAVE_PAGE_REMOVE, &start);
        if (ret)
        {
            SE_TRACE(SE_TRACE_ERROR, "PAGE_REMOVE failed %d\n", errno);
            return error_driver2urts(ret, errno);
        }
    }

    return SGX_SUCCESS;
}
 
//EDMM is supported if and only if all of the following requirements are met:
//1. We operate in HW mode
//2. CPU has EDMM support
//3. Driver has EDMM support
//4. Both the uRTS version and enclave (metadata) version are higher than 1.5
bool EnclaveCreatorHW::is_EDMM_supported(sgx_enclave_id_t enclave_id)
{
    bool supported = false, driver_supported = false, cpu_edmm = false;

    CEnclave *enclave = CEnclavePool::instance()->get_enclave(enclave_id);
    if (enclave == NULL)
        return false;

    cpu_edmm = is_cpu_support_edmm();
    driver_supported = is_driver_compatible();

    //return value of get_enclave_version() considers the version of uRTS and enclave metadata
    supported = use_se_hw() && cpu_edmm && driver_supported && (enclave->get_enclave_version() >= SDK_VERSION_2_0);

    return supported;
}

bool EnclaveCreatorHW::is_driver_compatible()
{
    open_device();
    return is_driver_support_edmm(m_hdevice);
}



