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

/*
 *: dynamic package initialization 
 */
int pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
    if ( !sgx_is_within_enclave((void *)once_control, sizeof(pthread_once_t))) {
        return EINVAL;
    }

    pthread_mutex_lock(&once_control->mutex);
    if (once_control->state == PTHREAD_NEEDS_INIT) {
        init_routine();
        once_control->state = PTHREAD_DONE_INIT;
    }
    pthread_mutex_unlock(&once_control->mutex);

    return (0);
}
