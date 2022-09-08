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


#include "se_memcpy.h"
#include "thread_data.h"
#include "global_data.h"
#include "rts.h"
#include "util.h"
#include "xsave.h"
#include "sgx_trts.h"
#include "sgx_lfence.h"
#include "sgx_spinlock.h"
#include "global_init.h"
#include "trts_internal.h"
#include "trts_inst.h"
#include "trts_emodpr.h"
#include "trts_util.h"
#include "metadata.h"
#  include "linux/elf_parser.h"
#  define GET_TLS_INFO  elf_tls_info

#include "pthread_imp.h"
#include "sgx_random_buffers.h"
#include "se_page_attr.h"

__attribute__((weak)) sgx_status_t _pthread_thread_run(void* ms) {UNUSED(ms); return SGX_SUCCESS;}
__attribute__((weak)) bool _pthread_enabled() {return false;}
__attribute__((weak)) void _pthread_tls_store_state(sgx_status_t state) {UNUSED(state);}
__attribute__((weak)) sgx_status_t _pthread_tls_get_state(void) {return SGX_SUCCESS;}
__attribute__((weak)) void _pthread_tls_store_context(void* context) {UNUSED(context);}
__attribute__((weak)) void _pthread_wakeup_join(void* ms) {UNUSED(ms);}
__attribute__((weak)) void _pthread_tls_destructors(void) {}

extern "C"
__attribute__((weak)) void tc_set_idle() {}


// is_ecall_allowed()
// check the index in the dynamic entry table
static sgx_status_t is_ecall_allowed(uint32_t ordinal)
{
    if(ordinal >= g_ecall_table.nr_ecall)
    {
        return SGX_ERROR_INVALID_FUNCTION;
    }
    thread_data_t *thread_data = get_thread_data();

    sgx_lfence();

    if(thread_data->last_sp == thread_data->stack_base_addr)
    {
        // root ECALL, check the priv bits.
        if (g_ecall_table.ecall_table[ordinal].is_priv)
            return SGX_ERROR_ECALL_NOT_ALLOWED;
        return SGX_SUCCESS;
    }
    ocall_context_t *context = reinterpret_cast<ocall_context_t*>(thread_data->last_sp);
    if(context->ocall_flag != OCALL_FLAG)
    {
        // abort the enclave if ocall frame is invalid
        abort();
    }
    uintptr_t ocall_index = context->ocall_index;
    if(ocall_index >= g_dyn_entry_table.nr_ocall)
    {
        return SGX_ERROR_INVALID_FUNCTION;
    }
    return (g_dyn_entry_table.entry_table[ocall_index * g_ecall_table.nr_ecall + ordinal] ? SGX_SUCCESS : SGX_ERROR_ECALL_NOT_ALLOWED);
}
// get_func_addr()
//      Get the address of ecall function from the ecall table
// Parameters:
//      [IN] ordinal - the index of the ecall function in the ecall table
// Return Value:
//      non-zero - success
//      zero - fail
//
static sgx_status_t get_func_addr(uint32_t ordinal, void **addr)
{
    if(ordinal == (uint32_t)ECMD_ECALL_PTHREAD)
    {
        *addr = (void*) _pthread_thread_run;
        return SGX_SUCCESS;
    }

    // Normal user-defined ECalls
    sgx_status_t status = is_ecall_allowed(ordinal);
    if(SGX_SUCCESS != status)
    {
        return status;
    }

    *addr = const_cast<void *>(g_ecall_table.ecall_table[ordinal].ecall_addr);
    if(!sgx_is_within_enclave(*addr, 0))
    {
        return SGX_ERROR_UNEXPECTED;
    }

    return SGX_SUCCESS;
}


typedef struct _tcs_node_t
{
    uintptr_t tcs;
    struct _tcs_node_t *next;
} tcs_node_t;

static tcs_node_t *g_tcs_node = NULL;
static uintptr_t g_tcs_cookie = 0;
#define ENC_TCS_POINTER(x)  (uintptr_t)(x) ^ g_tcs_cookie
#define DEC_TCS_POINTER(x)  (void *)((x) ^ g_tcs_cookie)

