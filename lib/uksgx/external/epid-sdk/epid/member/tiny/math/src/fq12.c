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
/// Implementation of Fq12 math
/*! \file */

#include "epid/member/tiny/math/fq12.h"

#include "epid/member/tiny/math/fq.h"
#include "epid/member/tiny/math/fq2.h"
#include "epid/member/tiny/math/fq6.h"
#include "epid/member/tiny/math/mathtypes.h"
#include "epid/member/tiny/math/vli.h"
#include "epid/member/tiny/stdlib/tiny_stdlib.h"

static void Fq12MulScalar(Fq12Elem* result, Fq12Elem const* left,
                          Fq6Elem const* right) {
  Fq6Mul(&result->z0, &left->z0, right);
  Fq6Mul(&result->z1, &left->z1, right);
}

static void Fq4Square(Fq2Elem* out0, Fq2Elem* out1, Fq2Elem const* in0,
                      Fq2Elem const* in1) {
  Fq2Elem tmp;
  Fq2Elem* temp = &tmp;
  Fq2Square(temp, in1);
  Fq2Add(out1, in0, in1);
  Fq2Square(out0, in0);
  Fq2Square(out1, out1);
  Fq2Sub(out1, out1, temp);
  Fq2Sub(out1, out1, out0);
  Fq2MulXi(temp, temp);
  Fq2Add(out0, out0, temp);
}

static void Fq12CondSet(Fq12Elem* result, Fq12Elem const* true_val,
                        Fq12Elem const* false_val, int truth_val) {
  Fq6CondSet(&result->z0, &true_val->z0, &false_val->z0, truth_val);
  Fq6CondSet(&result->z1, &true_val->z1, &false_val->z1, truth_val);
}

void Fq12Add(Fq12Elem* result, Fq12Elem const* left, Fq12Elem const* right) {
  Fq6Add(&result->z0, &left->z0, &right->z0);
  Fq6Add(&result->z1, &left->z1, &right->z1);
}

void Fq12Sub(Fq12Elem* result, Fq12Elem const* left, Fq12Elem const* right) {
  Fq6Sub(&result->z0, &left->z0, &right->z0);
  Fq6Sub(&result->z1, &left->z1, &right->z1);
}

void Fq12Square(Fq12Elem* result, Fq12Elem const* in) {
  Fq6Elem tmpa;
  Fq6Elem* temp_a = &tmpa;
  Fq6Square(temp_a, &in->z1);
  Fq6Add(&result->z1, &in->z0, &in->z1);
  Fq6Square(&result->z0, &in->z0);
  Fq6Square(&result->z1, &result->z1);
  Fq6Sub(&result->z1, &result->z1, (&result->z0));
  Fq6Sub(&result->z1, &result->z1, temp_a);
  Fq6MulV(temp_a, temp_a);
  Fq6Add((&result->z0), (&result->z0), temp_a);
}

void Fq12Mul(Fq12Elem* result, Fq12Elem const* left, Fq12Elem const* right) {
  Fq6Elem A;
  Fq6Elem B;
  Fq6Elem* t0 = &A;
  Fq6Elem* t1 = &B;

  Fq6Add(t0, &left->z0, &left->z1);
  Fq6Add(t1, &right->z0, &right->z1);
  Fq6Mul(t0, t0, t1);
  Fq6Mul(&result->z0, &left->z0, &right->z0);
  Fq6Sub(t0, t0, &result->z0);
  Fq6Mul(t1, &left->z1, &right->z1);
  Fq6Sub(&result->z1, t0, t1);
  Fq6MulV(t1, t1);
  Fq6Add(&result->z0, &result->z0, t1);
}

void Fq12Inv(Fq12Elem* result, Fq12Elem const* in) {
  Fq12Elem tmp3;
  Fq12Elem tmp4;
  Fq12Elem* const temp3 = &tmp3;
  Fq12Elem* const temp4 = &tmp4;
  Fq12Conj(temp3, in);
  Fq12Mul(temp4, temp3, in);
  Fq6Inv(&temp4->z0, &temp4->z0);
  Fq12MulScalar(result, temp3, &temp4->z0);
}

