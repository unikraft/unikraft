/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <string.h>
#include <uk/test.h>

/* Implements the test vectors of RFC8439:
 * https://datatracker.ietf.org/doc/html/rfc8439
 *
 * In the block tests we initialize the state using
 * the RFC8439 layout, i.e. 128-bit block and 128-bit
 * nonce, whichis different than the original version of
 * djb that we use. For these test the layout difference
 * doesn't make a difference, as the operations of the
 * block function are agnostic of semantics.
 *
 * RFC layout:
 * cccccccc  cccccccc  cccccccc  cccccccc
 * kkkkkkkk  kkkkkkkk  kkkkkkkk  kkkkkkkk
 * kkkkkkkk  kkkkkkkk  kkkkkkkk  kkkkkkkk
 * bbbbbbbb  nnnnnnnn  nnnnnnnn  nnnnnnnn
 *
 */

void uk_quarterround(__u32 x[16], int a, int b, int c, int d);
void uk_salsa20_wordtobyte(__u32 output[16], const __u32 in[16]);

/* Sect. 2.2.1 */
UK_TESTCASE(ukrandom_chacha, test_quarterround)
{
	__u32 x[16] = {0};

	x[0]  = 0x11111111;
	x[4]  = 0x01020304;
	x[8]  = 0x9b8d6f43;
	x[12] = 0x01234567;

	uk_quarterround(x, 0, 4, 8, 12);

	UK_TEST_EXPECT(x[0]  == 0xea2a92f4);
	UK_TEST_EXPECT(x[4]  == 0xcb1cf8ce);
	UK_TEST_EXPECT(x[8]  == 0x4581472e);
	UK_TEST_EXPECT(x[12] == 0x5881c4bb);
}

/* Sect. 2.2.1 */
UK_TESTCASE(ukrandom_chacha, test_quarterround_state)
{
	__u32 x[16] = {0x879531e0, 0xc5ecf37d, 0x516461b1, 0xc9a62f8a,
			0x44c20ef3, 0x3390af7f, 0xd9fc690b, 0x2a5f714c,
			0x53372767, 0xb00a5631, 0x974c541a, 0x359e9963,
			0x5c971061, 0x3d631689, 0x2098d9d6, 0x91dbd320};

	uk_quarterround(x, 2, 7, 8, 13);

	UK_TEST_EXPECT(x[0] == 0x879531e0);
	UK_TEST_EXPECT(x[1] == 0xc5ecf37d);
	UK_TEST_EXPECT(x[2] == 0xbdb886dc);
	UK_TEST_EXPECT(x[3] == 0xc9a62f8a);
	UK_TEST_EXPECT(x[4] == 0x44c20ef3);
	UK_TEST_EXPECT(x[5] == 0x3390af7f);
	UK_TEST_EXPECT(x[6] == 0xd9fc690b);
	UK_TEST_EXPECT(x[7] == 0xcfacafd2);
	UK_TEST_EXPECT(x[8] == 0xe46bea80);
	UK_TEST_EXPECT(x[9] == 0xb00a5631);
	UK_TEST_EXPECT(x[10] == 0x974c541a);
	UK_TEST_EXPECT(x[11] == 0x359e9963);
	UK_TEST_EXPECT(x[12] == 0x5c971061);
	UK_TEST_EXPECT(x[13] == 0xccc07c79);
	UK_TEST_EXPECT(x[14] == 0x2098d9d6);
	UK_TEST_EXPECT(x[15] == 0x91dbd320);
}

