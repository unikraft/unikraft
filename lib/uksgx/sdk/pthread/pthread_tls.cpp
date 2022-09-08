/*	$OpenBSD: rthread_cond.c,v 1.5 2019/01/29 17:40:26 mpi Exp $ */
/*
 * Copyright (c) 2017 Martin Pieuchot <mpi@openbsd.org>
 * Copyright (c) 2012 Philip Guenther <guenther@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <errno.h>
#include "sgx_trts.h"
#include "sgx_spinlock.h"
#include "pthread_imp.h"

#include "internal/se_cdefs.h"
#include "internal/arch.h"
#include "internal/thread_data.h"
#include "trts_internal.h"
#include "trts_util.h"

#define PTHREAD_DESTRUCTOR_ITERATIONS		4
#define PTHREAD_KEYS_MAX			256

static struct sgx_thread_key rkeys[PTHREAD_KEYS_MAX];
static volatile uint32_t rkeyslock = SGX_SPINLOCK_INITIALIZER;

extern __thread pthread_info pthread_info_tls;

/*
 *: thread-specific data key creation 
 */
int pthread_key_create(pthread_key_t *key, void (*destructor)(void*))
{
    if ( !sgx_is_within_enclave((void *)key, sizeof(pthread_key_t))) {
        return EINVAL;
    }

    static int hint;
    int i;

    sgx_spin_lock(&rkeyslock);
    if (rkeys[hint].used) {
        for (i = 0; i < PTHREAD_KEYS_MAX; i++) {
            if (!rkeys[i].used) break;
        }
        if (i == PTHREAD_KEYS_MAX) {
            sgx_spin_unlock(&rkeyslock);
            return (EAGAIN);
        }
        hint = i;
     }
    rkeys[hint].used = 1;
    rkeys[hint].destructor = destructor;

    *key = hint++;
    if (hint >= PTHREAD_KEYS_MAX)
        hint = 0;
    sgx_spin_unlock(&rkeyslock);

    return (0);
}

/*
 *: thread-specific data key deletion
 */
int pthread_key_delete(pthread_key_t key)
{
    if ( !sgx_is_within_enclave((void *)&key, sizeof(pthread_key_t))) {
        return EINVAL;
    }

    int rv = 0;

    if (key < 0 || key >= PTHREAD_KEYS_MAX)
        return (EINVAL);

    sgx_spin_lock(&rkeyslock);
    if (!rkeys[key].used) {
        rv = EINVAL;
        goto out;
    }

    rkeys[key].used = 0;
    rkeys[key].destructor = NULL;

out:
    sgx_spin_unlock(&rkeyslock);
    return (rv);
}


static struct sgx_pthread_storage * _pthread_findstorage(pthread_key_t key)
{
    struct sgx_pthread_storage *rs;

    if (!rkeys[key].used) {
        rs = NULL;
        return NULL;
    }

    pthread_info* thread_info = (pthread_info*)&pthread_info_tls;

    for (rs = thread_info->m_local_storage; rs; rs = rs->next) {
        if (rs->keyid == key)
            break;
    }
    if (!rs) {
        rs = (struct sgx_pthread_storage *)calloc(1, sizeof(*rs));
        if (!rs)
            return NULL;
        rs->keyid = key;
        rs->data = NULL;
        rs->next = thread_info->m_local_storage;
        thread_info->m_local_storage = rs;
    }

    return (rs);
}

/*
 *: get a thread-specific data value
 */
void *pthread_getspecific(pthread_key_t key)
{
    if ( !sgx_is_within_enclave((void *)&key, sizeof(pthread_key_t))) {
        return NULL;
    }

    struct sgx_pthread_storage *rs;

    if (key < 0 || key >= PTHREAD_KEYS_MAX)
        return (NULL);

    rs = _pthread_findstorage(key);
    if (!rs)
        return (NULL);

    return (rs->data);
}

/*
 *: thread-specific data management
 */
int pthread_setspecific(pthread_key_t key, const void *data)
{
    if ( !sgx_is_within_enclave((void *)&key, sizeof(pthread_key_t))) {
        return EINVAL;
    }

    struct sgx_pthread_storage *rs;

    if (key < 0 || key >= PTHREAD_KEYS_MAX)
        return (EINVAL);

    rs = _pthread_findstorage(key);
    if (!rs)
        return (ENOMEM);
    rs->data = (void *)data;

    return (0);
}

void _pthread_tls_destructors(void)
{
    struct sgx_pthread_storage *rs;
    int i;

    // For a child thread created inside enclave, we need to clear the TLS when
    // the thread exits.
    // In other cases, we need to make decision based on the tcs policy:
    //   1) Unbinding mode, we need to clear the TLS before the root ecall exits.
    //   2) Binding mode, we need to persist the TLS across multiple ECALLs. In this
    //      case, the destructor should not be called.
    if(NULL == pthread_info_tls.m_pthread && true == is_tcs_binding_mode())
        //do nothing
        return;

    sgx_spin_lock(&rkeyslock);
    for (i = 0; i < PTHREAD_DESTRUCTOR_ITERATIONS; i++) {
        for (rs = pthread_info_tls.m_local_storage; rs; rs = rs->next) {
            if (!rs->data)
                continue;
            if (rkeys[rs->keyid].destructor) {
                void (*destructor)(void *) = rkeys[rs->keyid].destructor;
                void *data = rs->data;
                rs->data = NULL;
                sgx_spin_unlock(&rkeyslock);
                destructor(data);
                sgx_spin_lock(&rkeyslock);
            }
        }
    }
    for (rs = pthread_info_tls.m_local_storage; rs; rs = pthread_info_tls.m_local_storage) {
        pthread_info_tls.m_local_storage = rs->next;
        free(rs);
    }
    sgx_spin_unlock(&rkeyslock);
}


