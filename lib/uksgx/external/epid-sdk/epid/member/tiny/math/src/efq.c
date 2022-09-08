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
/// Implementation of EFq math
/*! \file */

#include "epid/member/tiny/math/efq.h"
#include "epid/member/tiny/math/fq.h"
#include "epid/member/tiny/math/hashwrap.h"
#include "epid/member/tiny/math/mathtypes.h"
#include "epid/member/tiny/math/serialize.h"
#include "epid/member/tiny/math/vli.h"
#include "epid/member/tiny/stdlib/tiny_stdlib.h"

static int EFqMakePoint(EccPointFq* output, FqElem* in) {
  FqElem fq_sqrt = {0};
  FqElem fq_tmp = {0};
  FqSet(&fq_tmp, 3);
  FqSquare(&fq_sqrt, in);
  FqMul(&fq_sqrt, &fq_sqrt, in);
  FqAdd(&fq_sqrt, &fq_sqrt, &fq_tmp);
  if (!FqSqrt(&output->y, &fq_sqrt)) {
    return 0;
  }
  FqCp(&output->x, in);

  return 1;
}

void EFqMulSSCM(EccPointJacobiFq* result, EccPointJacobiFq const* base,
                FpElem const* exp) {
  EccPointJacobiFq efqj_1;
  EccPointJacobiFq efqj_2;

  uint32_t position;
  EFqInf(&efqj_1);
  EFqJCp(&efqj_2, base);
  for (position = 32 * NUM_ECC_DIGITS; position > 0; --position) {
    EFqDbl(&efqj_1, &efqj_1);
    EFqAdd(&efqj_2, base, &efqj_1);
    EFqCondSet(&efqj_1, &efqj_2, &efqj_1,
               VliTestBit(&(exp->limbs), position - 1));
  }
  EFqJCp(result, &efqj_1);
}

int EFqAffineExp(EccPointFq* result, EccPointFq const* base,
                 FpElem const* exp) {
  EccPointJacobiFq efqj;
  EFqFromAffine(&efqj, base);
  EFqMulSSCM(&efqj, &efqj, exp);
  return EFqToAffine(result, &efqj);
}

int EFqAffineMultiExp(EccPointFq* result, EccPointFq const* base0,
                      FpElem const* exp0, EccPointFq const* base1,
                      FpElem const* exp1) {
  EccPointJacobiFq efqj_base0;
  EccPointJacobiFq efqj_base1;
  EFqFromAffine(&efqj_base0, base0);
  EFqFromAffine(&efqj_base1, base1);
  EFqMultiExp(&efqj_base0, &efqj_base0, exp0, &efqj_base1, exp1);
  return EFqToAffine(result, &efqj_base0);
}

void EFqMultiExp(EccPointJacobiFq* result, EccPointJacobiFq const* base0,
                 FpElem const* exp0, EccPointJacobiFq const* base1,
                 FpElem const* exp1) {
  EccPointJacobiFq efqj_base0;
  EccPointJacobiFq efqj_a;
  EccPointJacobiFq efqj_b;
  int i, j;

  EFqAdd(&efqj_base0, base0, base1);
  EFqInf(&efqj_a);
  for (i = NUM_ECC_DIGITS - 1; i >= 0; i--) {
    for (j = 31; j >= 0; j--) {
      EFqAdd(&efqj_a, &efqj_a, &efqj_a);
      EFqInf(&efqj_b);
      EFqJCp(&efqj_b, base0);

      EFqCondSet(&efqj_b, base1, &efqj_b,
                 (int)((exp1->limbs.word[i] >> j) & (0x1)));

      EFqCondSet(&efqj_b, &efqj_base0, &efqj_b,
                 (int)((exp0->limbs.word[i] >> j) & (exp1->limbs.word[i] >> j) &
                       (0x1)));

      EFqAdd(&efqj_b, &efqj_a, &efqj_b);

      EFqCondSet(&efqj_a, &efqj_b, &efqj_a, (int)(((exp0->limbs.word[i] >> j) |
                                                   (exp1->limbs.word[i] >> j)) &
                                                  (0x1)));
    }
  }
  EFqJCp(result, &efqj_a);
}

