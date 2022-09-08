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

#include "stdlib.h"
#include "string.h"                 // memset
#include "mm_vrd.h"
#include "global_data.h"
#include "sgx_thread.h"
#include "mm_vrd_util.h"
#include "trts_util.h"
#include "trts_internal.h"

extern void *rsrv_mem_base;
extern size_t rsrv_mem_size;

sgx_thread_mutex_t  g_vrdl_mutex = SGX_THREAD_MUTEX_INITIALIZER;
static vrd_t*       g_vrdl = 0;                  // ptr to enclave memory mgmt str
static vrd_t*       g_vrdl_free_list = 0;        // free list of vrds for use by tedmm vrd subsystem
static uint32_t     g_vrdl_free_list_count = 0;  // free list of vrds for use by tedmm vrd subsystem

#define VRDL_FREELIST_LOW_THRESHOLD     4       // Replenish VRD free list after we fall below 4
#define VRDL_FREELIST_ALLOC_TARGET      16      // Replenish VRD free list with up to 16 VRDs on allocs
#define VRDL_FREELIST_HIGH_THRESHOLD    32      // Release VRDs from free list beyond this mark

static size_t vrd_alloc_addr(size_t size);
static void copy_vrd(vrd_t* vrd_copy, const vrd_t* vrd);
inline static bool addr_in_vrd(const size_t vaddr, const vrd_t* vrd);
inline static bool addrs_in_vrd(size_t vaddr_start, size_t vaddr_end, const vrd_t* vrd);
static bool free_list_put_vrd(vrd_t* vrd);
static vrd_t* free_list_get_vrd();
static void free_list_check_level();


/*********************************************************************
 * VRD subsystem initialization functions
 *********************************************************************/

/* init_vrdl()
 *  Initialize the VRD subsystem. g_vrdl, free_list, etc
 */
bool init_vrdl()
{
    if (g_vrdl || g_vrdl_free_list) {
        // sanity check -- only initialize once
        return false;
    }
    // Populate initial free_list of VRDs from heap; for first release, simple heap mallocs
    g_vrdl_free_list_count = 0;
    vrd_t* vrd = 0;
    while (g_vrdl_free_list_count < VRDL_FREELIST_HIGH_THRESHOLD)
    {
        vrd = (vrd_t*)malloc(sizeof(vrd_t));
        if (!free_list_put_vrd(vrd))
        {
            // On error, undo changes and return to clean state
            while (g_vrdl_free_list) {
                vrd = g_vrdl_free_list;
                g_vrdl_free_list = g_vrdl_free_list->next;
                free(vrd);
            }
            return false;
        }
    }
    return true;
}

/* get_vrd_rwx()
 *  Query VRD list for RWX sections at initialization in order to
 *  identify RWX sections we need to convert to RX. virtual_protect
 *  can modify the VRD list, so each call to this is treated as a new
 *  query.
 */
bool get_vrd_rwx(size_t* addr, size_t* size)
{
    vrd_t* current = g_vrdl;

    if (NULL == addr || NULL == size) {
        return false;
    }

    while (current)
    {
        if (current->state == VRD_STATE_COMMITTED_SYSTEM &&
            current->perms == SGX_PAGE_EXECUTE_READWRITE) {
            *addr = current->start_addr;
            *size = current->size;
            return true;
        }
        current = current->next;
    }
    return false;
}


/*********************************************************************
* VRD subsystem APIs
*********************************************************************/

vrd_t* find_vrd(const size_t vaddr)
{
    size_t heap_start = (size_t)get_heap_base();
    size_t heap_end = heap_start + get_heap_size() - 1;

    vrd_t* current = g_vrdl;

    while (current)
    {
        // sanity check on the VRDL; abort if corrupted
        if ((size_t)current < heap_start || (size_t)current > heap_end - sizeof(vrd_t)) {
            abort();
        }

        if (addr_in_vrd(vaddr, current)) {
            break;
        }
        current = current->next;
    }

    return current;
}

