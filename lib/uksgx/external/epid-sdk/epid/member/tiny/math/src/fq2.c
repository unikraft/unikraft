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
/// Implementation of Fq2 math
/*! \file */

#include "epid/member/tiny/math/fq2.h"

#include "epid/member/tiny/math/fq.h"
#include "epid/member/tiny/math/mathtypes.h"

void Fq2Cp(Fq2Elem* result, Fq2Elem const* in) {
  FqCp(&(result->x0), &(in->x0));
  FqCp(&(result->x1), &(in->x1));
}

void Fq2Set(Fq2Elem* result, uint32_t in) {
  FqSet(&(result->x0), in);
  FqClear(&(result->x1));
}

void Fq2Clear(Fq2Elem* result) {
  FqClear(&result->x0);
  FqClear(&result->x1);
}

void Fq2Add(Fq2Elem* result, Fq2Elem const* left, Fq2Elem const* right) {
  FqAdd(&(result->x0), &(left->x0), &(right->x0));
  FqAdd(&(result->x1), &(left->x1), &(right->x1));
}

void Fq2Exp(Fq2Elem* result, Fq2Elem const* base, VeryLargeInt const* exp) {
  int i, j;
  Fq2Elem tmp;
  Fq2Elem tmp2;
  Fq2Elem* temp = &tmp;
  Fq2Elem* temp2 = &tmp2;
  FqSet(&(temp->x0), 1);
  FqClear(&(temp->x1));
  for (i = NUM_ECC_DIGITS - 1; i >= 0; i--) {
    for (j = 31; j >= 0; j--) {
      Fq2Square(temp, temp);
      Fq2Mul(temp2, temp, base);

      Fq2CondSet(temp, temp2, temp, (int)((exp->word[i] >> j) & (0x1)));
    }
  }
  Fq2Cp(result, temp);
}

void Fq2Sub(Fq2Elem* result, Fq2Elem const* left, Fq2Elem const* right) {
  FqSub(&(result->x0), &(left->x0), &(right->x0));
  FqSub(&(result->x1), &(left->x1), &(right->x1));
}

void Fq2Mul(Fq2Elem* result, Fq2Elem const* left, Fq2Elem const* right) {
  FqElem A;
  FqElem B;
  FqElem* a = &A;
  FqElem* b = &B;

  FqAdd(a, &left->x0, &left->x1);
  FqAdd(b, &right->x0, &right->x1);
  FqMul(a, a, b);
  FqMul(&result->x0, &left->x0, &right->x0);
  FqSub(a, a, &result->x0);
  FqMul(b, &left->x1, &right->x1);
  FqSub(&result->x1, a, b);
  FqNeg(b, b);  // b = b*beta
  FqAdd(&result->x0, &result->x0, b);
}

void Fq2Inv(Fq2Elem* result, Fq2Elem const* in) {
  FqElem tmp;
  FqElem tmp2;
  FqElem* temp = &tmp;
  FqElem* temp2 = &tmp2;
  FqSquare(temp, &in->x1);
  FqSquare(temp2, &in->x0);
  FqAdd(temp, temp, temp2);
  FqInv(temp, temp);
  FqMul(&result->x0, temp, &in->x0);
  FqNeg(temp, temp);
  FqMul(&result->x1, temp, &in->x1);
}

void Fq2Neg(Fq2Elem* result, Fq2Elem const* in) {
  FqNeg(&(result->x0), &(in->x0));
  FqNeg(&(result->x1), &(in->x1));
}

void Fq2Conj(Fq2Elem* result, Fq2Elem const* in) {
  FqCp(&result->x0, &in->x0);
  FqNeg(&result->x1, &in->x1);
}

void Fq2Square(Fq2Elem* result, Fq2Elem const* in) {
  FqElem tmpa;
  FqElem* temp_a = &tmpa;
  FqElem tmpb;
  FqElem* temp_b = &tmpb;
  FqAdd(temp_a, &in->x0, &in->x1);
  FqMul(temp_b, &in->x0, &in->x1);
  FqSub(&result->x0, &in->x0, &in->x1);
  FqMul(&result->x0, temp_a, &result->x0);
  FqAdd(&result->x1, temp_b, temp_b);
}

void Fq2MulScalar(Fq2Elem* result, Fq2Elem const* left, FqElem const* right) {
  FqMul(&(result->x0), &(left->x0), right);
  FqMul(&(result->x1), &(left->x1), right);
}

void Fq2CondSet(Fq2Elem* result, Fq2Elem const* true_val,
                Fq2Elem const* false_val, int truth_val) {
  FqCondSet(&(result->x0), &(true_val->x0), &(false_val->x0), truth_val);
  FqCondSet(&(result->x1), &(true_val->x1), &(false_val->x1), truth_val);
}

int Fq2Eq(Fq2Elem const* left, Fq2Elem const* right) {
  return FqEq(&(left->x0), &(right->x0)) && FqEq(&(left->x1), &(right->x1));
}

void Fq2MulXi(Fq2Elem* result, Fq2Elem const* in) {
  // has the same effect as Fq2Mul(result, in, &Fq2xi) with better speed, low
  // space;
  FqElem tmp;
  FqElem* temp = &tmp;
  FqAdd(temp, &in->x0, &in->x0);
  FqSub(temp, temp, &in->x1);
  FqAdd(&result->x1, &in->x1, &in->x1);
  FqAdd(&result->x1, &result->x1, &in->x0);
  FqCp(&result->x0, temp);
}

int Fq2IsZero(Fq2Elem const* value) {
  return FqIsZero(&value->x0) && FqIsZero(&value->x1);
}