// do_save_tcs()
//      Save tcs while function do_ecall_add_thread invoked.
// Parameters:
//      [IN] ptcs - the tcs_t pointer which need to be saved
// Return Value:
//     zero - success
//     non-zero - fail
//
static sgx_status_t do_save_tcs(void *ptcs)
{
    if(unlikely(g_tcs_cookie == 0))
    {
        uintptr_t rand = 0;
        do
        {
            if(SGX_SUCCESS != sgx_read_rand((unsigned char *)&rand, sizeof(rand)))
            {
                return SGX_ERROR_UNEXPECTED;
            }
        } while(rand == 0);

        if(g_tcs_cookie == 0)
        {
            g_tcs_cookie = rand;
        }
    }

    tcs_node_t *tcs_node = (tcs_node_t *)malloc(sizeof(tcs_node_t));
    if(!tcs_node)
    {
        return SGX_ERROR_UNEXPECTED;
    }

    tcs_node->tcs = ENC_TCS_POINTER(ptcs);

    tcs_node->next = g_tcs_node;
    g_tcs_node = tcs_node;

    return SGX_SUCCESS;
}

// do_del_tcs()
//      Delete tcs from the global tcs list.
// Parameters:
//      [IN] ptcs - the tcs_t pointer which need to be deleted
// Return Value:
//     N/A
//
static void do_del_tcs(void *ptcs)
{
    if(!is_utility_thread())
        return;

    if (g_tcs_node != NULL)
    {
        if (DEC_TCS_POINTER(g_tcs_node->tcs) == ptcs)
        {
            tcs_node_t *tmp = g_tcs_node;
            g_tcs_node = g_tcs_node->next;
            free(tmp);
        }
        else
        {
            tcs_node_t *tcs_node = g_tcs_node->next;
            tcs_node_t *pre_tcs_node = g_tcs_node;
            while (tcs_node != NULL)
            {
                if (DEC_TCS_POINTER(tcs_node->tcs) == ptcs)
                {
                    pre_tcs_node->next = tcs_node->next;
                    free(tcs_node);
                    break;
                }

                pre_tcs_node = tcs_node;
                tcs_node = tcs_node->next;
            }
        }
    }
}

static int add_static_threads(const volatile layout_t *layout_start, const volatile layout_t *layout_end, size_t offset)
{
    int ret = -1;
    for (const volatile layout_t *layout = layout_start; layout < layout_end; layout++)
    {
        if (!IS_GROUP_ID(layout->group.id) && (layout->entry.si_flags & SI_FLAGS_TCS) && layout->entry.attributes == (PAGE_ATTR_EADD | PAGE_ATTR_EEXTEND))
        {
            uintptr_t tcs_addr = (uintptr_t)layout->entry.rva + offset + (uintptr_t)get_enclave_base();
            if (do_save_tcs(reinterpret_cast<void *>(tcs_addr)) != SGX_SUCCESS)
		    return (-1);
        }
        else if (IS_GROUP_ID(layout->group.id)){
            size_t step = 0;
            for(uint32_t j = 0; j < layout->group.load_times; j++)
            {
                step += (size_t)layout->group.load_step;
                if(0 != (ret = add_static_threads(&layout[-layout->group.entry_count], layout, step)))
                    return ret;
            }
        }
    }
    return 0;
}

static volatile bool           g_is_first_ecall = true;
static volatile sgx_spinlock_t g_ife_lock       = SGX_SPINLOCK_INITIALIZER;