/* Sect. 2.3.2 */
UK_TESTCASE(ukrandom_chacha, test_block_fn)
{
	__u32 in[16] = {0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
			0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c,
			0x13121110, 0x17161514, 0x1b1a1918, 0x1f1e1d1c,
			0x00000001, 0x09000000, 0x4a000000, 0x00000000};
	__u32 out[16];

	uk_salsa20_wordtobyte(out, in);

	UK_TEST_EXPECT(out[0] == 0xe4e7f110);
	UK_TEST_EXPECT(out[1] == 0x15593bd1);
	UK_TEST_EXPECT(out[2] == 0x1fdd0f50);
	UK_TEST_EXPECT(out[3] == 0xc47120a3);
	UK_TEST_EXPECT(out[4] == 0xc7f4d1c7);
	UK_TEST_EXPECT(out[5] == 0x0368c033);
	UK_TEST_EXPECT(out[6] == 0x9aaa2204);
	UK_TEST_EXPECT(out[7] == 0x4e6cd4c3);
	UK_TEST_EXPECT(out[8] == 0x466482d2);
	UK_TEST_EXPECT(out[9] == 0x09aa9f07);
	UK_TEST_EXPECT(out[10] == 0x05d7c214);
	UK_TEST_EXPECT(out[11] == 0xa2028bd9);
	UK_TEST_EXPECT(out[12] == 0xd19c12b5);
	UK_TEST_EXPECT(out[13] == 0xb94e16de);
	UK_TEST_EXPECT(out[14] == 0xe883d0cb);
	UK_TEST_EXPECT(out[15] == 0x4e3c50a2);
}

/* Appendix A.A1 Test Vector #1 */
UK_TESTCASE(ukrandom_chacha, test_block_fn_tv1)
{
	__u32 in[16] = {0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000};
	__u32 out[16];

	uk_salsa20_wordtobyte(out, in);

	UK_TEST_EXPECT(out[0] == 0xade0b876);
	UK_TEST_EXPECT(out[1] == 0x903df1a0);
	UK_TEST_EXPECT(out[2] == 0xe56a5d40);
	UK_TEST_EXPECT(out[3] == 0x28bd8653);
	UK_TEST_EXPECT(out[4] == 0xb819d2bd);
	UK_TEST_EXPECT(out[5] == 0x1aed8da0);
	UK_TEST_EXPECT(out[6] == 0xccef36a8);
	UK_TEST_EXPECT(out[7] == 0xc70d778b);
	UK_TEST_EXPECT(out[8] == 0x7c5941da);
	UK_TEST_EXPECT(out[9] == 0x8d485751);
	UK_TEST_EXPECT(out[10] == 0x3fe02477);
	UK_TEST_EXPECT(out[11] == 0x374ad8b8);
	UK_TEST_EXPECT(out[12] == 0xf4b8436a);
	UK_TEST_EXPECT(out[13] == 0x1ca11815);
	UK_TEST_EXPECT(out[14] == 0x69b687c3);
	UK_TEST_EXPECT(out[15] == 0x8665eeb2);
}

/* Appendix A.A1  Test Vector #2 */
UK_TESTCASE(ukrandom_chacha, test_block_fn_tv2)
{
	__u32 in[16] = {0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000001, 0x00000000, 0x00000000, 0x00000000};
	__u32 out[16];

	uk_salsa20_wordtobyte(out, in);

	UK_TEST_EXPECT(out[0] == 0xbee7079f);
	UK_TEST_EXPECT(out[1] == 0x7a385155);
	UK_TEST_EXPECT(out[2] == 0x7c97ba98);
	UK_TEST_EXPECT(out[3] == 0x0d082d73);
	UK_TEST_EXPECT(out[4] == 0xa0290fcb);
	UK_TEST_EXPECT(out[5] == 0x6965e348);
	UK_TEST_EXPECT(out[6] == 0x3e53c612);
	UK_TEST_EXPECT(out[7] == 0xed7aee32);
	UK_TEST_EXPECT(out[8] == 0x7621b729);
	UK_TEST_EXPECT(out[9] == 0x434ee69c);
	UK_TEST_EXPECT(out[10] == 0xb03371d5);
	UK_TEST_EXPECT(out[11] == 0xd539d874);
	UK_TEST_EXPECT(out[12] == 0x281fed31);
	UK_TEST_EXPECT(out[13] == 0x45fb0a51);
	UK_TEST_EXPECT(out[14] == 0x1f0ae1ac);
	UK_TEST_EXPECT(out[15] == 0x6f4d794b);
}

