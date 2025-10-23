/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <string.h>
#include <uk/test.h>

#include "chacha.h"

/* Implements the test vectors of RFC8439:
 * https://datatracker.ietf.org/doc/html/rfc8439
 *
 * In the quarter round and block function tests, we
 * initialize the state using the RFC layout, i.e.
 * 32-bit block counter and 96-bit nonce, which is
 * different from djb's original version that we
 * implement. For these tests the differences in the
 * layout have no impact, as the operations on the
 * block are agnostic of semantics.
 *
 * RFC layout:
 * cccccccc  cccccccc  cccccccc  cccccccc
 * kkkkkkkk  kkkkkkkk  kkkkkkkk  kkkkkkkk
 * kkkkkkkk  kkkkkkkk  kkkkkkkk  kkkkkkkk
 * bbbbbbbb  nnnnnnnn  nnnnnnnn  nnnnnnnn
 *
 * We implement the encryption tests from Appendix A.2
 * as a means of end-to-end testing. We apply a small
 * trick on the iv assignment to match the RFC block
 * layout, in order to use unmodified test vectors.
 */

#define le32(_p)			    \
	(((_p)[0]) | ((_p)[1] << 8) |	    \
	 ((_p)[2] << 16) | ((_p)[3] << 24))

/* Sect. 2.2.1 */
UK_TESTCASE(ukrandom_chacha, test_quarterround)
{
	__u32 x[16] = {0};

	x[0]  = 0x11111111;
	x[4]  = 0x01020304;
	x[8]  = 0x9b8d6f43;
	x[12] = 0x01234567;

	chacha_quarterround(x, 0, 4, 8, 12);

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

	chacha_quarterround(x, 2, 7, 8, 13);

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

	chacha_wordtobyte(out, in);

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

	chacha_wordtobyte(out, in);

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

	chacha_wordtobyte(out, in);

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

	chacha_wordtobyte(out, in);

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

	chacha_wordtobyte(out, in);

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

	chacha_wordtobyte(out, in);

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

/* Appendix A.A2 Test Vector #1 */
UK_TESTCASE(ukrandom_chacha, test_encryption_tv1)
{
	struct chacha_ctx ctx;
	__u32 key[8] = {0};
	__u32 iv[2] = {0};

	chacha_init(&ctx, key, iv, 0);

	UK_TEST_EXPECT(chacha_rand32(&ctx) == 0xade0b876);
	UK_TEST_EXPECT(chacha_rand32(&ctx) == 0x903df1a0);
	UK_TEST_EXPECT(chacha_rand32(&ctx) == 0xe56a5d40);
	UK_TEST_EXPECT(chacha_rand32(&ctx) == 0x28bd8653);
	UK_TEST_EXPECT(chacha_rand32(&ctx) == 0xb819d2bd);
	UK_TEST_EXPECT(chacha_rand32(&ctx) == 0x1aed8da0);
	UK_TEST_EXPECT(chacha_rand32(&ctx) == 0xccef36a8);
	UK_TEST_EXPECT(chacha_rand32(&ctx) == 0xc70d778b);
	UK_TEST_EXPECT(chacha_rand32(&ctx) == 0x7c5941da);
	UK_TEST_EXPECT(chacha_rand32(&ctx) == 0x8d485751);
	UK_TEST_EXPECT(chacha_rand32(&ctx) == 0x3fe02477);
	UK_TEST_EXPECT(chacha_rand32(&ctx) == 0x374ad8b8);
	UK_TEST_EXPECT(chacha_rand32(&ctx) == 0xf4b8436a);
	UK_TEST_EXPECT(chacha_rand32(&ctx) == 0x1ca11815);
	UK_TEST_EXPECT(chacha_rand32(&ctx) == 0x69b687c3);
	UK_TEST_EXPECT(chacha_rand32(&ctx) == 0x8665eeb2);
}

/* Appendix A.A2 Test Vector #2 */
UK_TESTCASE(ukrandom_chacha, test_encryption_tv2)
{
	struct chacha_ctx ctx;
	__u32 key[8] = {0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x01000000};
	__u32 iv[2] = {0x00000000, 0x02000000}; /* adapt to rfc block */

	const char *p[] = {
		"Any ", "subm", "issi", "on t",
		"o th", "e IE", "TF i", "nten",
		"ded ", "by t", "he C", "ontr",
		"ibut", "or f", "or p", "ubli",
		"cati", "on a", "s al", "l or",
		" par", "t of", " an ", "IETF",
		" Int", "erne", "t-Dr", "aft ",
		"or R", "FC a", "nd a", "ny s",
		"tate", "ment", " mad", "e wi",
		"thin", " the", " con", "text",
		" of ", "an I", "ETF ", "acti",
		"vity", " is ", "cons", "ider",
		"ed a", "n \"I", "ETF ", "Cont",
		"ribu", "tion", "\". S", "uch ",
		"stat", "emen", "ts i", "nclu",
		"de o", "ral ", "stat", "emen",
		"ts i", "n IE", "TF s", "essi",
		"ons,", " as ", "well", " as ",
		"writ", "ten ", "and ", "elec",
		"tron", "ic c", "ommu", "nica",
		"tion", "s ma", "de a", "t an",
		"y ti", "me o", "r pl", "ace,",
		" whi", "ch a", "re a", "ddre",
		"ssed",  " to"
	};

	__u32 c[] = {
		0x7df0fba3, 0xde2ffaf3, 0xa26c374f, 0x7073823e,
		0x9f5d6041, 0xbd574f4f, 0x1d2cff8c, 0xec55794b,
		0x8b94972a, 0x152972d3, 0x37d3f3c8, 0x0570d3f7,
		0xd6969e0e, 0x9fc3b747, 0xca31e056, 0x0d25b65e,
		0x27e04240, 0xfaecec85, 0xe8b54b4b, 0x0e44d0ea,
		0xdbe8b620, 0xa781d809, 0x422f13c6, 0x5079520e,
		0x77fabd42, 0x05a9d873, 0x29b34714, 0x1c41e11c,
		0x55650468, 0x05c4a62a, 0x5e4d76b7, 0x5aa8be87,
		0x49840fd0, 0xd0728fed, 0x05ab62d6, 0x66ca9126,
		0x6dc84b42, 0xa40ef82d, 0xf9ab431f, 0x9d25d337,
		0xdfd0b2c4, 0x916c8ab4, 0xf7d7dd39, 0x28e96669,
		0x3b5535e6, 0x875c6ca7, 0xd4357b9d, 0x2be6b29e,
		0xaccd7108, 0xe2398963, 0x0e1e8a5e, 0x0f28d5f9,
		0x8b32caa8, 0x763c1c35, 0xcfcb8959, 0x6c8baa3d,
		0x9faf3acc, 0x2bc97939, 0x88fc2037, 0x84ed95dc,
		0x9c05bea1, 0xfdb99964, 0xe8e736a2, 0x0b4bb018,
		0x871e9cc3, 0xfe3b196b, 0x3f756955, 0xc08c1288,
		0x639baa8a, 0x806fa1d1, 0xd75425ef, 0x1f419c18,
		0x52ca6958, 0xa33fb8c5, 0xb916f26f, 0x6200d3c1,
		0x2dfdbcbe, 0x91e0bcc5, 0xa7fd3419, 0xe6f6869a,
		0x59d7ce98, 0x649bffc3, 0x3d8f3377, 0x85cdf9a4,
		0x8299ea14, 0x41b3afcc, 0xd94d38b2, 0xabd1f302,
		0xd21dc67a, 0xba216f9c, 0x372f865b, 0xfd7ce330,
		0x6c80fdc4, 0x21f222
	};

	chacha_init(&ctx, key, iv, 1);

	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[0]))  == c[0]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[1]))  == c[1]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[2]))  == c[2]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[3]))  == c[3]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[4]))  == c[4]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[5]))  == c[5]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[6]))  == c[6]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[7]))  == c[7]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[8]))  == c[8]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[9]))  == c[9]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[10])) == c[10]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[11])) == c[11]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[12])) == c[12]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[13])) == c[13]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[14])) == c[14]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[15])) == c[15]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[16])) == c[16]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[17])) == c[17]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[18])) == c[18]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[19])) == c[19]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[20])) == c[20]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[21])) == c[21]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[22])) == c[22]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[23])) == c[23]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[24])) == c[24]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[25])) == c[25]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[26])) == c[26]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[27])) == c[27]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[28])) == c[28]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[29])) == c[29]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[30])) == c[30]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[31])) == c[31]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[32])) == c[32]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[33])) == c[33]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[34])) == c[34]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[35])) == c[35]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[36])) == c[36]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[37])) == c[37]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[38])) == c[38]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[39])) == c[39]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[40])) == c[40]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[41])) == c[41]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[42])) == c[42]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[43])) == c[43]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[44])) == c[44]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[45])) == c[45]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[46])) == c[46]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[47])) == c[47]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[48])) == c[48]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[49])) == c[49]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[50])) == c[50]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[51])) == c[51]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[52])) == c[52]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[53])) == c[53]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[54])) == c[54]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[55])) == c[55]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[56])) == c[56]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[57])) == c[57]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[58])) == c[58]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[59])) == c[59]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[60])) == c[60]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[61])) == c[61]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[62])) == c[62]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[63])) == c[63]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[64])) == c[64]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[65])) == c[65]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[66])) == c[66]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[67])) == c[67]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[68])) == c[68]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[69])) == c[69]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[70])) == c[70]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[71])) == c[71]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[72])) == c[72]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[73])) == c[73]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[74])) == c[74]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[75])) == c[75]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[76])) == c[76]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[77])) == c[77]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[78])) == c[78]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[79])) == c[79]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[80])) == c[80]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[81])) == c[81]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[82])) == c[82]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[83])) == c[83]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[84])) == c[84]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[85])) == c[85]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[86])) == c[86]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[87])) == c[87]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[88])) == c[88]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[89])) == c[89]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[90])) == c[90]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[91])) == c[91]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[92])) == c[92]);
	UK_TEST_EXPECT(((chacha_rand32(&ctx) ^ le32(p[93])) & 0xffffff) == c[93]);
}