typedef sgx_status_t (*ecall_func_t)(void *ms);
static sgx_status_t trts_ecall(uint32_t ordinal, void *ms)
{
    sgx_status_t status = SGX_ERROR_UNEXPECTED;

    if (unlikely(g_is_first_ecall))
    {
        // The thread performing the global initialization cannot do a nested ECall
        thread_data_t *thread_data = get_thread_data();
        if (thread_data->last_sp != thread_data->stack_base_addr)
        { // nested ecall
            return SGX_ERROR_ECALL_NOT_ALLOWED;
        }

        sgx_spin_lock(&g_ife_lock);
        if (g_is_first_ecall)
        {
#ifndef SE_SIM
            if(EDMM_supported)
            {
                // save all the static threads into the thread table. These TCS would be trimmed in the uninit flow
                if (add_static_threads(
                            &g_global_data.layout_table[0],
                            &g_global_data.layout_table[0] + g_global_data.layout_entry_num,
                            0) != 0)
                {
                    return SGX_ERROR_UNEXPECTED;
                }

                //change back the page permission
                size_t enclave_start = (size_t)get_enclave_base();
                if((status = change_protection((void *)enclave_start)) != SGX_SUCCESS)
                {
                    sgx_spin_unlock(&g_ife_lock);
                    return status;
                }
            }
#endif
            //invoke global object's construction
            init_global_object();
            g_is_first_ecall = false;
        }
        sgx_spin_unlock(&g_ife_lock);
    }

    void *addr = NULL;
    status = get_func_addr(ordinal, &addr);
    if(status == SGX_SUCCESS)
    {
        ecall_func_t func = (ecall_func_t)addr;

        sgx_lfence();

        status = func(ms);
    }
    
    return status;
}

extern "C" uintptr_t __stack_chk_guard;
static void init_static_stack_canary(void *tcs)
{
    size_t *canary = TCS2CANARY(tcs);
    *canary = (size_t)__stack_chk_guard;
}

sgx_status_t do_init_thread(void *tcs, bool enclave_init)
{
    thread_data_t *thread_data = GET_PTR(thread_data_t, tcs, g_global_data.td_template.self_addr);
#ifndef SE_SIM
    size_t saved_stack_commit_addr = thread_data->stack_commit_addr;
    bool thread_first_init = (saved_stack_commit_addr == 0) ? true : false;
#endif
    size_t stack_guard = thread_data->stack_guard;
    size_t thread_flags = thread_data->flags;
    memcpy_s(thread_data, SE_PAGE_SIZE, const_cast<thread_data_t *>(&g_global_data.td_template), sizeof(thread_data_t));
    thread_data->last_sp += (size_t)tcs;
    thread_data->self_addr += (size_t)tcs;
    thread_data->stack_base_addr += (size_t)tcs;
    thread_data->stack_limit_addr += (size_t)tcs;
    thread_data->stack_commit_addr = thread_data->stack_limit_addr;
    thread_data->first_ssa_gpr += (size_t)tcs;
    thread_data->tls_array += (size_t)tcs;
    thread_data->tls_addr += (size_t)tcs;
    thread_data->last_sp -= (size_t)STATIC_STACK_SIZE;
    thread_data->stack_base_addr -= (size_t)STATIC_STACK_SIZE;
    thread_data->stack_guard = stack_guard;
    thread_data->flags = thread_flags;
    init_static_stack_canary(tcs);

    if (EDMM_supported && enclave_init)
    {
        thread_data->flags = SGX_UTILITY_THREAD;
    }
#ifndef SE_SIM
    if (thread_first_init)
    {
        if (EDMM_supported && (enclave_init || is_dynamic_thread(tcs)))
        {
            uint32_t page_count = get_dynamic_stack_max_page();
            thread_data->stack_commit_addr += ((sys_word_t)page_count << SE_PAGE_SHIFT);
        }
    }
    else
    {
        thread_data->stack_commit_addr = saved_stack_commit_addr;
    }
#endif

    uintptr_t tls_addr = 0;
    size_t tdata_size = 0;

    if(0 != GET_TLS_INFO(get_enclave_base(), &tls_addr, &tdata_size))
    {
        return SGX_ERROR_UNEXPECTED;
    }
    if(tls_addr)
    {
        memset((void *)TRIM_TO_PAGE(thread_data->tls_addr), 0, ROUND_TO_PAGE(thread_data->self_addr - thread_data->tls_addr));
        memcpy_s((void *)(thread_data->tls_addr), thread_data->self_addr - thread_data->tls_addr, (void *)tls_addr, tdata_size);
    }

    return SGX_SUCCESS;
}

