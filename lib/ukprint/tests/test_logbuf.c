/* SPDX-License-Identifier: BSD-3-Clause */
/* Test for circular log buffer implementation */

#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include <uk/random.h>
#include <uk/test.h>

#include "../logbuf.h"

#define MIN_MSG_LEN		10
#define MAX_MSG_LEN		256

/* overrides MAX_MSG_LEN in test_buffer_wraparound() */
#define WRAP_MSG_LEN		(CONFIG_LIBUKPRINT_LOGBUF_SIZE / 4)

#define STRESS_TEST_COUNT	1000

static UK_LOGBUF(uk_logbuf_test);

static long gen_random(void)
{
	unsigned long buf;
	int rc;

	rc = uk_random_fill_buffer(&buf, 1);
	UK_ASSERT(!rc);

	return buf;
}

/* Generate a random message of specified length */
static char *testbuf_gen_random_msg(size_t len)
{
	char *msg;
	__sz i;

	msg = malloc(len + 1);
	UK_ASSERT(msg);

	for (i = 0; i < len; i++)
		msg[i] = 'A' + (gen_random() % 26);
	msg[len] = '\0';

	return msg;
}

/* Flush test buffer. Assumes all messages statically
 * allocated, or managed by caller otherwise.
 */
static inline void testbuf_flush(void)
{
	uk_logbuf_test.head = NULL;
	uk_logbuf_test.tail = NULL;
}

/* Write a message to the log buffer */
static void testbuf_write(const char *fmt, ...)
{
	struct uk_print_msg msg;
	va_list ap;

	va_start(ap, fmt);
	UK_PRINT_MSG_ALLOC(msg, UK_PRINT_RAW, uk_libid_self(),
			   __STR_BASENAME__, __LINE__, fmt, ap);

	uk_logbuf_write(&uk_logbuf_test, &msg);

	UK_PRINT_MSG_FREE(msg);
	va_end(ap);
}

/* Count messages in the log buffer */
static int testbuf_count(void)
{
	struct uk_logbuf_msg *lm;
	int count = 0;

	if (uk_logbuf_test.head)
		uk_logbuf_foreach(&uk_logbuf_test, lm)
			count++;
	return count;
}

/* Compare logbuf messages to array of strings */
static __bool testbuf_equal(const char *str_vec[])
{
	struct uk_logbuf_msg *lm;
	int i = 0;

	uk_logbuf_foreach(&uk_logbuf_test, lm)
		if (strcmp(lm->msg.msg, str_vec[i++]))
			return false;
	return true;
}

/* Check logbuf contains last N messages */
static __bool testbuf_contains(const char *str_vec[], size_t vec_len)
{
	struct uk_logbuf_msg *lm;
	size_t i = 0;

	lm = uk_logbuf_test.head;
	UK_ASSERT(lm);

	/* Locate the message in the vector the buffer's
	 * head points to. Then make sure that rest of the
	 * messages in the vector match the rest of the
	 * messages in the logbuffer.
	 */
	for (i = 0; i < vec_len; i++)
		if (!strcmp(lm->msg.msg, str_vec[i]))
			return testbuf_equal(&str_vec[i]);

	return false;
}

/* Check logbuf's last msg matches msg */
static __bool testbuf_last(const char *msg)
{
	struct uk_logbuf_msg *lm;

	lm = uk_logbuf_test.tail;
	return strcmp(lm->msg.msg, msg) ? false : true;
}

/* Test: Empty buf */
UK_TESTCASE(logbuf_testsuite, test_empty_buf)
{
	testbuf_flush();
	UK_TEST_EXPECT_SNUM_EQ(testbuf_count(), 0);
}

/* Basic single message write and read */
UK_TESTCASE(logbuf_testsuite, test_single_message)
{
	const char *str_vec[] = {"Test message"};

	testbuf_flush();

	testbuf_write("%s", str_vec[0]);
	UK_TEST_EXPECT_SNUM_EQ(testbuf_count(), 1);
	UK_TEST_EXPECT(testbuf_equal(str_vec));
}