int EFqAffineAdd(EccPointFq* result, EccPointFq const* left,
                 EccPointFq const* right) {
  EccPointJacobiFq efqj_a;
  EccPointJacobiFq efqj_b;
  if (EFqEqAffine(left, right)) return EFqAffineDbl(result, left);

  EFqFromAffine(&efqj_a, left);
  EFqFromAffine(&efqj_b, right);
  EFqAdd(&efqj_a, &efqj_a, &efqj_b);

  return EFqToAffine(result, &efqj_a);
}

int EFqAffineDbl(EccPointFq* result, EccPointFq const* in) {
  EccPointJacobiFq efqj_a;
  EFqFromAffine(&efqj_a, in);
  EFqAdd(&efqj_a, &efqj_a, &efqj_a);

  return EFqToAffine(result, &efqj_a);
}

void EFqDbl(EccPointJacobiFq* result, EccPointJacobiFq const* in) {
  FqElem fq_a;
  FqElem fq_b;

  FqAdd(&(result->Z), &(in->Z), &(in->Z));
  FqMul(&(result->Z), &(result->Z), &(in->Y));
  FqSquare(&fq_a, &(in->X));
  FqAdd(&fq_b, &fq_a, &fq_a);
  FqAdd(&fq_b, &fq_b, &fq_a);
  FqSquare(&fq_a, &(in->Y));
  FqAdd(&fq_a, &fq_a, &fq_a);
  FqSquare(&(result->Y), &fq_a);
  FqAdd(&(result->Y), &(result->Y), &(result->Y));
  FqAdd(&fq_a, &fq_a, &fq_a);
  FqMul(&fq_a, &fq_a, &(in->X));
  FqSquare(&(result->X), &fq_b);
  FqSub(&(result->X), &(result->X), &fq_a);
  FqSub(&(result->X), &(result->X), &fq_a);
  FqSub(&fq_a, &fq_a, &(result->X));
  FqMul(&fq_a, &fq_a, &fq_b);
  FqSub(&(result->Y), &fq_a, &(result->Y));
}

void EFqAdd(EccPointJacobiFq* result, EccPointJacobiFq const* left,
            EccPointJacobiFq const* right) {
  FqElem fq0;
  FqElem fq1;
  FqElem fq2;
  FqElem fq3;
  FqElem fq4;
  FqElem fq5;

  if (FqIsZero(&(left->Z))) {
    EFqJCp(result, right);
    return;
  }
  if (FqIsZero(&(right->Z))) {
    EFqJCp(result, left);
    return;
  }

  FqSquare(&fq2, &(right->Z));
  FqSquare(&fq3, &(left->Z));
  // P.X * Q.Z^2
  FqMul(&fq0, &(left->X), &fq2);
  // Q.X * P.Z^2
  FqMul(&fq1, &(right->X), &fq3);
  // Q.X*P.Z^2 - P*X+Q*Z^2
  FqSub(&fq5, &fq1, &fq0);
  FqMul(&fq3, &(right->Y), &fq3);
  // P.Y * Q.Z^3
  FqMul(&fq3, &(left->Z), &fq3);
  FqMul(&fq2, &(left->Y), &fq2);
  // Q.Y * P.Z^3
  FqMul(&fq2, &(right->Z), &fq2);
  FqSub(&fq4, &fq3, &fq2);

  if (FqIsZero(&fq5)) {
    if (FqIsZero(&fq4)) {
      EFqDbl(result, left);
      return;
    } else {
      EFqInf(result);
      return;
    }
  }
  FqMul(&(result->Z), &(left->Z), &(right->Z));
  FqMul(&(result->Z), &(result->Z), &fq5);
  // Q.X*P.Z^2 + P*X+Q*Z^2
  FqAdd(&fq1, &fq0, &fq1);
  FqMul(&fq2, &fq2, &fq5);
  FqSquare(&fq5, &fq5);
  FqMul(&fq1, &fq1, &fq5);
  FqSquare(&(result->X), &fq4);
  FqSub(&(result->X), &(result->X), &fq1);
  FqMul(&fq2, &fq2, &fq5);
  FqMul(&fq0, &fq0, &fq5);
  FqSub(&fq0, &fq0, &(result->X));
  FqMul(&(result->Y), &fq4, &fq0);
  FqSub(&(result->Y), &(result->Y), &fq2);
}