sgx_status_t do_ecall(int index, void *ms, void *tcs)
{
    sgx_status_t status = SGX_ERROR_UNEXPECTED;
    if(ENCLAVE_INIT_DONE != get_enclave_state())
    {
        return status;
    }
    thread_data_t *thread_data = get_thread_data();
    if( (NULL == thread_data) || 
            ((thread_data->stack_base_addr == thread_data->last_sp) && 
                    ( (0 != g_global_data.thread_policy) ||
                       (SGX_PTHREAD_EXIT == _pthread_tls_get_state()) ||    /*Force do initial this tcs if it was used by pthread() created thread previously.*/
                        (index == ECMD_ECALL_PTHREAD))))  /*Force do initial this tcs if it used by pthread() created thread. */
    {
        status = do_init_thread(tcs, false);
        if(0 != status)
        {
            return status;
        }
    }
    thread_data = get_thread_data();
    if(thread_data->stack_base_addr == thread_data->last_sp)
    {
        //root ecall
	//
        // If PKRU is supported, write 0 to PKRU register.
	sgx_wrpkru(0);

        if(_pthread_enabled())
        {
            jmp_buf     buf = {0};
            if(0 == setjmp(buf))
            {
                _pthread_tls_store_context((void*)buf);
                status = random_stack_advance<0x800>(trts_ecall, index, ms);
            }
            else
            {
                //Enter here if pthread_exit() is called inside ECALL functions.
                //Important: manually reset the last_sp
                thread_data->last_sp = thread_data->stack_base_addr;
                status = SGX_PTHREAD_EXIT;
            }
            if(ECMD_ECALL_PTHREAD == index || SGX_PTHREAD_EXIT == status)
            {
                /* 
                  * Set the TCS's tls state variable to "SGX_PTHREAD_EXIT":
                  *  1. Pthread() create thread exits normally.
                  *  2. ECALL() is exited by calling pthread_exit().
                  *
                  * In future, the TCS will always be initialized no matter it's used by a new normal root ECALL() or it's used by a new pthread() create thread.
                  *
                  * As example: (In bind mode)
                  * 1. This TCS is used by pthread created thread. So the TCS's state will be set as "SGX_PTHREAD_EXIT" after the thread exits.
                  * 2. Then the same TCS is used by a normal root ECALL, the TCS will still be initialized because it's state was set as "SGX_PTHREAD_EXIT".
                  *
                */
                _pthread_tls_store_state(SGX_PTHREAD_EXIT);
            }
            //-- execute some resource recycle function here, such as tls resource recycle
            _pthread_tls_destructors();
            _pthread_wakeup_join(ms); 
        }
        else 
        {
            //sgx pthread lib isn't linked
            status = random_stack_advance<0x800>(trts_ecall, index, ms);
        }
    }
    else
    {
        status = trts_ecall(index, ms);
    }

    // TCS unbind mode and root ECALL:
    // 1. If tcmalloc is used, release the cached free memory back to its central list.
    // 2. If tcmalloc is not used, it's a no-op.
    if ((0 != g_global_data.thread_policy) &&
            (thread_data->stack_base_addr == thread_data->last_sp))
    {
        tc_set_idle();
    }

    return status;
}

sgx_status_t do_ecall_add_thread(void *ms)
{
    sgx_status_t status = SGX_ERROR_UNEXPECTED;

    if(!is_utility_thread())
        return status;

    struct ms_tcs *tcs = (struct ms_tcs*)ms;
    if (tcs == NULL)
    {
        return status;
    }

    if (!sgx_is_outside_enclave(tcs, sizeof(struct ms_tcs)))
    {
        return status;
    }

    sgx_lfence();

    const struct ms_tcs mtcs = *tcs;
    void* ptcs = mtcs.ptcs;
    if (ptcs == NULL)
    {
        return status;
    }

    sgx_lfence();

    status = do_save_tcs(ptcs);
    if(SGX_SUCCESS != status)
    {
        return status;
    }

    status = do_add_thread(ptcs);
    if (SGX_SUCCESS != status)
    {
    	do_del_tcs(ptcs);
        return status;
    }

    return status;
}

