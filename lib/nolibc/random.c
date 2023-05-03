/* SPDX-License-Identifier: MIT */
/*
 * The following implementation is based on the musl implementation.
 *
 * Copyright © 2005-2020 Rich Felker, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <stdint.h>

/*
 * this code uses the same lagged fibonacci generator as the
 * original bsd random implementation except for the seeding
 * which was broken in the original
 */

static uint32_t init[] = {
	0x00000000, 0x5851f42d, 0xc0b18ccf, 0xcbb5f646,
	0xc7033129, 0x30705b04, 0x20fd5db4, 0x9a8b7f78,
	0x502959d8, 0xab894868, 0x6c0356a7, 0x88cdb7ff,
	0xb477d43f, 0x70a3a52b, 0xa8e4baf1, 0xfd8341fc,
	0x8ae16fd9, 0x742d2f7a, 0x0d1f0796, 0x76035e09,
	0x40f7702c, 0x6fa72ca5, 0xaaa84157, 0x58a0df74,
	0xc74a0364, 0xae533cc4, 0x04185faf, 0x6de3b115,
	0x0cab8628, 0xf043bfa4, 0x398150e9, 0x37521657};

static int n = 31;
static int i = 3;
static int j;
static uint32_t *x = init+1;

static uint32_t lcg31(uint32_t x)
{
	return (1103515245*x + 12345) & 0x7fffffff;
}

static uint64_t lcg64(uint64_t x)
{
	return 6364136223846793005ull*x + 1;
}

static void *savestate()
{
	x[-1] = (n<<16)|(i<<8)|j;
	return x-1;
}

static void loadstate(uint32_t *state)
{
	x = state+1;
	n = x[-1]>>16;
	i = (x[-1]>>8)&0xff;
	j = x[-1]&0xff;
}

static void __srandom(unsigned int seed)
{
	int k;
	uint64_t s = seed;

	if (n == 0) {
		x[0] = s;
		return;
	}
	i = n == 31 || n == 7 ? 3 : 1;
	j = 0;
	for (k = 0; k < n; k++) {
		s = lcg64(s);
		x[k] = s>>32;
	}
	/* make sure x contains at least one odd number */
	x[0] |= 1;
}

void srandom(unsigned int seed)
{
	__srandom(seed);
}

char *initstate(unsigned int seed, char *state, size_t size)
{
	void *old;

	if (size < 8)
		return 0;
	old = savestate();
	if (size < 32)
		n = 0;
	else if (size < 64)
		n = 7;
	else if (size < 128)
		n = 15;
	else if (size < 256)
		n = 31;
	else
		n = 63;
	x = (uint32_t *)state + 1;
	__srandom(seed);
	savestate();
	return old;
}

char *setstate(char *state)
{
	void *old;

	old = savestate();
	loadstate((uint32_t *)state);
	return old;
}

long random(void)
{
	long k;

	if (n == 0) {
		k = x[0] = lcg31(x[0]);
		goto end;
	}
	x[i] += x[j];
	k = x[i]>>1;
	if (++i == n)
		i = 0;
	if (++j == n)
		j = 0;
end:
	return k;
}