vrd_t* insert_vrd(size_t start_addr, size_t size, uint32_t perms, uint32_t vrd_state, uint32_t page_type, sgx_status_t* error)
{
    size_t encl_start = (size_t)get_enclave_base();
    size_t encl_end = (size_t)rsrv_mem_base + rsrv_mem_size - 1;
    size_t addr = start_addr;
    if (!addr)
        addr = vrd_alloc_addr(size);
    size_t end_addr = addr + size - 1;

    if (!addr || addr < encl_start || end_addr > encl_end || addr > end_addr) {
        tedmm_set_error(error, SGX_ERROR_INVALID_PARAMETER);
        return NULL;
    }

    vrd_t* vrd = free_list_get_vrd();
    if (!vrd) {
        // Free list is empty, try to refill it if there's heap space avail
        free_list_check_level();
        vrd = free_list_get_vrd();
    }
    if (!vrd) {
        tedmm_set_error(error, SGX_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    vrd->flags = 0;
    vrd->size = size;
    vrd->state = vrd_state;
    vrd->page_type = page_type;
    vrd->perms = perms;
    vrd->start_addr = addr;

    vrd_t* current = g_vrdl;
    vrd_t* prev = NULL;
    bool bResult = false;

    while (current)
    {
        // address range can't overlap with an existing vrd
        if (addrs_in_vrd(vrd->start_addr, end_addr, current)) {
            tedmm_set_error(error, SGX_ERROR_INVALID_PARAMETER);
            goto out;
        }

        // Insert before current if appropriate
        if ((vrd->start_addr + vrd->size) < vrd->start_addr) {
            // We built the VRD list and somehow it contains
            // invalid information
            abort();
        }
        if (vrd->start_addr + vrd->size - 1 < current->start_addr)
        {
            // sanity check pointers before using
            if (current->prev != NULL && current->prev->next != current) {
                bResult = false;
                goto out;
            }
            // Set up pointers for new node
            vrd->prev = current->prev;
            vrd->next = current;
            // Update existing list.
            if (current->prev != NULL)
                current->prev->next = vrd;
            current->prev = vrd;
            bResult = true;
            goto out;
        }
        else // or move current to next
        {
            prev = current;
            current = current->next;
        }
    }

    // Or insert at the end if this address range is bigger than existing
    if (!g_vrdl) // Empty list
    {
        g_vrdl = vrd;
        vrd->prev = NULL;
        vrd->next = NULL;
    }
    else
    {
        vrd->next = NULL;
        vrd->prev = prev;
        prev->next = vrd;
    }
    bResult = true;

out:
    if (!bResult) {
        free_list_put_vrd(vrd);
        return NULL;
    }

    return vrd;
}


/* remove_vrd()
      vrd is removed and returned to free list
      fails only if vrd is invalid
 */
bool remove_vrd(vrd_t* vrd)
{
    if (!vrd)
        return false;

    // Sanity check pointers for corruption; if there is a prev or next it should
    // link back to us or our list is trashed
    if ((vrd->prev && vrd->prev->next != vrd) ||
        (vrd->next && vrd->next->prev != vrd)) {
        abort();
    }

    if (!vrd->prev) // first in list
        g_vrdl = vrd->next;
    else
        vrd->prev->next = vrd->next;

    if (vrd->next) // not last in list
        vrd->next->prev = vrd->prev;

    free_list_put_vrd(vrd);

    return true;
}

/*
    Use this function to split a vrd (typically with the intention of changing
    one or more properties of a portion of the vrd)
 */
bool split_vrds_if_needed(size_t start_addr, size_t end_addr)
{
    vrd_t *vrd = find_vrd(start_addr);

    if (!vrd || start_addr > end_addr)
        return false;

    // See if the first address is in the middle of a vrd
    if (vrd->start_addr < start_addr) {
        vrd_t* new_vrd = free_list_get_vrd();
        if (!new_vrd) {
            free_list_check_level();
            new_vrd = free_list_get_vrd();
        }
        if (!new_vrd) {
            return false;
        }
        copy_vrd(new_vrd, vrd);

        // update pointers, inserting the new item before the existing
        if (!vrd->prev) // special case: vrd is first item
            g_vrdl = new_vrd;
        else {
            if (vrd->prev->next != vrd) {
                // Critical error - the VRDL is not correct.
                abort();
            }
            vrd->prev->next = new_vrd;
        }

        new_vrd->prev = vrd->prev;
        new_vrd->next = vrd;
        vrd->prev = new_vrd;

        // update address ranges in the two VRDs
        new_vrd->size = start_addr - vrd->start_addr;
        vrd->start_addr = start_addr;
        vrd->size = vrd->size - new_vrd->size;
    }

    vrd = find_vrd(end_addr);
    if (!vrd)
        return false;

    // See if the second address is in the middle of a vrd
    if (end_addr < (vrd->start_addr + vrd->size - 1)) {
        vrd_t* new_vrd = free_list_get_vrd();
        if (!new_vrd) {
            free_list_check_level();
            new_vrd = free_list_get_vrd();
        }
        if (!new_vrd) {
            return false;
        }
        copy_vrd(new_vrd, vrd);

        // update pointers, inserting the new item after the existing
        if (vrd->next) {// skip if vrd is last item
            if (vrd->next->prev != vrd) {
                abort();
            }
            vrd->next->prev = new_vrd;
        }

        new_vrd->next = vrd->next;
        new_vrd->prev = vrd;
        vrd->next = new_vrd;

        // update address ranges in the two VRDs
        new_vrd->size = (vrd->start_addr + vrd->size - 1) - end_addr; // vrd end - new end of vrd gives size of new_vrd
        new_vrd->start_addr = end_addr + 1;
        vrd->size = vrd->size - new_vrd->size;
    }

    return true;
}

/* combine_vrds()
 *  Combine VRDs with contiguous addresses *IF* they are now the same. The
 *  addresses passed in indicate a range of addresses recently updated that
 *  indicate which VRDs may be able to be merged.
 */
void combine_vrds(size_t start_addr, size_t end_addr)
{
    // Look at the VRDs in this address range and see if they can be
    // combined with adjacent VRDs.
    size_t addr = start_addr;
    vrd_t* vrd = find_vrd(addr);
    vrd_t* prev = 0;
    vrd_t* next = 0;
    size_t next_addr;

    while (vrd && addr < end_addr) {
        // Move past all addrs in this VRD for next lookup
        next_addr = vrd->start_addr + vrd->size;
        next = vrd->next;

        // Check if prev VRD has the same properties and can be combined
        prev = vrd->prev;
        if (prev && (prev->start_addr + prev->size == vrd->start_addr)) // check the two regions are contiguous
        {
            if (prev->perms == vrd->perms &&
                prev->flags == vrd->flags &&
                prev->page_type == vrd->page_type &&
                prev->state == vrd->state)
            {
                prev->size += vrd->size;
                prev->next = vrd->next;
                if (prev->next)
                    prev->next->prev = prev;
                free_list_put_vrd(vrd);
            }
        }
        vrd = next;
        addr = next_addr;
    }

    // At the end, check the last VRD against the next one out of addr range
    if (vrd && vrd->next)
    {
        next = vrd->next;
        if(vrd->start_addr + vrd->size == next->start_addr)
        {
            if (vrd->perms == next->perms &&
                vrd->flags == next->flags &&
                vrd->page_type == next->page_type &&
                vrd->state == next->state)
            {
                vrd->size += next->size;
                vrd->next = next->next;
                if (vrd->next)
                    vrd->next->prev = vrd;
                free_list_put_vrd(next);
            }
        }
    }

    // Since combine_vrds() is called at the end of sgx_virtual operations
    // it's also a good place to keep the free_list at the right level.
    free_list_check_level();
}


/*********************************************************************
 * VRD subsystem Helper functions for multiple VRD operations
 *********************************************************************/

/* check_vrds_state()
 *  iterate through all VRDs in this range and make sure they
 *  are in the state
 */
bool check_vrds_state(size_t start_addr, size_t end_addr, uint32_t state)
{
    return check_vrds_states(start_addr, end_addr, state, state);
}

/* check_vrds_states()
 *  iterate through all VRDs in this range and make sure they
 *  are all in either state_0 or state_1
 */
bool check_vrds_states(size_t start_addr, size_t end_addr, uint32_t state_0, uint32_t state_1)
{
    size_t addr = start_addr;
    vrd_t* vrd = 0;

    while (addr < end_addr) {
        vrd = find_vrd(addr);
        if (!vrd || (vrd->state != state_0 &&
            vrd->state != state_1))
            return false;
        // we've verified all addrs in this VRD, increment
        // to next unverified addr
        addr = vrd->start_addr + vrd->size;
    }
    return true;
}

/* check_vrds_not_state()
 *  iterate through all VRDs in this range and make sure they
 *  exist (in VRDL) and are not in the state
 */
bool check_vrds_not_state(size_t start_addr, size_t end_addr, uint32_t state)
{
    size_t addr = start_addr;
    vrd_t* vrd = 0;

    while (addr < end_addr) {
        vrd = find_vrd(addr);
        if (!vrd || (vrd->state == state))
            return false;
        // we've verified all addrs in this VRD, increment
        // to next unverified addr
        addr = vrd->start_addr + vrd->size;
    }
    return true;
}

/* check_vrds_pagetype()
 *   iterate through all VRDs in this range and make sure they
 *   are Page Type = pt
 */
bool check_vrds_pagetype(size_t start_addr, size_t end_addr, uint32_t pt)
{
    return check_vrds_pagetypes(start_addr, end_addr, pt, pt);
}

bool check_vrds_pagetypes(size_t start_addr, size_t end_addr, uint32_t pt_0, uint32_t pt_1)
{
    size_t addr = start_addr;
    vrd_t* vrd = 0;

    while (addr < end_addr) {
        vrd = find_vrd(addr);
        if (!vrd || ((vrd->page_type != pt_0) && (vrd->page_type != pt_1))) {
            return false;
        }
        // verified all addrs in this VRD, increment to next unverified addr
        addr = vrd->start_addr + vrd->size;
    }
    return true;
}

/* check_vrds_pr_pe_needed()
 *  Walk all VRDs in address range and see if EMODPR or EMODPE is needed to
 *  change the permission of the VRD from existing permissions to new permissions
 *  passed in. To simplify things if ANY VRD requires PR/PE, all VRDs will be
 *  EMODPR/EMODPEd by invoker of this function.
 */
bool check_vrds_pr_pe_needed(size_t start_addr, size_t end_addr, uint32_t new_perms, bool* pr_needed, bool* pe_needed)
{
    size_t addr = start_addr;
    vrd_t* vrd = 0;

    bool pr = false;
    bool pe = false;

    while (addr < end_addr) {
        vrd = find_vrd(addr);
        if (!vrd || !(vrd->state == VRD_STATE_COMMITTED || vrd->state == VRD_STATE_COMMITTED_SYSTEM))
            return false;

        // old < new correctly detects pe needed in all but one special case, RX->RW
        if (vrd->perms < new_perms ||
            (vrd->perms == SGX_PAGE_EXECUTE_READ && new_perms == SGX_PAGE_READWRITE))
            pe = true;

        // old > new correctly detects pr needed in all but one special case, RW->RX
        if (vrd->perms > new_perms ||
            (vrd->perms == SGX_PAGE_READWRITE && new_perms == SGX_PAGE_EXECUTE_READ))
            pr = true;

        // Windows will set the PTEs to either RW or RWX. Only the X bit needs to be updated
        // when it changes. Since pr = true causes a call out to VirtualProtect() which
        // sets the PTEs, set PR=true any time the X permission changes.
        if (((vrd->perms > SGX_PAGE_READWRITE) && (new_perms < SGX_PAGE_EXECUTE_READ))      // X removed
            || ((vrd->perms < SGX_PAGE_EXECUTE_READ) && (new_perms > SGX_PAGE_READWRITE)))  // X added
            pr = true;

        // verified all addrs in this VRD, increment to next unverified addr
        addr = vrd->start_addr + vrd->size;
    }
    *pr_needed = pr;
    *pe_needed = pe;

    return true;
}

/* set_vrds_state()
 *  Set the state of all VRDs from start to end addr to new_state.
 *  It is best split the VRDs before operting on them, so any
 *  errors allocating memory will be known first and the operation
 *  can return an error before starting. The check here is just a backup.
 */
bool set_vrds_state(size_t start_addr, size_t end_addr, uint32_t new_state)
{
    // Break VRDs into chunks that can be modified below, if needed
    if (!split_vrds_if_needed(start_addr, end_addr))
        return false;

    size_t addr = start_addr;
    vrd_t* vrd = 0;

    while (addr < end_addr) {
        vrd = find_vrd(addr);
        if (!vrd)
            return false;

        vrd->state = new_state;
        // we've verified all addrs in this VRD, increment
        // to next unverified addr
        addr = vrd->start_addr + vrd->size;
    }
    return true;
}

/* set_vrds_perms()
 *  Set the perms of all VRDs from start to end addr to new_state.
 *  It is best split the VRDs before operting on them, so any
 *  errors allocating memory will be known first and the operation
 *  can return an error before starting. The check here is just a backup.
 */
bool set_vrds_perms(size_t start_addr, size_t end_addr, uint32_t new_perms)
{
    // Break VRDs into chunks that can be modified below, if needed
    if (!split_vrds_if_needed(start_addr, end_addr))
        return false;

    size_t addr = start_addr;
    vrd_t* vrd = 0;

    while (addr < end_addr) {
        vrd = find_vrd(addr);
        if (!vrd)
            return false;

        vrd->perms = new_perms;
        // we've verified all addrs in this VRD, increment
        // to next unverified addr
        addr = vrd->start_addr + vrd->size;
    }
    return true;
}

/* get_vrds_perms()
 *  Get the perms for the VRD at start_addr. The perms of this page
 *  define the get value for a set of VRDs starting at that address.
 */
void get_vrds_perms(size_t start_addr, uint32_t* perms)
{
    // If perms param isn't passed in, that's not a failure, it just
    // means user doesn't care about value.
    if (!perms)
        return;

    vrd_t* vrd = find_vrd(start_addr);
    if (NULL != vrd) {
        *perms = vrd->perms;
    }
}

/* free_vrds()
 *  Free all VRDs in this address range.
 */
bool free_vrds(size_t start_addr, size_t end_addr)
{
    size_t addr = start_addr;
    vrd_t* vrd = 0;

    while (addr < end_addr) {
        vrd = find_vrd(addr);
        if (!vrd)
            return false;
        // we've verified all addrs in this VRD, increment
        // to next unverified addr
        addr = vrd->start_addr + vrd->size;
        if (addr < vrd->start_addr) {
            // We built the VRDs and somehow it contains
            // invalid information
            abort();
        }
        remove_vrd(vrd);
    }
    return true;
}

/*********************************************************************
 * Internal VRD utility functions
 *********************************************************************/

/* copy_vrd()
 *  Make a copy of a VRD
 *  vrd_copy = vrd
 */
static void copy_vrd(vrd_t* vrd_copy, const vrd_t* vrd)
{
    memcpy(vrd_copy, vrd, sizeof(vrd_t));
}

/* vrd_alloc_addr()
 *  Find an open address in the ELRANGE for creating a new VRD.
 *  This is used when the invoker asks us to assign the address.
 */
static size_t vrd_alloc_addr(size_t size)
{
    size_t addr = (size_t)rsrv_mem_base;
    size_t enclave_end = (size_t)rsrv_mem_base + rsrv_mem_size - 1;
    vrd_t* next_vrd = g_vrdl;

    while (next_vrd)
    {
        // if there's space before the next VRD, return the address
        if ((addr < next_vrd->start_addr) &&
            (next_vrd->start_addr - addr >= size))
            return addr;
        // otherwise move to the next VRD to look at and
        // set addr to just past this VRD's addr space
        addr = next_vrd->start_addr + next_vrd->size;
        next_vrd = next_vrd->next;
    }

    // If there wasn't a space between existing VRDs, check the
    // possible open space from the last VRD to the end of the enclave
    //
    // Note: enclave_end is the last usable enclave address, but if the
    // VRD is covering the end of the enclave, addr could be the address
    // just after the enclave. Add 1 to enclave_end BEFORE subtracting
    // or the code below may have an integer underflow on the subtraction.
    if ((enclave_end + 1) < addr)
    {
        // We built the VRD list and somehow it contains
        // invalid information
        abort();
    }
    if ((enclave_end + 1) - addr >= size)
    {
        return addr;
    }

    // No space found to allocate in, return 0
    return 0;
}

inline static bool addr_in_vrd(const size_t vaddr, const vrd_t* vrd)
{
    if (vaddr >= vrd->start_addr && vaddr < vrd->start_addr + vrd->size)
        return true;

    return false;
}

inline static bool addrs_in_vrd(size_t vaddr_start, size_t vaddr_end, const vrd_t* vrd)
{
    // [start to end] must be less than vrd->start - OR -
    // [start to end] must be greater than vrd->end
    // for the range to not overlap with the vrd
    if ((vaddr_end < vrd->start_addr) || (vaddr_start >= vrd->start_addr + vrd->size))
        return false;

    return true;
}

/*
 * Internal VRD utility functions to manage the free list of VRDs
 */

/* void free_list_check_level()
 *  Repopulates the free list of VRDs if it is almost empty
 *  or releases some VRDs from the free list if it is too full.
 */
static void free_list_check_level()
{
    if (g_vrdl_free_list_count >= VRDL_FREELIST_LOW_THRESHOLD &&
        g_vrdl_free_list_count <= VRDL_FREELIST_HIGH_THRESHOLD)
        return;

    vrd_t* vrd = 0;

    // if the free list is low, refill it to target level
    while (g_vrdl_free_list_count < VRDL_FREELIST_ALLOC_TARGET) {
        vrd = (vrd_t*)malloc(sizeof(vrd_t));
        if (!free_list_put_vrd(vrd))
            break;
    }

    // if the free list is too high, release some of the vrds
    while (g_vrdl_free_list_count > VRDL_FREELIST_ALLOC_TARGET) {
        vrd = free_list_get_vrd();
        if (!vrd)
            break;
        free(vrd);
    }
}


/* bool free_list_put_vrd(vrd_t* vrd)
 *  Release a VRD back to the free_list
 *  NOTE: On failure, invoker assumes func owns and frees the VRD if needed.
 */
static bool free_list_put_vrd(vrd_t* vrd)
{
    if (!vrd)
        return false;

    memset(vrd, 0, sizeof(vrd_t));
    vrd->state = VRD_STATE_FREE;

    // Don't need prev for the free list
    vrd->next = g_vrdl_free_list;
    g_vrdl_free_list = vrd;
    g_vrdl_free_list_count++;
    return true;
}

/* vrd_t* free_list_get_vrd()
 *  Get an unused VRD for use from the free list
 *  return vrd to free_list using free_list_put_vrd()
 */
static vrd_t* free_list_get_vrd()
{
    if (!g_vrdl_free_list_count)
        return NULL;

    vrd_t* vrd = g_vrdl_free_list;
    g_vrdl_free_list = g_vrdl_free_list->next;
    g_vrdl_free_list_count--;
    return vrd;
}