int EFqRand(EccPointFq* result, BitSupplier rnd_func, void* rnd_param) {
  FqElem fq;
  do {
    if (!FqRand(&fq, rnd_func, rnd_param)) {
      return 0;
    }
  } while (!EFqMakePoint(result, &fq));
  return 1;
}

void EFqSet(EccPointJacobiFq* result, FqElem const* x, FqElem const* y) {
  FqCp(&result->X, x);
  FqCp(&result->Y, y);
  FqSet(&result->Z, 1);
}

int EFqIsInf(EccPointJacobiFq const* in) {
  return FqIsZero(&in->X) && FqIsZero(&in->Z) && (!FqIsZero(&in->Y));
}

void EFqFromAffine(EccPointJacobiFq* result, EccPointFq const* in) {
  FqCp(&result->X, &in->x);
  FqCp(&result->Y, &in->y);
  FqSet(&result->Z, 1);
}

int EFqToAffine(EccPointFq* result, EccPointJacobiFq const* in) {
  FqElem fq_inv;
  if (EFqIsInf(in)) {
    return 0;
  }
  FqInv(&fq_inv, &in->Z);
  FqMul(&result->x, &in->X, &fq_inv);
  FqMul(&result->x, &result->x, &fq_inv);
  FqMul(&result->y, &in->Y, &fq_inv);
  FqMul(&result->y, &result->y, &fq_inv);
  FqMul(&result->y, &result->y, &fq_inv);

  return 1;
}

void EFqNeg(EccPointJacobiFq* result, EccPointJacobiFq const* in) {
  FqCp(&result->X, &in->X);
  FqNeg(&result->Y, &in->Y);
  FqCp(&result->Z, &in->Z);
}

int EFqEq(EccPointJacobiFq const* left, EccPointJacobiFq const* right) {
  FqElem fq1;
  FqElem fq2;
  FqElem fq3;
  FqElem fq4;
  if (EFqIsInf(left) && EFqIsInf(right)) {
    return 1;
  }
  if (EFqIsInf(left) || EFqIsInf(right)) {
    return 0;
  }
  // Z1^2
  FqSquare(&fq1, &left->Z);
  // Z2^2
  FqSquare(&fq2, &right->Z);
  // (Z1^2)*X2
  FqMul(&fq3, &fq1, &right->X);
  // (Z2^2)*X1
  FqMul(&fq4, &fq2, &left->X);
  // Z1^3
  FqMul(&fq1, &fq1, &left->Z);
  // Z2^3
  FqMul(&fq2, &fq2, &right->Z);
  // (Z1^3)*Y2
  FqMul(&fq1, &fq1, &right->Y);
  // (Z2^3)*Y1
  FqMul(&fq2, &fq2, &left->Y);
  // (Z1^2)*X2 == (Z2^2)*X1  &&  (Z1^3)*Y2 == (Z2^3)*Y1
  return FqEq(&fq1, &fq2) && FqEq(&fq3, &fq4);
}

int EFqHash(EccPointFq* result, unsigned char const* msg, size_t len,
            HashAlg hashalg) {
  tiny_sha hash_context;
  FqElem three;
  FqElem tmp;
  uint32_t hash_salt = 0;
  uint32_t buf = 0;
  sha_digest hash_buf;
  // 1/q in Fq
  FqElem montgomery_r = {
      0x512ccfed, 0x2cd6d224, 0xed67f57d, 0xf3239a04,
      0x118e5b60, 0xb91a0da1, 0x00030f32, 0,
  };
  if ((kSha512 != hashalg) && (kSha256 != hashalg)) {
    return 0;
  }
  FqSet(&three, 3);

  for (hash_salt = 0; hash_salt < 0xFFFFFFFF; ++hash_salt) {
    tinysha_init(hashalg, &hash_context);

    Uint32Serialize((OctStr32*)&buf, hash_salt);
    tinysha_update(&hash_context, &buf, sizeof(buf));
    tinysha_update(&hash_context, msg, len);

    tinysha_final(hash_buf.digest, &hash_context);

    FqFromHash(&result->x, hash_buf.digest, tinysha_digest_size(&hash_context));
    FqSquare(&tmp, &result->x);
    FqMul(&tmp, &tmp, &result->x);
    FqAdd(&tmp, &tmp, &three);
    if (FqSqrt(&result->y, &tmp)) {
      FqNeg(&tmp, &result->y);
      // Verify and Non-tiny member use montgomery representation to determine
      // if negation is needed: this is to be compatible with them
      FqMul(&montgomery_r, &result->y, &montgomery_r);
      FqCondSet(&result->y, &tmp, &result->y, montgomery_r.limbs.word[0] & 1);
      return 1;
    }
  }
  return 0;
}

