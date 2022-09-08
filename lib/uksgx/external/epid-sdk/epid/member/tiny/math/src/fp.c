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
/// Implementation of Fp math
/*! \file */

#include "epid/member/tiny/math/fp.h"

#include <limits.h>  // for CHAR_BIT
#include "epid/member/tiny/math/mathtypes.h"
#include "epid/member/tiny/math/serialize.h"
#include "epid/member/tiny/math/vli.h"
#include "epid/member/tiny/stdlib/tiny_stdlib.h"

/// A security parameter. In this version of Intel(R) EPID SDK, slen = 128
#define EPID_SLEN 128
/// buffer size of random integer t in INT32
#define RAND_NUM_WORDS \
  ((sizeof(FpElem) + EPID_SLEN / CHAR_BIT) / sizeof(uint32_t))

static VeryLargeInt const epid20_p = {{0xD10B500D, 0xF62D536C, 0x1299921A,
                                       0x0CDC65FB, 0xEE71A49E, 0x46E5F25E,
                                       0xFFFCF0CD, 0xFFFFFFFF}};
static FpElem const one = {{{1, 0, 0, 0, 0, 0, 0, 0}}};
static VeryLargeInt const p_minus_one = {{0xD10B500C, 0xF62D536C, 0x1299921A,
                                          0x0CDC65FB, 0xEE71A49E, 0x46E5F25E,
                                          0xFFFCF0CD, 0xFFFFFFFF}};

int FpInField(FpElem const* in) { return (VliCmp(&in->limbs, &epid20_p) < 0); }

void FpAdd(FpElem* result, FpElem const* left, FpElem const* right) {
  VliModAdd(&result->limbs, &left->limbs, &right->limbs, &epid20_p);
}

void FpMul(FpElem* result, FpElem const* left, FpElem const* right) {
  VliModMul(&result->limbs, &left->limbs, &right->limbs, &epid20_p);
}

void FpSub(FpElem* result, FpElem const* left, FpElem const* right) {
  VliModSub(&result->limbs, &left->limbs, &right->limbs, &epid20_p);
}

void FpExp(FpElem* result, FpElem const* base, VeryLargeInt const* exp) {
  VliModExp(&result->limbs, &base->limbs, exp, &epid20_p);
}

void FpNeg(FpElem* result, FpElem const* in) {
  VliCondSet(&result->limbs, &epid20_p, &in->limbs, VliIsZero(&in->limbs));
  VliSub(&result->limbs, &epid20_p, &result->limbs);
}

int FpEq(FpElem const* left, FpElem const* right) {
  return (VliCmp(&left->limbs, &right->limbs) == 0);
}

void FpInv(FpElem* result, FpElem const* in) {
  VliModInv(&result->limbs, &in->limbs, &epid20_p);
}

int FpRand(FpElem* result, BitSupplier rnd_func, void* rnd_param) {
  VeryLargeIntProduct deserialized_t = {{0}};
  uint32_t t[RAND_NUM_WORDS] = {0};
  OctStr32 const* src = (OctStr32 const*)t;

  int i;
  if (rnd_func(t, sizeof(FpElem) * CHAR_BIT + EPID_SLEN, rnd_param)) {
    return 0;
  }
  for (i = RAND_NUM_WORDS - 1; i >= 0; i--) {
    src = Uint32Deserialize(deserialized_t.word + i, src);
  }
  VliModBarrett(&result->limbs, &deserialized_t, &epid20_p);
  return 1;
}

int FpRandNonzero(FpElem* result, BitSupplier rnd_func, void* rnd_param) {
  VeryLargeIntProduct deserialized_t = {{0}};
  uint32_t t[RAND_NUM_WORDS] = {0};
  OctStr32 const* src = (OctStr32 const*)t;
  int i;
  if (rnd_func(t, sizeof(FpElem) * CHAR_BIT + EPID_SLEN, rnd_param)) {
    return 0;
  }
  for (i = RAND_NUM_WORDS - 1; i >= 0; i--) {
    src = Uint32Deserialize(deserialized_t.word + i, src);
  }
  VliModBarrett(&result->limbs, &deserialized_t, &p_minus_one);
  // (t mod(p-1)) + 1 gives number in [1,p-1]
  FpAdd(result, result, &one);
  return 1;
}

void FpClear(FpElem* result) { VliClear(&result->limbs); }

void FpSet(FpElem* result, uint32_t in) {
  FpClear(result);
  *(uint32_t*)(result->limbs.word) = in;
}

void FpFromHash(FpElem* result, unsigned char const* hash, size_t len) {
  size_t i;
  VeryLargeIntProduct vli;
  memset(&vli, 0, sizeof(vli));
  for (i = 0; i < len; i++) {
    ((uint8_t*)vli.word)[len - i - 1] = hash[i];
  }
  VliModBarrett(&result->limbs, &vli, &epid20_p);
}