void Fq12Neg(Fq12Elem* result, Fq12Elem const* in) {
  Fq6Neg(&result->z0, &in->z0);
  Fq6Neg(&result->z1, &in->z1);
}

void Fq12Set(Fq12Elem* result, uint32_t val) {
  Fq12Clear(result);
  FqSet(&(*result).z0.y0.x0, val);
}

void Fq12Exp(Fq12Elem* result, Fq12Elem const* base, VeryLargeInt const* exp) {
  int i;
  Fq12Elem tmp, tmp2, *const temp = &tmp, *const temp2 = &tmp2;
  Fq12Clear(temp);
  temp->z0.y0.x0.limbs.word[0]++;
  for (i = NUM_ECC_DIGITS * 32 - 1; i >= 0; i--) {
    Fq12Square(temp, temp);
    Fq12Mul(temp2, temp, base);

    Fq12CondSet(temp, temp2, temp,
                (int)((exp->word[i / 32] >> (i & 31)) & (0x1)));
  }
  Fq12Cp(result, temp);
}

void Fq12MultiExp(Fq12Elem* result, Fq12Elem const* base0,
                  VeryLargeInt const* exp0, Fq12Elem const* base1,
                  VeryLargeInt const* exp1, Fq12Elem const* base2,
                  VeryLargeInt const* exp2, Fq12Elem const* base3,
                  VeryLargeInt const* exp3) {
  Fq12Elem tmp;
  Fq12Exp(result, base0, exp0);
  Fq12Exp(&tmp, base1, exp1);
  Fq12Mul(result, result, &tmp);
  Fq12Exp(&tmp, base2, exp2);
  Fq12Mul(result, result, &tmp);
  Fq12Exp(&tmp, base3, exp3);
  Fq12Mul(result, result, &tmp);
}

int Fq12Eq(Fq12Elem const* left, Fq12Elem const* right) {
  return Fq6Eq(&left->z0, &right->z0) && Fq6Eq(&left->z0, &right->z0);
}

void Fq12Conj(Fq12Elem* result, Fq12Elem const* in) {
  Fq6Cp(&result->z0, &in->z0);
  Fq6Neg(&result->z1, &in->z1);
}

void Fq12ExpCyc(Fq12Elem* result, Fq12Elem const* in, VeryLargeInt const* t) {
  int i = 0;
  Fq12Elem ac;
  Fq12Elem* const acc = &ac;
  Fq12Cp(acc, in);
  Fq12Cp(result, in);

  for (i = 61; i >= 0; i--) {
    Fq12SqCyc(result, result);

    if (VliTestBit(t, (uint32_t)i)) {
      Fq12Mul(result, result, acc);
    }
  }
}

