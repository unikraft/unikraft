/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Radu Andrei Tudorica <raduandreitudorica3@gmail.com>
 *
 *
 *
 * Copyright (c) 2026 Radu Andrei Tudorica <raduandreitudorica3@gmail.com>
 *                     All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <uk/test.h>
#include <uk/allocbbuddy.h>
#include <uk/essentials.h>
#include <uk/arch/paging.h>
#include <uk/alloc.h>
#include <errno.h>
#include <uk/errptr.h>

#define POOL_SIZE (64 * 1024)

UK_TESTCASE(ukallocbbuddy, test_init)
{
	struct uk_alloc *head;
	static char pool[POOL_SIZE] __align(PAGE_SIZE);

	head = uk_allocbbuddy_init(pool, POOL_SIZE);
	UK_TEST_ASSERT(head != NULL);
}

UK_TESTCASE(ukallocbbuddy, test_alloc_dealloc)
{
    struct uk_alloc *head;
    static char pool[POOL_SIZE] __align(PAGE_SIZE);
    void *ptr;

    head = uk_allocbbuddy_init(pool, POOL_SIZE);
    UK_TEST_ASSERT(head != NULL);

    ptr = uk_palloc(head, 1);
    UK_TEST_EXPECT(ptr != NULL);

    if(ptr)
        uk_pfree(head, ptr, 1);
}

UK_TESTCASE(ukallocbbuddy, test_exhaustion)
{
    struct uk_alloc *head;
    static char pool[POOL_SIZE] __align(PAGE_SIZE);
    void *ptr;

    head = uk_allocbbuddy_init(pool, POOL_SIZE);
    UK_TEST_ASSERT(head != NULL);

    ptr = uk_palloc(head, 1000);

    UK_TEST_EXPECT(ptr == NULL);
    UK_TEST_EXPECT(errno == ENOMEM);
}

UK_TESTCASE(ukallocbbuddy, test_coalescing)
{
    struct uk_alloc *head;
    static char pool[POOL_SIZE] __align(PAGE_SIZE);
    void *p1, *p2, *p3;

    head = uk_allocbbuddy_init(pool, POOL_SIZE);
    UK_TEST_ASSERT(head != NULL);

    p1 = uk_palloc(head, 1);
    p2 = uk_palloc(head, 1);
    UK_TEST_EXPECT(p1 != NULL && p2 != NULL);

    uk_pfree(head, p1, 1);
    uk_pfree(head, p2, 1);

    p3 = uk_palloc(head, 2);
    UK_TEST_EXPECT(p3 != NULL);

    if (p3)
        uk_pfree(head, p3, 2);
}

UK_TESTCASE(ukallocbbuddy, test_multipage_alloc)
{
    struct uk_alloc *head;
    static char pool[POOL_SIZE] __align(PAGE_SIZE);
    void *ptr;
    unsigned long pages = 4;

    head = uk_allocbbuddy_init(pool, POOL_SIZE);
    UK_TEST_ASSERT(head != NULL);

    ptr = uk_palloc(head, pages);
    UK_TEST_EXPECT(ptr != NULL);

    UK_TEST_EXPECT(((uintptr_t)ptr % PAGE_SIZE) == 0);

    if (ptr)
        uk_pfree(head, ptr, pages);
}
uk_testsuite_register(ukallocbbuddy, NULL);

