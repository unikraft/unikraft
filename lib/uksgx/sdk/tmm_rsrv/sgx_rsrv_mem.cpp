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


#include "mm_vrd.h"
#include "sgx_rsrv_mem_mngr.h"
#include "mm_vrd_util.h"
#include "util.h"
#include "global_data.h"
#include "trts_inst.h"
#include "trts_internal.h"
#include "trts_util.h"
#include "arch.h"
#include <sgx_trts.h>
#include <string.h>
#include <errno.h>

#define SGX_PAGE_NOACCESS          0x01     // -
#define SGX_PAGE_READONLY          0x02     // R
#define SGX_PAGE_READWRITE         0x04     // RW
#define SGX_PAGE_EXECUTE_READ      0x20     // RX
#define SGX_PAGE_EXECUTE_READWRITE 0x40     // RWX

extern void *rsrv_mem_base;
extern size_t rsrv_mem_size;
extern size_t rsrv_mem_min_size;
extern size_t g_peak_rsrv_mem_committed;

static size_t rsrv_mem_committed = 0;

static sgx_status_t tprotect_internal(size_t start, size_t size, uint64_t perms);


static int init_rsrv_mem_vrd()
{
    sgx_status_t sgx_ret = SGX_ERROR_UNEXPECTED;
    // Initialize the vrdl once.
    if (!init_vrdl())
    {
        return ENOMEM;
    }
    // Protect pages other than rsrv_mem region
    vrd_t * pvrd = insert_vrd((size_t)get_enclave_base(),
                              (size_t)g_global_data.rsrv_offset,
                              SGX_PAGE_NOACCESS, VRD_STATE_LOCKED, VRD_PT_REG, &sgx_ret);
    if(pvrd == NULL)
    {
        if(sgx_ret == SGX_ERROR_OUT_OF_MEMORY)
            return ENOMEM;
        else 
            return EINVAL;
    }
    return 0;
}

sgx_status_t sgx_get_rsrv_mem_info(void ** start_addr, size_t * max_size)
{
    if(start_addr == NULL && max_size == NULL)
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    if(start_addr != NULL)
    {
        *start_addr = rsrv_mem_base;
    }
    if (max_size != NULL)
    {
        *max_size = rsrv_mem_size;
    }
    return SGX_SUCCESS;
}