void Fq12SqCyc(Fq12Elem* result, Fq12Elem const* in) {
  Fq2Elem const* a0 = &(in->z0).y0;
  Fq2Elem const* a1 = &(in->z1).y0;
  Fq2Elem const* a2 = &(in->z0).y1;
  Fq2Elem const* a3 = &(in->z1).y1;
  Fq2Elem const* a4 = &(in->z0).y2;
  Fq2Elem const* a5 = &(in->z1).y2;
  Fq2Elem* e0 = &(result->z0).y0;
  Fq2Elem* e1 = &(result->z1).y0;
  Fq2Elem* e2 = &(result->z0).y1;
  Fq2Elem* e3 = &(result->z1).y1;
  Fq2Elem* e4 = &(result->z0).y2;
  Fq2Elem* e5 = &(result->z1).y2;
  Fq2Elem tmp1;
  Fq2Elem tmp2;
  Fq2Elem tmp3;
  Fq2Elem tmp4;
  Fq2Elem* temp1 = &tmp1;
  Fq2Elem* temp2 = &tmp2;
  Fq2Elem* temp3 = &tmp3;
  Fq2Elem* temp4 = &tmp4;

  Fq4Square(temp1, temp2, a0, a3);  // t00,t11 = sq(a0,a3)
  Fq2Add(e0, a0, a0);               // e0 = 3*t00 - 2*a0
  Fq2Sub(e0, temp1, e0);
  Fq2Add(e0, temp1, e0);
  Fq2Add(e0, temp1, e0);
  Fq2Add(e3, a3, a3);  // e3 = 3*t11 - 2*a3
  Fq2Add(e3, temp2, e3);
  Fq2Add(e3, temp2, e3);
  Fq2Add(e3, temp2, e3);

  Fq4Square(temp1, temp2, a2, a5);  // t02, t10 = sq(a2,a5)
  Fq2MulXi(temp2, temp2);
  Fq4Square(temp3, temp4, a1, a4);  // t01, t12 = sq(a1,a4)

  Fq2Add(e4, a4, a4);
  Fq2Sub(e4, temp1, e4);
  Fq2Add(e4, temp1, e4);
  Fq2Add(e4, temp1, e4);
  Fq2Add(e1, a1, a1);
  Fq2Add(e1, temp2, e1);
  Fq2Add(e1, temp2, e1);
  Fq2Add(e1, temp2, e1);

  Fq2Add(e2, a2, a2);
  Fq2Sub(e2, temp3, e2);
  Fq2Add(e2, temp3, e2);
  Fq2Add(e2, temp3, e2);
  Fq2Add(e5, a5, a5);
  Fq2Add(e5, temp4, e5);
  Fq2Add(e5, temp4, e5);
  Fq2Add(e5, temp4, e5);
}

void Fq12MulSpecial(Fq12Elem* result, Fq12Elem const* left,
                    Fq12Elem const* right) {
  Fq2Elem T3;
  Fq2Elem* t3 = &T3;
  Fq2Elem const* b0 = &(right->z0.y0);
  Fq2Elem const* b1 = &right->z1.y0;
  Fq2Elem const* b3 = &right->z1.y1;
  Fq6Elem T0;
  Fq6Elem T1;
  Fq6Elem T2;
  Fq6Elem* t0 = &T0;
  Fq6Elem* t1 = &T1;
  Fq6Elem* t2 = &T2;
  Fq6Elem const* a0 = &left->z0;
  Fq6Elem const* a1 = &left->z1;
  Fq6Elem* r0 = &result->z0;
  Fq6Elem* r1 = &result->z1;

#if defined(DEBUG)
  // validate algorithm precondition
  if (!Fq2IsZero(&right->z0.y1) || !Fq2IsZero(&right->z0.y2) ||
      !Fq2IsZero(&right->z1.y2)) {
    memset(&result, 0xff, sizeof(result));
    return;
  }
#endif  // defined(DEBUG)

  Fq6Add(t0, a0, a1);
  Fq2Add(t3, b0, b1);
  Fq6MulScalar(t0, t0, t3);

  Fq6MulScalar(t2, a1, b3);
  Fq6MulV(t2, t2);

  Fq6MulScalar(t1, a1, b1);
  Fq6Sub(t0, t0, t1);
  Fq6Add(t2, t2, t1);
  Fq6MulV(t2, t2);

  Fq6MulScalar(t1, a0, b0);
  Fq6Add(t2, t2, t1);
  Fq6Sub(t0, t0, t1);

  Fq6MulScalar(t1, a0, b3);
  Fq6MulV(t1, t1);
  Fq6Add(r1, t1, t0);
  Fq6Cp(r0, t2);
}

void Fq12Cp(Fq12Elem* result, Fq12Elem const* in) {
  Fq6Cp(&result->z0, &in->z0);
  Fq6Cp(&result->z1, &in->z1);
}

void Fq12Clear(Fq12Elem* result) {
  Fq6Clear(&result->z0);
  Fq6Clear(&result->z1);
}
