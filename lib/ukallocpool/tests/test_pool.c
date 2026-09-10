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
#include <uk/alloc.h>
#include <uk/essentials.h>
#include <uk/allocpool.h>
#include <uk/page.h>

#define TEST_OBJ_SIZE  64
#define TEST_OBJ_ALIGN 8
#define TEST_OBJ_COUNT 10

UK_TESTCASE(ukallocpool, test_take_return)
{
   struct uk_allocpool *pool;
	__sz memory_needed;
	void *object;
	unsigned int initial_count; /* Corectat: adăugat 'n' */
	
	static char pool_buffer[__PAGE_SIZE] __align(__PAGE_SIZE);

	/* Calculate required memory and initialize the pool */
	memory_needed = uk_allocpool_reqmem(TEST_OBJ_COUNT, TEST_OBJ_SIZE, TEST_OBJ_ALIGN);
	pool = uk_allocpool_init(pool_buffer, memory_needed, TEST_OBJ_SIZE, TEST_OBJ_ALIGN);
	UK_TEST_ASSERT(pool != NULL);

	/* Record the initial number of available objects in the pool */
	initial_count = uk_allocpool_availcount(pool);

	/* Take an object from the pool using the fast-path interface. */
	object = uk_allocpool_take(pool);
	UK_TEST_EXPECT(object != NULL);

	if (object) {
		/* Verify that the returned pointer respects the requested alignment */
		UK_TEST_EXPECT_SNUM_EQ(((uintptr_t)object % TEST_OBJ_ALIGN), 0);

		/* The available object count should decrease by exactly 1 */
		/* Corectat: folosit 'pool' in loc de 'my_pool' */
		UK_TEST_EXPECT_SNUM_EQ(uk_allocpool_availcount(pool), initial_count - 1);

		/* Return the object back to the pool's free list */
		uk_allocpool_return(pool, object);

		UK_TEST_EXPECT_SNUM_EQ(uk_allocpool_availcount(pool), initial_count);
	}
}

UK_TESTCASE(ukallocpool, test_batch_operations)
{
	struct uk_allocpool *pool;
	__sz memory_needed;
	unsigned int initial_count;
	unsigned int taken_count;
	void *object_array[5];
	static char pool_buffer[__PAGE_SIZE] __align(__PAGE_SIZE);

	memory_needed = uk_allocpool_reqmem(TEST_OBJ_COUNT, TEST_OBJ_SIZE, TEST_OBJ_ALIGN);
	pool = uk_allocpool_init(pool_buffer, memory_needed, TEST_OBJ_SIZE, TEST_OBJ_ALIGN);
	UK_TEST_ASSERT(pool != NULL);

	initial_count = uk_allocpool_availcount(pool);

	/* * Attempt to take 5 objects at once.
	 * The function should return the number of successfully allocated objects.
	 */
	taken_count = uk_allocpool_take_batch(pool, object_array, 5);
	
	/* We expect exactly 5 objects to be successfully extracted */
	UK_TEST_EXPECT_SNUM_EQ(taken_count, 5);
	
	/* The available object count should decrease by exactly 5 */
	UK_TEST_EXPECT_SNUM_EQ(uk_allocpool_availcount(pool), initial_count - 5);

	/* * Return all 5 objects back to the pool in a single fast operation.
	 */
	uk_allocpool_return_batch(pool, object_array, 5);

	/* Verify that all 5 objects were successfully returned to the free list */
	UK_TEST_EXPECT_SNUM_EQ(uk_allocpool_availcount(pool), initial_count);
}

UK_TESTCASE(ukallocpool, test_exhaustion)
{
	struct uk_allocpool *pool;
	__sz memory_needed;
	void *object;
	unsigned int i;

	static char pool_buffer[__PAGE_SIZE] __align(__PAGE_SIZE);

	memory_needed = uk_allocpool_reqmem(TEST_OBJ_COUNT, TEST_OBJ_SIZE, TEST_OBJ_ALIGN);
	pool = uk_allocpool_init(pool_buffer, memory_needed, TEST_OBJ_SIZE, TEST_OBJ_ALIGN);
	UK_TEST_ASSERT(pool != NULL);

	/* We loop exactly TEST_OBJ_COUNT times to take all available objects. */
	for (i = 0; i < TEST_OBJ_COUNT; i++) {
		object = uk_allocpool_take(pool);
		UK_TEST_EXPECT(object != NULL);
	}

	UK_TEST_EXPECT_SNUM_EQ(uk_allocpool_availcount(pool), 0);

	/* Attempt to take one more object from the exhausted pool. */
	object = uk_allocpool_take(pool);

	UK_TEST_EXPECT(object == NULL);
}

UK_TESTCASE(ukallocpool, test_stability)
{
	struct uk_allocpool *pool;
	__sz memory_needed;
	void *object_array[TEST_OBJ_COUNT];
	unsigned int i, cycle;

	static char pool_buffer[__PAGE_SIZE] __align(__PAGE_SIZE);

	memory_needed = uk_allocpool_reqmem(TEST_OBJ_COUNT, TEST_OBJ_SIZE, TEST_OBJ_ALIGN);
	pool = uk_allocpool_init(pool_buffer, memory_needed, TEST_OBJ_SIZE, TEST_OBJ_ALIGN);
	UK_TEST_ASSERT(pool != NULL);

	/* Run 3 cycles of full take/return */
	for (cycle = 0; cycle < 3; cycle++) {
		/* Fully drain */
		for (i = 0; i < TEST_OBJ_COUNT; i++) {
			object_array[i] = uk_allocpool_take(pool);
			UK_TEST_EXPECT(object_array[i] != NULL);
		}

		/* Fully return */
		for (i = 0; i < TEST_OBJ_COUNT; i++) {
			uk_allocpool_return(pool, object_array[i]);
		}

		/* Verify count is back to max */
		UK_TEST_EXPECT_SNUM_EQ(uk_allocpool_availcount(pool), TEST_OBJ_COUNT);
	}
}

uk_testsuite_register(ukallocpool, NULL);
