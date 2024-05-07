/*-
 * Copyright (c) 2013-2015 Mark R V Murray
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SYS_DEV_RANDOM_FORTUNA_H_INCLUDED
#define	SYS_DEV_RANDOM_FORTUNA_H_INCLUDED

#include <uk/mutex.h>

typedef struct uk_mutex mtx_t;

/* TODO: Check if _KERNEL should be defined */
#ifdef _KERNEL
#define	RANDOM_RESEED_INIT_LOCK(x)		UK_MUTEX_INITIALIZER(&fortuna_state.fs_mtx)
/* Unikfrat does not have a deinit function */
#define	RANDOM_RESEED_DEINIT_LOCK(x)		do { } while (0)
#define	RANDOM_RESEED_LOCK(x)			uk_mutex_lock(&fortuna_state.fs_mtx)
#define	RANDOM_RESEED_UNLOCK(x)			uk_mutex_unlock(&fortuna_state.fs_mtx)
#define	RANDOM_RESEED_ASSERT_LOCK_OWNED(x)	fortuna_state.fs_mtx.lock_count > 0
#define	RANDOM_RESEED_ASSERT_LOCK_NOT_OWNED()	fortuna_state.fs_mtx.lock_count == 0
#else
#define	RANDOM_RESEED_INIT_LOCK(x)		UK_MUTEX_INITIALIZER(&fortuna_state.fs_mtx)
/* Unikfrat does not have a deinit function */
#define	RANDOM_RESEED_DEINIT_LOCK(x)            do { } while (0)
#define	RANDOM_RESEED_LOCK(x)			uk_mutex_lock(&fortuna_state.fs_mtx)
#define	RANDOM_RESEED_UNLOCK(x)			uk_mutex_unlock(&fortuna_state.fs_mtx)
#define	RANDOM_RESEED_ASSERT_LOCK_OWNED(x)
#define	RANDOM_RESEED_ASSERT_LOCK_NOT_OWNED()
#endif

#endif /* SYS_DEV_RANDOM_FORTUNA_H_INCLUDED */
