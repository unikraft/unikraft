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

#ifndef _MM_VRD_H_
#define _MM_VRD_H_

#include <stddef.h>         // for size_t
#include <stdbool.h>
#include "se_types.h"
#include "sgx_thread.h"
#include "sgx_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SGX_MEM_COMMIT_IMMEDIATE 0x0800
#define SGX_MEM_RESERVE          0x2000
#define SGX_MEM_DECOMMIT         0x4000
#define SGX_MEM_RELEASE          0x8000

#define SGX_PAGE_NOACCESS          0x01     // -
#define SGX_PAGE_READONLY          0x02     // R
#define SGX_PAGE_READWRITE         0x04     // RW
#define SGX_PAGE_EXECUTE_READ      0x20     // RX
#define SGX_PAGE_EXECUTE_READWRITE 0x40     // RWX

#ifndef SIZE_T
#define SIZE_T size_t
#endif

/* Note:
 * Pages of type PT_REG can be changed to PT_TCS.
 * Pages of type PT_REG or PT_TCS can be change to type PT_TRIM.
 */
#define SGX_PAGE_ENCLAVE_THREAD_CONTROL     0x80000000  // Set page type to SI_FLAG_TCS
#define SGX_PAGE_ENCLAVE_DECOMMIT           0x10000000  // Set page type to SI_FLAG_TRIM

extern sgx_thread_mutex_t g_vrdl_mutex;

typedef struct vrd_t {
    size_t      start_addr;
    size_t      size;        // bytes
    uint32_t    flags;       // 0 unless usage limits (don't allow free/ perm change etc)
    uint32_t    state;       // VRD_STATE_FREE, VRD_STATE_RESERVED, VRD_STATE_COMMITTED
    uint32_t    perms;       // SGX_PAGE_NOACCESS, READONLY, READWRITE, EXECUTE_READ, EXECUTE_READWRITE
    uint32_t    page_type;   // VRD_PT_REG, VRD_PT_TCS, VRD_PT_TRIM
    vrd_t*      next;        // next in double linked list
    vrd_t*      prev;        // prev in double linked list
} vrd_t;

#define VRD_STATE_FREE              0       // On the free list
#define VRD_STATE_RESERVED          1       // Enclave reserved memory, manageable by sgx_virtual_alloc/free
#define VRD_STATE_RESERVED_SYSTEM   2       // Enclave reserved memory, user can't touch with sgx_virtual_alloc/free
#define VRD_STATE_COMMITTED         3       // Enclave committed memory, manageable by sgx_virtual_alloc/free
#define VRD_STATE_COMMITTED_SYSTEM  4       // Enclave committed memory, user can't touch with sgx_virtual_alloc/free
                                            // but CAN change page permissions on
#define VRD_STATE_LOCKED            5       // This VRD is locked down at enclave init time and memory state won't be updated

#define VRD_PT_NONE     0
#define VRD_PT_REG      1
#define VRD_PT_TCS      2
#define VRD_PT_TRIM     3

/*
 * NOTE: All methods here should be called while holding the g_vrdl_mutex!
 */

 /*
  *  VRD subsystem initialization functions
  */
bool init_vrdl();
bool get_vrd_rwx(size_t* addr, size_t* size);

/* find_vrd()
 *      return ptr to VRD in VRDL that contains the addr
 */
vrd_t* find_vrd(const size_t vaddr);

/* insert_vrd()
 *      return ptr to VRD or NULL if failure
 *      error is optional and will return additional failure codes if passed in
 */
vrd_t* insert_vrd(size_t start_addr, size_t size, uint32_t perms, uint32_t vrd_state, uint32_t page_type, sgx_status_t* error);

/* remove_vrd()
 *      vrd is removed from VRDL and returned to free list
 */
bool remove_vrd(vrd_t* vrd);

/* split_vrds_if_needed()
 *      Prepare the VRD list to handle a state change to the address range by
 *      splitting VRDs that would see partial VRD changes.
 */
bool split_vrds_if_needed(size_t start_addr, size_t end_addr);

/* combine_vrds()
 *      Combine VRDs with contiguous addresses *IF* they are now the same. The
 *      addresses passed in indicate a range of addresses recently updated that
 *      indicate which VRDs may be able to be merged.
 */
void combine_vrds(size_t start_addr, size_t end_addr);

/*
 * Helper functions that apply address range operations across multiple vrds
 */

bool check_vrds_states(size_t start_addr, size_t end_addr, uint32_t state_0, uint32_t state_1);
bool check_vrds_state(size_t start_addr, size_t end_addr, uint32_t state);
bool check_vrds_not_state(size_t start_addr, size_t end_addr, uint32_t state);
bool check_vrds_pagetype(size_t start_addr, size_t end_addr, uint32_t pt);
bool check_vrds_pagetypes(size_t start_addr, size_t end_addr, uint32_t pt_0, uint32_t pt_1);
bool check_vrds_pr_pe_needed(size_t start_addr, size_t end_addr, uint32_t new_perms, bool* pr_needed, bool* pe_needed);
bool set_vrds_state(size_t start_addr, size_t end_addr, uint32_t new_state);
bool set_vrds_perms(size_t start_addr, size_t end_addr, uint32_t new_perms);
void get_vrds_perms(size_t start_addr, uint32_t* perms);
bool free_vrds(size_t start_addr, size_t end_addr);

#ifdef __cplusplus
}
#endif

#endif