uint32_t volatile g_uninit_flag = 0;
// do_uninit_enclave()
//      Run the global uninitialized functions when the enclave is destroyed.
// Parameters:
//      [IN] tcs - used for running this task
// Return Value:
//     zero - success
//     non-zero - fail
//
sgx_status_t do_uninit_enclave(void *tcs)
{
#ifndef SE_SIM
    // This function should only be called when
    //  1. EDMM is enabled
    //  2. on HW mode
    // urts would not call this ECALL either on simulation mode
    // or on non-EDMM supported platform.
    if (!EDMM_supported)
    {
        set_enclave_state(ENCLAVE_CRASHED);
        return SGX_ERROR_UNEXPECTED;
    }

    if(!is_utility_thread() && is_dynamic_thread_exist())
    {
        set_enclave_state(ENCLAVE_CRASHED);
        return SGX_ERROR_UNEXPECTED;
    }

    // Set uninit_flag to indicate the do_uninit_enclave is called
    __sync_or_and_fetch(&g_uninit_flag, 1);

    tcs_node_t *tcs_node = g_tcs_node;
    g_tcs_node = NULL;
    while (tcs_node != NULL)
    {
        if (DEC_TCS_POINTER(tcs_node->tcs) == tcs)
        {
            tcs_node_t *tmp = tcs_node;
            tcs_node = tcs_node->next;
            free(tmp);
            continue;
        }

        size_t start = (size_t)DEC_TCS_POINTER(tcs_node->tcs);
        size_t end = start + (1 << SE_PAGE_SHIFT);
        int rc = sgx_accept_forward(SI_FLAG_TRIM | SI_FLAG_MODIFIED, start, end);
        if(rc != 0)
        {
            set_enclave_state(ENCLAVE_CRASHED);
            return SGX_ERROR_UNEXPECTED;
        }

        tcs_node_t *tmp = tcs_node;
        tcs_node = tcs_node->next;
        free(tmp);
    }

    sgx_spin_lock(&g_ife_lock);
    if (!g_is_first_ecall)
    {
        uninit_global_object();
    }
    sgx_spin_unlock(&g_ife_lock);
#else
    UNUSED(tcs);
#endif    
    set_enclave_state(ENCLAVE_CRASHED);

    return SGX_SUCCESS;
}

extern sdk_version_t g_sdk_version;

extern "C" sgx_status_t trts_mprotect(size_t start, size_t size, uint64_t perms)
{
    int rc = -1;
    size_t page;
    sgx_status_t ret = SGX_SUCCESS;
    SE_DECLSPEC_ALIGN(sizeof(sec_info_t)) sec_info_t si;

    //Error return if start or size is not page-aligned or size is zero.
    if (!IS_PAGE_ALIGNED(start) || (size == 0) || !IS_PAGE_ALIGNED(size))
        return SGX_ERROR_INVALID_PARAMETER;
    if (g_sdk_version == SDK_VERSION_2_0)
    {
        ret = change_permissions_ocall(start, size, perms, EDMM_MODPR);
        if (ret != SGX_SUCCESS)
            return ret;
    }

    si.flags = perms|SI_FLAG_REG|SI_FLAG_PR;
    memset(&si.reserved, 0, sizeof(si.reserved));

    for(page = start; page < start + size; page += SE_PAGE_SIZE)
    {
        do_emodpe(&si, page);
        // If the target permission to set is RWX, no EMODPR, hence no EACCEPT.
        if ((perms & (SI_FLAG_W|SI_FLAG_X)) != (SI_FLAG_W|SI_FLAG_X))
        {
            rc = do_eaccept(&si, page);
            if(rc != 0)
                return (sgx_status_t)rc;
        }
    }

    return SGX_SUCCESS;
}