void * sgx_alloc_rsrv_mem_ex(void *desired_addr, size_t length)
{
    static volatile bool g_first_alloc = true;
    vrd_t * rsrv_vrd = NULL;
    sgx_status_t sgx_ret = SGX_ERROR_UNEXPECTED;
    // Do not allow to allocate memory if  no reserved memory is configured
    if (0 == rsrv_mem_size)
    {
        errno = EPERM;
        return NULL;
    }
    // Sanity checks
    if(!IS_PAGE_ALIGNED(desired_addr) || length == 0 || !IS_PAGE_ALIGNED(length) || length > rsrv_mem_size)
    {
        errno = EINVAL;
        return NULL;
    }
    if(desired_addr != NULL)
    {
        size_t end = (size_t)desired_addr + length - 1;
        if((size_t)desired_addr > end || length > end || end > (size_t)rsrv_mem_base + rsrv_mem_size - 1 ||
           (size_t)desired_addr < (size_t)rsrv_mem_base)
        {
            errno = EINVAL;
            return NULL;
        }
    }

    sgx_thread_mutex_lock(&g_vrdl_mutex);
    if(unlikely(g_first_alloc))
    {
        int ret = init_rsrv_mem_vrd();
        if(ret != 0)
        {
            sgx_thread_mutex_unlock(&g_vrdl_mutex);
            errno = ret;
            return NULL;
        }
        g_first_alloc = false;
    }

    // Allocate memory from reserved memory region
    rsrv_vrd = insert_vrd((size_t)desired_addr, length,
                          (!EDMM_supported && g_global_data.rsrv_executable ) ? SGX_PAGE_EXECUTE_READWRITE : SGX_PAGE_READWRITE,
                          VRD_STATE_COMMITTED,
                          VRD_PT_REG, &sgx_ret);
    if (NULL == rsrv_vrd)
    {
        sgx_thread_mutex_unlock(&g_vrdl_mutex);
        errno = EINVAL;
        if(sgx_ret == SGX_ERROR_OUT_OF_MEMORY)
            errno = ENOMEM;
        return NULL;
    }
    // Actually insert_vrd() confirms the allocated memory is in reserved memory region.
    // Below is just a defence in depth.
    if(rsrv_vrd->start_addr > SIZE_MAX - length || 
       (rsrv_vrd->start_addr + length > (size_t)rsrv_mem_base + rsrv_mem_size))
    {
        remove_vrd(rsrv_vrd);
        sgx_thread_mutex_unlock(&g_vrdl_mutex);
        errno = ENOMEM;
        return NULL; 
    }

    if(rsrv_vrd->start_addr + length > (size_t)rsrv_mem_base + rsrv_mem_committed)
    {
        size_t prev_rsrv_mem_committed = rsrv_mem_committed;
        void * start_addr = NULL;
        size_t size = 0;
        size_t offset = (rsrv_vrd->start_addr + length) - ((size_t)rsrv_mem_base + rsrv_mem_committed);
        rsrv_mem_committed += ROUND_TO(offset, SE_PAGE_SIZE);
        if(EDMM_supported && rsrv_vrd->start_addr + length > (size_t)rsrv_mem_base + rsrv_mem_min_size)
        {
            // Need to apply pages
            if(prev_rsrv_mem_committed > rsrv_mem_min_size)
            {
                start_addr = (void *)((size_t)rsrv_mem_base + prev_rsrv_mem_committed);
                size = ROUND_TO(offset, SE_PAGE_SIZE);
            }
            else
            {
                start_addr = (void *)((size_t)rsrv_mem_base + rsrv_mem_min_size);
                size = rsrv_mem_committed - rsrv_mem_min_size;
            }
            // EACCEPT the new pages
            int ret = apply_EPC_pages(start_addr, size >> SE_PAGE_SHIFT);
            if(ret != 0)
            {
                rsrv_mem_committed = prev_rsrv_mem_committed;
                remove_vrd(rsrv_vrd);
                sgx_thread_mutex_unlock(&g_vrdl_mutex);
                errno = ENOMEM;
                return NULL;
            }
        }
    }
    
    g_peak_rsrv_mem_committed = g_peak_rsrv_mem_committed < rsrv_mem_committed ? rsrv_mem_committed : g_peak_rsrv_mem_committed;

    // Record the start_addr before combine_vrds()    
    void* start_addr = (void *) rsrv_vrd->start_addr;
    combine_vrds((size_t)rsrv_vrd->start_addr, (size_t)rsrv_vrd->start_addr + length - 1);
    sgx_thread_mutex_unlock(&g_vrdl_mutex);
    return (void *)start_addr;
}

/*
 * sgx_alloc_rsrv_mem
 * Reserves a range of EPC memory from the reserved memory area.
 * @return Pointer to the new area on success, otherwise NULL.
 */
void * sgx_alloc_rsrv_mem(size_t length)
{
    return sgx_alloc_rsrv_mem_ex(NULL, length);
}

/*
 * sgx_free_rsrv_mem
 * Frees a range of EPC memory from the reserved memory area.
 * @return 0 on success, otherwise -1.
 */
