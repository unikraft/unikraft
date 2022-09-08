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
/// Implementation of Fq math
/*! \file */

#include "epid/member/tiny/math/fq.h"

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

static VeryLargeInt const epid20_q = {{0xAED33013, 0xD3292DDB, 0x12980A82,
                                       0x0CDC65FB, 0xEE71A49F, 0x46E5F25E,
                                       0xFFFCF0CD, 0xFFFFFFFF}};

int FqInField(FqElem const* in) { return (VliCmp(&in->limbs, &epid20_q) < 0); }

void FqAdd(FqElem* result, FqElem const* left, FqElem const* right) {
  VliModAdd(&result->limbs, &left->limbs, &right->limbs, &epid20_q);
}

void FqSub(FqElem* result, FqElem const* left, FqElem const* right) {
  VliModSub(&result->limbs, &left->limbs, &right->limbs, &epid20_q);
}

void FqMul(FqElem* result, FqElem const* left, FqElem const* right) {
  VliModMul(&result->limbs, &left->limbs, &right->limbs, &epid20_q);
}

void FqExp(FqElem* result, FqElem const* base, VeryLargeInt const* exp) {
  VliModExp(&result->limbs, &base->limbs, exp, &epid20_q);
}

void FqCp(FqElem* result, FqElem const* in) {
  VliSet(&result->limbs, &in->limbs);
}

int FqIsZero(FqElem const* value) { return VliIsZero(&value->limbs); }

void FqInv(FqElem* result, FqElem const* in) {
  VliModInv(&result->limbs, &in->limbs, &epid20_q);
}

void FqNeg(FqElem* result, FqElem const* in) {
  VliCondSet(&result->limbs, &epid20_q, &in->limbs, VliIsZero(&in->limbs));
  VliSub(&result->limbs, &epid20_q, &result->limbs);
}

void FqSquare(FqElem* result, FqElem const* in) {
  VliModSquare(&result->limbs, &in->limbs, &epid20_q);
}

void FqClear(FqElem* result) { VliClear(&result->limbs); }

void FqSet(FqElem* result, uint32_t in) {
  FqClear(result);
  *(uint32_t*)(result->limbs.word) = in;
}

int FqEq(FqElem const* left, FqElem const* right) {
  return (VliCmp(&left->limbs, &right->limbs) == 0);
}

void FqCondSet(FqElem* result, FqElem const* true_val, FqElem const* false_val,
               int truth_val) {
  VliCondSet(&result->limbs, &true_val->limbs, &false_val->limbs, truth_val);
}

int FqSqrt(FqElem* result, FqElem const* in) {
  VeryLargeInt tmp;
  // Intel(R) EPID 2.0 parameter q meets q = 3 mod 4.
  // Square root can be computed as in^((q+1)/4) mod q.
  VliRShift(&tmp, &epid20_q, 2);  // tmp = (q-3)/4
  (tmp.word[0])++;                // tmp = (q+1)/4
  FqExp(result, in, &tmp);        // result = in^((q+1)/4) mod q
  // validate sqrt exists
  VliModSquare(&tmp, &result->limbs, &epid20_q);
  return 0 == VliCmp(&tmp, &in->limbs);
}

int FqRand(FqElem* result, BitSupplier rnd_func, void* rnd_param) {
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
  VliModBarrett(&result->limbs, &deserialized_t, &epid20_q);
  return 1;
}

void FqFromHash(FqElem* result, unsigned char const* hash, size_t len) {
  size_t i;
  VeryLargeIntProduct vli;
  memset(&vli, 0, sizeof(vli));
  for (i = 0; i < len; i++) {
    ((uint8_t*)vli.word)[len - i - 1] = hash[i];
  }
  VliModBarrett(&result->limbs, &vli, &epid20_q);
}
