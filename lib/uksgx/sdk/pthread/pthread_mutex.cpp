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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "sgx_trts.h"
#include "sgx_spinlock.h"
#include "pthread_imp.h"
#include "util.h"

static volatile uint32_t static_init_lock = SGX_SPINLOCK_INITIALIZER;

int pthread_mutex_init(pthread_mutex_t *mutexp, const pthread_mutexattr_t *attr)
{
    UNUSED(attr);
    return sgx_thread_mutex_init(mutexp, NULL);
}

int pthread_mutex_destroy(pthread_mutex_t *mutexp)
{
    return sgx_thread_mutex_destroy(mutexp);
}

int pthread_mutex_lock(pthread_mutex_t *mutexp)
{
    return sgx_thread_mutex_lock(mutexp);
}

int pthread_mutex_trylock(pthread_mutex_t *mutexp)
{
    return sgx_thread_mutex_trylock(mutexp);
}

int pthread_mutex_unlock(pthread_mutex_t *mutexp)
{
    return sgx_thread_mutex_unlock(mutexp);
}