int sgx_free_rsrv_mem(void * addr, size_t length)
{
    bool brc = false;
    vrd_t * vrd = NULL;
    size_t  end_addr = (size_t)addr + length - 1;

    // Sanity checks
    if (0 == rsrv_mem_size || addr == 0 || !IS_PAGE_ALIGNED(addr) ||
       (length == 0) || !IS_PAGE_ALIGNED(length) ||
       !sgx_is_within_enclave(addr, length))
    {
        errno = EINVAL;
        return -1;
    }
    if((size_t)addr < (size_t)rsrv_mem_base || (size_t)addr > (SIZE_MAX - length) ||
       (size_t)addr + length > (size_t)rsrv_mem_base + rsrv_mem_size)
    {
        errno = EINVAL;
        return -1;
    }

    // Verify mapping exists
    sgx_thread_mutex_lock(&g_vrdl_mutex);
    vrd = find_vrd((size_t)addr);
    if (NULL == vrd)
    {
        sgx_thread_mutex_unlock(&g_vrdl_mutex);
        errno = EINVAL;
        return -1;
    }
    brc = split_vrds_if_needed((size_t)addr, end_addr);
    if (true == brc)
    {
        vrd = find_vrd((size_t)addr);
        if (NULL == vrd)
        {
            // Shouldn't get here because we found a VRD above
            sgx_thread_mutex_unlock(&g_vrdl_mutex);
            abort();
        }
    }

    if(EDMM_supported)
    {
        // Before free the vrd, we change back the page permission to RW if necessary
        if(tprotect_internal((size_t)addr, length, SI_FLAG_R | SI_FLAG_W) != SGX_SUCCESS)
        {
            sgx_thread_mutex_unlock(&g_vrdl_mutex);
            return -1; 
        }
    }

    // Remove the mapping
    free_vrds((size_t)addr, end_addr);
    sgx_thread_mutex_unlock(&g_vrdl_mutex);

    return 0;
}



#include "global_data.h"
#include "trts_emodpr.h"

static sgx_status_t tprotect_internal(size_t start, size_t size, si_flags_t perms)
{
    sgx_status_t ret = SGX_SUCCESS;

    //Error return if start or size is not page-aligned or size is zero.
    if (!IS_PAGE_ALIGNED(start) || (size == 0) || !IS_PAGE_ALIGNED(size))
        return SGX_ERROR_INVALID_PARAMETER;
    
    bool pr_needed = false, pe_needed = false;
    uint32_t target_prot = convert_si_flags_to_protect(perms);

    // Before free the vrd, we change back the page permission if necessary
    if (!check_vrds_pr_pe_needed(start, start + size, target_prot, &pr_needed, &pe_needed))
    {
        return SGX_ERROR_UNEXPECTED;
    }
    
    // EMODPE/EACCEPT requires OS level R permission for the page. Therefore,
    // If target permission is NONE, we should change the OS level permission to NONE after EACCEPT
    // If original permission is NONE, we should change the OS level permission before EMODPE
    if(pr_needed || pe_needed)
    {
        // Ocall to EMODPR if target perm is not RWX and mprotect() if target perm is not NONE
        ret = change_permissions_ocall(start, size, perms, EDMM_MODPR);
        if (ret != SGX_SUCCESS)
            abort();
    }
    si_flags_t sf = perms|SI_FLAG_PR|SI_FLAG_REG;

    if(pe_needed)
    {
        if(emodpe_pages((void *)start, size / SE_PAGE_SIZE, sf))
            abort();
    }
    if(pr_needed && ((perms & (SI_FLAG_W|SI_FLAG_X)) != (SI_FLAG_W|SI_FLAG_X)))
    {
    	// If the target permission to set is RWX, no EMODPR, hence no EACCEPT.
        if(accept_modified_pages((void *)start, size / SE_PAGE_SIZE, sf))
            abort();
    }
    if( pr_needed && perms == SI_FLAG_NONE )
    {
        // If the target permission is NONE, ocall to mprotect() to change the OS permission
        ret = change_permissions_ocall(start, size, perms, EDMM_MPROTECT);
        if (ret != SGX_SUCCESS)
            abort();
    } 
    return ret;
}