void EFqCp(EccPointFq* result, EccPointFq const* in) {
  FqCp(&result->x, &in->x);
  FqCp(&result->y, &in->y);
}

int EFqEqAffine(EccPointFq const* left, EccPointFq const* right) {
  return FqEq(&left->x, &right->x) && FqEq(&left->y, &right->y);
}

void EFqCondSet(EccPointJacobiFq* result, EccPointJacobiFq const* true_val,
                EccPointJacobiFq const* false_val, int truth_val) {
  FqCondSet(&result->X, &true_val->X, &false_val->X, truth_val);
  FqCondSet(&result->Y, &true_val->Y, &false_val->Y, truth_val);
  FqCondSet(&result->Z, &true_val->Z, &false_val->Z, truth_val);
}

void EFqJCp(EccPointJacobiFq* result, EccPointJacobiFq const* in) {
  FqCp(&result->X, &in->X);
  FqCp(&result->Y, &in->Y);
  FqCp(&result->Z, &in->Z);
}

void EFqInf(EccPointJacobiFq* result) {
  FqSet(&result->X, 0);
  FqSet(&result->Y, 1);
  FqSet(&result->Z, 0);
}

int EFqOnCurve(EccPointFq const* in) {
  // test that Y^2 mod q == (X^3 + a*Z^4*X + b*Z^6) mod q
  // This simplifies to: Y^2 mod q == (X^3 + 3) mod q
  //      since: Z = 1
  //             a = 0
  //             b = 3
  FqElem t1;
  FqElem t2;
  FqSquare(&t1, &in->x);
  FqMul(&t2, &in->x, &t1);
  FqSquare(&t1, &in->y);
  FqSub(&t1, &t1, &t2);
  t1.limbs.word[0] -= 3;  // check equal to curve b
                          // this operation will not always
                          // result in the same value as T1 - 3
                          // however it will always result in a correct
                          // value for the zero check below
  return FqIsZero(&t1);
}

int EFqJOnCurve(EccPointJacobiFq const* in) {
  FqElem fq1;
  FqElem fq2;

  FqSquare(&fq1, &in->Z);
  FqSquare(&fq2, &fq1);
  FqMul(&fq2, &fq2, &fq1);  // T2 = Z^6
  FqAdd(&fq1, &fq2, &fq2);
  FqAdd(&fq1, &fq1, &fq2);  // T1 = 3 * Z^6
  FqSquare(&fq2, &in->X);
  FqMul(&fq2, &fq2, &in->X);  // T2 = X^3
  FqAdd(&fq1, &fq1, &fq2);    // T1 = X^3 + 3 Z^6
  FqSquare(&fq2, &in->Y);
  FqMul(&fq1, &fq1, &in->Z);
  FqMul(&fq2, &fq2, &in->Z);
  return FqEq(&fq1, &fq2);  // check Y^2 = X^3 + 3 Z^6
}

int EFqJRand(EccPointJacobiFq* result, BitSupplier rnd_func, void* rnd_param) {
  FqElem fq;
  do {
    if (!FqRand(&fq, rnd_func, rnd_param)) {
      return 0;
    }
  } while (!EFqMakePoint((EccPointFq*)result, &fq));

  FqSet(&result->Z, 1);

  return 1;
}