/* Test: Multiple messages with different lengths */
UK_TESTCASE(logbuf_testsuite, test_varied_length_messages)
{
	const char *str_vec[] = {
		"Short",
		"A medium length message here",
		"A much longer message that contains more text",
		"X",
		"Another message with some content to fill the buffer"
	};
	int str_count = ARRAY_SIZE(str_vec);
	int msg_count;
	int i;

	testbuf_flush();
	for (i = 0; i < str_count; i++)
		testbuf_write(str_vec[i]);

	msg_count = testbuf_count();
	UK_TEST_EXPECT_SNUM_EQ(msg_count, str_count);
	UK_TEST_EXPECT(testbuf_equal(str_vec));
}

/* Test: Random length messages with possible wrap around */
UK_TESTCASE(logbuf_testsuite, test_random_length_messages)
{
	char *msg_vec[50];
	size_t msg_len;
	size_t i;

	testbuf_flush();

	for (i = 0; i < ARRAY_SIZE(msg_vec); i++) {
		msg_len = MIN_MSG_LEN +
			  (gen_random() % (MAX_MSG_LEN - MIN_MSG_LEN));
		msg_vec[i] = testbuf_gen_random_msg(msg_len);

		testbuf_write(msg_vec[i]);
		UK_TEST_EXPECT(testbuf_last(msg_vec[i]));

		/* Verify integrity every 10 iterations */
		if (i % 10 == 0)
			UK_TEST_EXPECT(testbuf_contains((const char **)msg_vec,
							i + 1));
	}

	/* Final verification */
	UK_TEST_EXPECT(testbuf_contains((const char **)msg_vec, i + 1));
	UK_TEST_EXPECT_SNUM_GT(testbuf_count(), 0);
	UK_TEST_EXPECT_SNUM_LT(testbuf_count(), ARRAY_SIZE(msg_vec) + 1);
}

/* Test: Buffer wraparound with large messages */
UK_TESTCASE(logbuf_testsuite, test_buffer_wraparound)
{
	__sz msg_len = WRAP_MSG_LEN;
	char *msg_vec[20];
	size_t i;

	testbuf_flush();

	/* Write large enough messages to force wraparound */
	for (i = 0; i < ARRAY_SIZE(msg_vec); i++) {
		msg_vec[i] = testbuf_gen_random_msg(msg_len);
		testbuf_write(msg_vec[i]);
		UK_TEST_EXPECT(testbuf_last(msg_vec[i]));
	}
	UK_TEST_EXPECT(testbuf_contains((const char **)msg_vec, i + 1));
	UK_TEST_EXPECT_SNUM_GT(testbuf_count(), 0);
	UK_TEST_EXPECT_SNUM_LE(testbuf_count(), ARRAY_SIZE(msg_vec));
}

/* Test: Stress test with many random messages */
UK_TESTCASE(logbuf_testsuite, test_stress_random)
{
	char *msg_vec[STRESS_TEST_COUNT];
	size_t msg_len;
	size_t i;

	testbuf_flush();

	for (i = 0; i < ARRAY_SIZE(msg_vec); i++) {
		msg_len = MIN_MSG_LEN +
			  (gen_random() % (MAX_MSG_LEN - MIN_MSG_LEN));
		msg_vec[i] = testbuf_gen_random_msg(msg_len);

		testbuf_write(msg_vec[i]);
		UK_TEST_EXPECT(testbuf_last(msg_vec[i]));

		/* Verify integrity every 1/10th messages to reduce flood */
		if (i % (ARRAY_SIZE(msg_vec) / 10) == 0)
			UK_TEST_EXPECT(testbuf_contains((const char **)msg_vec,
							i + 1));
	}

	/* Final verification */
	UK_TEST_EXPECT(testbuf_contains((const char **)msg_vec, i + 1));
	UK_TEST_EXPECT_SNUM_GT(testbuf_count(), 0);
	UK_TEST_EXPECT_SNUM_LE(testbuf_count(), ARRAY_SIZE(msg_vec));
}

uk_testsuite_register(logbuf_testsuite, NULL);
