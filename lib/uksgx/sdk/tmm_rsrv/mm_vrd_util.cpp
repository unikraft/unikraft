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

#include <stdint.h>
#include <stdlib.h>         // abort()
#include "arch.h"
#include "util.h"
#include "mm_vrd.h"
#include "trts_inst.h"      // do_eaccept(), do_emodpe()
#include "string.h"         // memset()
#include "mm_vrd_util.h"

//  accept_modified_pages()
//      EACCEPT n_pages emodpt/emodpr-ed pages, starting @ addr
//
//  Return values:
//    0 - success
//    1 - invalid alignment of params
//
int accept_modified_pages(void* addr, size_t n_pages, si_flags_t si_flags)
{
    size_t t_addr = (size_t)addr;
    SE_DECLSPEC_ALIGN(sizeof(sec_info_t)) sec_info_t secinfo;

    if (!addr || !IS_PAGE_ALIGNED(addr) || !n_pages) {
        return 1;
    }

    memset(&secinfo, 0, sizeof(secinfo));
    secinfo.flags = si_flags;

    for (unsigned int i = 0; i < n_pages; i++) {
        do_eaccept(&secinfo, t_addr);
        t_addr = t_addr + SE_PAGE_SIZE;
    }

    return 0;
}

//  emodpe_pages()
//      EMODPE n_pages pages, starting @ addr
//
//  Return values:
//    0 - success
//    1 - invalid alignment of params
//
int emodpe_pages(void* addr, size_t n_pages, si_flags_t si_flags)
{
    size_t t_addr = (size_t)addr;
    SE_DECLSPEC_ALIGN(sizeof(sec_info_t)) sec_info_t secinfo;

    if (!addr || !IS_PAGE_ALIGNED(addr) || !n_pages)
        return 1;

    memset(&secinfo, 0, sizeof(secinfo));
    secinfo.flags = si_flags;

    for (unsigned int i = 0; i < n_pages; i++) {
        do_emodpe(&secinfo, t_addr);
        t_addr = t_addr + SE_PAGE_SIZE;
    }

    return 0;
}

//  accept_pending_pages(void *addr, size_t n_pages)
//      EACCEPT n_pages EAUGed pages
//
//  Return values:
//    0 - success
//    1 - invalid alignment of params
//
int accept_pending_pages(void* addr, size_t n_pages)
{
    // 1. Construct a SECINFO for: PT_REG + RW + Pending
    // 2. do_eaccept(const sec_info_t *secinfo, size_t page_addr);

    size_t t_addr = (size_t)addr;
    SE_DECLSPEC_ALIGN(sizeof(sec_info_t)) sec_info_t secinfo;

    if (!addr || !IS_PAGE_ALIGNED(addr) || !n_pages)
        return 1;

    // EACCEPT after EAUG is always RW | PT_REG | PENDING
    memset(&secinfo, 0, sizeof(secinfo));
    secinfo.flags = SI_FLAGS_RW;

    for (unsigned int i = 0; i < n_pages; i++) {
        do_eaccept(&secinfo, t_addr); // aborts() on error
        t_addr = t_addr + SE_PAGE_SIZE;
    }

    return 0;
}


si_flags_t convert_protect_to_si_flags(uint32_t protect, bool modified, bool pending, bool perm_reduced)
{
    si_flags_t siFlags = 0;

    switch (protect)
    {
    case SGX_PAGE_NOACCESS:
        siFlags = 0 | SI_FLAG_REG;
        break;
    case SGX_PAGE_READONLY:
        siFlags = SI_FLAG_R | SI_FLAG_REG;
        break;
    case SGX_PAGE_READWRITE:
        siFlags = SI_FLAG_R | SI_FLAG_W | SI_FLAG_REG;
        break;
    case SGX_PAGE_EXECUTE_READ:
        siFlags = SI_FLAG_R | SI_FLAG_X | SI_FLAG_REG;
        break;
    case SGX_PAGE_EXECUTE_READWRITE:
        siFlags = SI_FLAG_R | SI_FLAG_W | SI_FLAG_X | SI_FLAG_REG;
        break;
    case SGX_PAGE_ENCLAVE_DECOMMIT:
        siFlags = SI_FLAG_TRIM;
        break;
    case SGX_PAGE_ENCLAVE_THREAD_CONTROL:
        siFlags = SI_FLAG_TCS;
        break;
    default:
        break;
    }

    if (modified)
        siFlags |= (si_flags_t)SI_FLAG_MODIFIED;
    if (pending)
        siFlags |= (si_flags_t)SI_FLAG_PENDING;
    if (perm_reduced)
        siFlags |= (si_flags_t)SI_FLAG_PR;

    return siFlags;
}

uint32_t convert_si_flags_to_protect(si_flags_t si)
{
    uint32_t protect = 0;
    switch(si & ((SI_FLAG_X | SI_FLAG_R | SI_FLAG_W)))
    {
        case SI_FLAG_R:
            protect = SGX_PAGE_READONLY;
            break;
        case SI_FLAG_R | SI_FLAG_W:
            protect = SGX_PAGE_READWRITE;
            break;
        case SI_FLAG_R | SI_FLAG_X:
            protect = SGX_PAGE_EXECUTE_READ;
            break;
        case SI_FLAG_R | SI_FLAG_W | SI_FLAG_X:
            protect = SGX_PAGE_EXECUTE_READWRITE;
            break;
        case SI_FLAG_NONE:
            protect = SGX_PAGE_NOACCESS;
            break;
        default:
            break;

    }
    return protect;
}


