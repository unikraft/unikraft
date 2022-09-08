/*############################################################################
# Copyright 2017 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
############################################################################*/
/// A SHA-512 implementation.
/*! \file */

#include "epid/member/tiny/math/sha512.h"

#include <stdint.h>
#include "epid/member/tiny/stdlib/tiny_stdlib.h"

static void sha512_compress(uint64_t* iv, void const* in);

void tinysha512_init(sha512_state* s) {
  s->iv[0] = 0x6a09e667f3bcc908ULL;
  s->iv[1] = 0xbb67ae8584caa73bULL;
  s->iv[2] = 0x3c6ef372fe94f82bULL;
  s->iv[3] = 0xa54ff53a5f1d36f1ULL;
  s->iv[4] = 0x510e527fade682d1ULL;
  s->iv[5] = 0x9b05688c2b3e6c1fULL;
  s->iv[6] = 0x1f83d9abfb41bd6bULL;
  s->iv[7] = 0x5be0cd19137e2179ULL;

  s->bits_hashed_high = s->bits_hashed_low = (uint64_t)0;
  s->leftover_offset = 0;
}

void tinysha512_update(sha512_state* s, void const* data, size_t data_length) {
  unsigned char const* tmp_data = (unsigned char const*)data;

  while (data_length-- > 0) {
    s->leftover[s->leftover_offset++] = *(tmp_data++);
    if (s->leftover_offset >= SHA512_BLOCK_SIZE) {
      sha512_compress(s->iv, s->leftover);
      s->leftover_offset = 0;
      s->bits_hashed_low += (SHA512_BLOCK_SIZE << 3);
    }
  }
}

void tinysha512_final(unsigned char* digest, sha512_state* s) {
  size_t i, j;

  s->bits_hashed_low += (s->leftover_offset << 3);

  s->leftover[s->leftover_offset++] = 0x80;
  /* there is always room for one byte */
  if (s->leftover_offset > (sizeof(s->leftover) - 16)) {
    /* the data's bit length cannot be encoded in the last block */
    (void)memset(s->leftover + s->leftover_offset, 0x00,
                 sizeof(s->leftover) - s->leftover_offset);
    sha512_compress(s->iv, s->leftover);
    s->leftover_offset = 0;
  }

  (void)memset(s->leftover + s->leftover_offset, 0x00,
               sizeof(s->leftover) - 16 - s->leftover_offset);
  for (i = 0; i < 8; i++) {
    s->leftover[sizeof(s->leftover) - (i + 1)] =
        (unsigned char)(s->bits_hashed_low >> (8 * i));
    s->leftover[sizeof(s->leftover) - (i + 1) - 8] =
        (unsigned char)(s->bits_hashed_high >> (8 * i));
  }
  sha512_compress(s->iv, s->leftover);

  for (i = 0; i < SHA512_DIGEST_WORDS; ++i) {
    uint64_t w = s->iv[i];
    for (j = 0; j < 8; j++) {
      digest[j] = (unsigned char)(w >> (8 * (7 - j)));
    }
    digest += 8;
  }
}

#define B(x, j) \
  (((uint64_t)(*(((unsigned char const*)(&x)) + j))) << ((7 - j) * 8))

static uint64_t pull64(const uint64_t x) {
  int16_t i;
  uint64_t result = 0;
  for (i = 0; i < 8; i++) result |= B(x, i);
  return result;
}
static uint64_t ROTR(const uint64_t x, unsigned char s) {
  return (x >> s) | (x << (64 - s));
}

#define Sigma0(x) (ROTR((x), 28) ^ ROTR((x), 34) ^ ROTR((x), 39))

#define Sigma1(x) (ROTR((x), 14) ^ ROTR((x), 18) ^ ROTR((x), 41))
#define sigma0(x) (ROTR((x), 1) ^ ROTR((x), 8) ^ ((x) >> 7))
#define sigma1(x) (ROTR((x), 19) ^ ROTR((x), 61) ^ ((x) >> 6))

#define Ch(x, y, z) (((x) & (y)) ^ ((~(x)) & (z)))
#define Maj(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

static const uint64_t k512[80] = {
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL,
    0xe9b5dba58189dbbcULL, 0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL,
    0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL, 0xd807aa98a3030242ULL,
    0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL,
    0xc19bf174cf692694ULL, 0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL,
    0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL, 0x2de92c6f592b0275ULL,
    0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL,
    0xbf597fc7beef0ee4ULL, 0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
    0x06ca6351e003826fULL, 0x142929670a0e6e70ULL, 0x27b70a8546d22ffcULL,
    0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
    0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL,
    0x92722c851482353bULL, 0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL,
    0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL, 0xd192e819d6ef5218ULL,
    0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL,
    0x34b0bcb5e19b48a8ULL, 0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL,
    0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL, 0x748f82ee5defb2fcULL,
    0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL,
    0xc67178f2e372532bULL, 0xca273eceea26619cULL, 0xd186b8c721c0c207ULL,
    0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL, 0x06f067aa72176fbaULL,
    0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
    0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL,
    0x431d67c49c100d4cULL, 0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL,
    0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL};

static void sha512_compress(uint64_t* iv, void const* in) {
  const uint64_t* w = (const uint64_t*)in;
  uint64_t a, e, t;
  uint64_t work_space[80 + 9];
  uint64_t* work_ptr;
  int i;
  volatile int j;

  work_ptr = work_space + 80;
  // j is declared volatile to prevent the optimizer from replacing the for loop
  // with a memcpy
  for (j = 0; j < 8; j++) work_ptr[j] = iv[j];

  for (i = 0; i < 80; i++, work_ptr--) {
    if (i < 16) {
      t = pull64(w[i]);
    } else {
      t = sigma0(work_ptr[8 + 16 - 1]);
      t += sigma1(work_ptr[8 + 16 - 14]);
      t += work_ptr[8 + 16] + work_ptr[8 + 16 - 9];
    }
    work_ptr[8] = t;
    t += work_ptr[7] + Sigma1(work_ptr[4]) +
         Ch(work_ptr[4], work_ptr[5], work_ptr[6]) + k512[i];
    e = work_ptr[3] + t;
    a = t + Sigma0(work_ptr[0]) + Maj(work_ptr[0], work_ptr[1], work_ptr[2]);
    work_ptr[-1] = a;
    work_ptr[3] = e;
  }

  for (i = 0; i < 8; i++) iv[i] += work_ptr[i];
}