UK_TESTCASE(ukrandom_chacha, test_encryption_tv3)
{
	struct chacha_ctx ctx;
	__u32 key[8] = {0xa540921c, 0x8ad355eb, 0x868833f3, 0xf0b5f604,
			0xc1173947, 0x09802b40, 0xbc5cca9d, 0xc0757020};
	__u32 iv[2] = {0x00000000, 0x02000000}; /* adapt to rfc block */

	char *p[] = {
		"'Twa",  "s br",  "illi", "g, a",
		"nd t",  "he s",  "lith", "y to",
		"ves\n", "Did ",  "gyre", " and",
		" gim",  "ble ",  "in t", "he w",
		"abe:",  "\nAll", " mim", "sy w",
		"ere ",  "the ",  "boro", "gove",
		"s,\nA", "nd t",  "he m", "ome ",
		"rath",  "s ou",  "tgra", "be.",
	};

	__u32 c[] = {
		0x7f34e662, 0xa487ed95, 0x42e7fa5f, 0xdfa1276f,
		0x1091b65f, 0x730d4c04, 0xa9ff8e11, 0xcfe5015b,
		0xf23d6d16, 0xf9ca21d7, 0xb15f1eb2, 0x7168614c,
		0x4fc584fd, 0x83b2659d, 0xe47f6c19, 0xeb5305f6,
		0x02649cf3, 0xe33422c4, 0x3e6b352a, 0xa6124376,
		0x0532551a, 0xd6ea1657, 0xf8682596, 0x773f3f7d,
		0xd1a8c604, 0x4dbfd1bc, 0x4b15d650, 0xb131a76d,
		0xfd8db587, 0x36fa8a72, 0x7a797a75, 0xd188c1
	};
	chacha_init(&ctx, key, iv, 42);

	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[0]))  == c[0]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[1]))  == c[1]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[2]))  == c[2]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[3]))  == c[3]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[4]))  == c[4]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[5]))  == c[5]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[6]))  == c[6]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[7]))  == c[7]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[8]))  == c[8]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[9]))  == c[9]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[10])) == c[10]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[11])) == c[11]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[12])) == c[12]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[13])) == c[13]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[14])) == c[14]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[15])) == c[15]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[16])) == c[16]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[17])) == c[17]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[18])) == c[18]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[19])) == c[19]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[20])) == c[20]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[21])) == c[21]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[22])) == c[22]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[23])) == c[23]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[24])) == c[24]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[25])) == c[25]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[26])) == c[26]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[27])) == c[27]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[28])) == c[28]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[29])) == c[29]);
	UK_TEST_EXPECT((chacha_rand32(&ctx) ^ le32(p[30])) == c[30]);
	UK_TEST_EXPECT(((chacha_rand32(&ctx) ^ le32(p[31])) & 0xffffff) == c[31]);
}

uk_testsuite_register(ukrandom_chacha, NULL);