sgx_status_t sgx_tprotect_rsrv_mem(void *addr, size_t len, int prot)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    // The operation is only allowed when EDMM is enabled and reserved memory is configured
    if(!EDMM_supported)
    {
        uint32_t cur_prot=0;
        get_vrds_perms((size_t)addr, &cur_prot);
        //The target address's possible priority is SGX_PAGE_EXECUTE_READWRITE or SGX_PAGE_READWRITE
        if(SGX_PAGE_NOACCESS == cur_prot || 
                (SGX_PAGE_READWRITE == cur_prot && (prot &  SGX_PROT_EXEC))) /*Current is read and write, but target exist execute*/
            return SGX_ERROR_INVALID_PARAMETER;
        //Current is read, write & execute, so it's always success.
        return SGX_SUCCESS;
    }
    if(!sgx_is_within_enclave(addr, len))
        return SGX_ERROR_INVALID_PARAMETER;
    
    if(g_sdk_version < SDK_VERSION_2_3)
    {
        // NONE support requires uRTS changes. Therefore, if the enclave is built with update SDK and loaded with old uRTS, 
        // we return an error code indicating that uRTS should be updated.
        return SGX_ERROR_UPDATE_NEEDED;
    } 
    if(0 == rsrv_mem_size || (size_t)addr < (size_t)rsrv_mem_base ||
       !IS_PAGE_ALIGNED(addr) || len < SE_PAGE_SIZE || !IS_PAGE_ALIGNED(len))
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    if(((!(prot & SGX_PROT_READ) && (prot & (SGX_PROT_WRITE | SGX_PROT_EXEC))) ||
        (prot & ~(SGX_PROT_READ | SGX_PROT_WRITE | SGX_PROT_EXEC))))
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    uint64_t perms = SI_FLAG_NONE;
    if((prot & SGX_PROT_EXEC) == SGX_PROT_EXEC)
    {
        perms |= SI_FLAG_X;
    }
    if((prot & SGX_PROT_READ) == SGX_PROT_READ)
    {
        perms |= SI_FLAG_R;
    }
    if((prot & SGX_PROT_WRITE) == SGX_PROT_WRITE)
    {
        perms |= SI_FLAG_W;
    }

    uint32_t protect = convert_si_flags_to_protect(perms);

    size_t end_addr = (size_t)addr + len - 1;
    size_t tmp_addr = (size_t)addr;

    vrd_t *vrd = NULL;
    bool brc = false;

    sgx_thread_mutex_lock(&g_vrdl_mutex);

    vrd = find_vrd((size_t)addr);
    if(vrd == NULL)
    {
        ret = SGX_ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    // Break VRDs into chunks that can be modified below, if requested
    // operation works on partial VRD address ranges.
    brc = split_vrds_if_needed((size_t)addr, end_addr);
    if(brc == true)
    {
        vrd = find_vrd((size_t)addr);
        if(NULL == vrd)
        {
            ret = SGX_ERROR_UNEXPECTED;
            goto cleanup;
        }
    }
    else
    {
        ret = SGX_ERROR_OUT_OF_MEMORY;
        goto cleanup;
    }

    if (!check_vrds_pagetype((size_t)addr, end_addr, VRD_PT_REG))
    {
        ret = SGX_ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    ret = tprotect_internal((size_t)addr, len, perms);
    if(ret != SGX_SUCCESS)
    {
        goto cleanup;
    }

    // Set new page permissions in vrds
    tmp_addr = (size_t)addr;
    while (tmp_addr < end_addr)
    {
        vrd = find_vrd((size_t)tmp_addr);
        if (!vrd)
        {
            ret = SGX_ERROR_UNEXPECTED;
            goto cleanup;
        }
        vrd->perms = protect;
        // We've verified all addrs in this VRD, increment
        // to next unverified addr
        tmp_addr = vrd->start_addr + vrd->size;
    }

    combine_vrds((size_t)addr, end_addr);
    ret = SGX_SUCCESS;

cleanup:
    sgx_thread_mutex_unlock(&g_vrdl_mutex);
    return ret;
}
