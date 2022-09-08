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

#include "util.h"
#include "se_detect.h"
#include "se_thread.h"
#include "sgx_thread.h"
#include "se_event.h"
#include "enclave.h"
#include "rts_cmd.h"
#include "metadata.h"
#include <pthread.h>
#include <unistd.h>
#include "get_thread_id.h"

static void* pthread_create_routine(void* arg)
{
    if(NULL == arg)
        //It's a critial error.
        abort();

    CTrustThread * trust_thread = (CTrustThread *)arg;
    CEnclave *enclave = ((CTrustThread *)trust_thread)->get_enclave();
    if(NULL == enclave) {
         //It's a critial error.
         abort();
    }
    enclave->atomic_inc_ref(); 
    //bind the trust_thread with this new thread's thread_id
    se_thread_id_t thread_id = get_thread_id();
    enclave->get_thread_pool()->bind_pthread(thread_id, trust_thread);

    uint64_t waiter = 0;
    enclave->ecall(ECMD_ECALL_PTHREAD, NULL, (void*)&waiter, false);
    if(waiter != 0)
    {
        //wake the pthread_join waiter
        se_handle_t hevent = CEnclavePool::instance()->get_event((void *)waiter);
        if (hevent != NULL) {
            se_event_wake(hevent);  //Needn't check it's return value
        }
    }
    CEnclavePool::instance()->unref_enclave(enclave);
    return NULL;
}

/*
 * ocall used to create an separate thread.
 * tcs: tcs
*/
extern "C" sgx_status_t pthread_create_ocall(unsigned long long self)
{
    CEnclave *enclave = CEnclavePool::instance()->get_enclave_with_tcs((void*)self);
    if(NULL == enclave)
        return SGX_ERROR_INVALID_PARAMETER;

    CTrustThread *trust_thread = enclave->get_free_tcs();
    if(NULL == trust_thread)
        return SGX_ERROR_OUT_OF_TCS;

    pthread_attr_t attr;
    if(pthread_attr_init(&attr) != 0)
        return SGX_ERROR_UNEXPECTED;
    //Set the new thread as "PTHREAD_CREATE_DETACHED", it will optimize SGX pthread_create performance.
    if(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
        return SGX_ERROR_UNEXPECTED;

    pthread_t thread;
    int ret = pthread_create(&thread, &attr, pthread_create_routine, (void *)(trust_thread));  
    if(ret != 0)
    {
        //Add back the tcs to free queue directly, because the tcs hasn't been binded with the new thread yet.
        enclave->get_thread_pool()->add_to_free_thread_vector(trust_thread);
        return SGX_ERROR_UNEXPECTED;
    }

    return SGX_SUCCESS;
}

/*
 * ocall 
 * waiter: The target thread's tcs
*/
extern "C" sgx_status_t pthread_wait_timeout_ocall(unsigned long long waiter, unsigned long long timeout)
{
    se_handle_t hevent = CEnclavePool::instance()->get_event((void*)waiter);
    if (hevent == NULL)
        return SGX_ERROR_DEVICE_BUSY;
		
    if (SE_MUTEX_SUCCESS != se_event_wait_timeout(hevent, timeout))
        return SGX_ERROR_UNEXPECTED;

    return SGX_SUCCESS;
}

extern "C" sgx_status_t pthread_wakeup_ocall(unsigned long long waiter)
{
    se_handle_t hevent = CEnclavePool::instance()->get_event((void *)waiter);
    if (hevent != NULL) {
	se_event_wake(hevent);	//Needn't check it's return value
    }
    return SGX_SUCCESS;
}