/* Appendix A.A1 Test Vector #3 */
UK_TESTCASE(ukrandom_chacha, test_block_fn_tv3)
{
	__u32 in[16] = {0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x01000000,
			0x00000001, 0x00000000, 0x00000000, 0x00000000};
	__u32 out[16];

	uk_salsa20_wordtobyte(out, in);

	UK_TEST_EXPECT(out[0] == 0x2452eb3a);
	UK_TEST_EXPECT(out[1] == 0x9249f8ec);
	UK_TEST_EXPECT(out[2] == 0x8d829d9b);
	UK_TEST_EXPECT(out[3] == 0xddd4ceb1);
	UK_TEST_EXPECT(out[4] == 0xe8252083);
	UK_TEST_EXPECT(out[5] == 0x60818b01);
	UK_TEST_EXPECT(out[6] == 0xf38422b8);
	UK_TEST_EXPECT(out[7] == 0x5aaa49c9);
	UK_TEST_EXPECT(out[8] == 0xbb00ca8e);
	UK_TEST_EXPECT(out[9] == 0xda3ba7b4);
	UK_TEST_EXPECT(out[10] == 0xc4b592d1);
	UK_TEST_EXPECT(out[11] == 0xfdf2732f);
	UK_TEST_EXPECT(out[12] == 0x4436274e);
	UK_TEST_EXPECT(out[13] == 0x2561b3c8);
	UK_TEST_EXPECT(out[14] == 0xebdd4aa6);
	UK_TEST_EXPECT(out[15] == 0xa0136c00);
}

/* Appendix A.A1 Test Vector #4 */
UK_TESTCASE(ukrandom_chacha, test_block_fn_tv4)
{
	__u32 in[16] = {0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
			0x0000ff00, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000002, 0x00000000, 0x00000000, 0x00000000};
	__u32 out[16];

	uk_salsa20_wordtobyte(out, in);

	UK_TEST_EXPECT(out[0] == 0xfb4dd572);
	UK_TEST_EXPECT(out[1] == 0x4bc42ef1);
	UK_TEST_EXPECT(out[2] == 0xdf922636);
	UK_TEST_EXPECT(out[3] == 0x327f1394);
	UK_TEST_EXPECT(out[4] == 0xa78dea8f);
	UK_TEST_EXPECT(out[5] == 0x5e269039);
	UK_TEST_EXPECT(out[6] == 0xa1bebbc1);
	UK_TEST_EXPECT(out[7] == 0xcaf09aae);
	UK_TEST_EXPECT(out[8] == 0xa25ab213);
	UK_TEST_EXPECT(out[9] == 0x48a6b46c);
	UK_TEST_EXPECT(out[10] == 0x1b9d9bcb);
	UK_TEST_EXPECT(out[11] == 0x092c5be6);
	UK_TEST_EXPECT(out[12] == 0x546ca624);
	UK_TEST_EXPECT(out[13] == 0x1bec45d5);
	UK_TEST_EXPECT(out[14] == 0x87f47473);
	UK_TEST_EXPECT(out[15] == 0x96f0992e);
}

/* Appendix A.A1 Test Vector #5 */
UK_TESTCASE(ukrandom_chacha, test_block_fn_tv5)
{
	__u32 in[16] = {0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x02000000};
	__u32 out[16];

	uk_salsa20_wordtobyte(out, in);

	UK_TEST_EXPECT(out[0] == 0x374dc6c2);
	UK_TEST_EXPECT(out[1] == 0x3736d58c);
	UK_TEST_EXPECT(out[2] == 0xb904e24a);
	UK_TEST_EXPECT(out[3] == 0xcd3f93ef);
	UK_TEST_EXPECT(out[4] == 0x88228b1a);
	UK_TEST_EXPECT(out[5] == 0x96a4dfb3);
	UK_TEST_EXPECT(out[6] == 0x5b76ab72);
	UK_TEST_EXPECT(out[7] == 0xc727ee54);
	UK_TEST_EXPECT(out[8] == 0x0e0e978a);
	UK_TEST_EXPECT(out[9] == 0xf3145c95);
	UK_TEST_EXPECT(out[10] == 0x1b748ea8);
	UK_TEST_EXPECT(out[11] == 0xf786c297);
	UK_TEST_EXPECT(out[12] == 0x99c28f5f);
	UK_TEST_EXPECT(out[13] == 0x628314e8);
	UK_TEST_EXPECT(out[14] == 0x398a19fa);
	UK_TEST_EXPECT(out[15] == 0x6ded1b53);
}

uk_testsuite_register(ukrandom_chacha, NULL);
