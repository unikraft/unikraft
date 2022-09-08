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
/// Implementation of Fq6 math
/*! \file */

#include "epid/member/tiny/math/fq6.h"

#include "epid/member/tiny/math/fq2.h"
#include "epid/member/tiny/math/mathtypes.h"

void Fq6Add(Fq6Elem* result, Fq6Elem const* left, Fq6Elem const* right) {
  Fq2Add(&result->y0, &left->y0, &right->y0);
  Fq2Add(&result->y1, &left->y1, &right->y1);
  Fq2Add(&result->y2, &left->y2, &right->y2);
}

void Fq6Sub(Fq6Elem* result, Fq6Elem const* left, Fq6Elem const* right) {
  Fq2Sub(&result->y0, &left->y0, &right->y0);
  Fq2Sub(&result->y1, &left->y1, &right->y1);
  Fq2Sub(&result->y2, &left->y2, &right->y2);
}

void Fq6Mul(Fq6Elem* result, Fq6Elem const* left, Fq6Elem const* right) {
  Fq2Elem tmpa;
  Fq2Elem tmpb;
  Fq2Elem tmpc;
  Fq2Elem tmpd;
  Fq2Elem tmpe;
  Fq2Elem* temp_a = &tmpa;
  Fq2Elem* temp_b = &tmpb;
  Fq2Elem* temp_c = &tmpc;
  Fq2Elem* temp_d = &tmpd;
  Fq2Elem* temp_e = &tmpe;
  Fq2Mul(temp_a, &left->y0, &right->y0);   // temp_a = t0 = a[0] * b[0]
  Fq2Mul(temp_b, &left->y1, &right->y1);   // temp_b = t1
  Fq2Mul(temp_c, &left->y2, &right->y2);   // temp_c = t2
  Fq2Add(temp_d, &left->y1, &left->y2);    // temp_d = t3
  Fq2Add(temp_e, &right->y1, &right->y2);  // temp_e = t4
  Fq2Mul(temp_d, temp_d, temp_e);          // temp_d = t3
  Fq2Sub(temp_d, temp_d, temp_b);
  Fq2Sub(temp_d, temp_d, temp_c);
  Fq2MulXi(temp_e, temp_d);
  Fq2Add(temp_d, &left->y0, &left->y1);
  Fq2Add(&result->y1, &right->y0, &right->y1);  // &result->y1 = t4
  Fq2Mul(temp_d, temp_d, &result->y1);
  Fq2MulXi(&result->y1, temp_c);  // result->y1 = Fq2.mulXi(t2)
  Fq2Add(&result->y1, &result->y1, temp_d);
  Fq2Sub(&result->y1, &result->y1, temp_a);
  Fq2Sub(&result->y1, &result->y1, temp_b);
  Fq2Add(temp_d, &left->y0, &left->y2);
  Fq2Sub(temp_b, temp_b, temp_c);
  Fq2Add(temp_c, &right->y0, &right->y2);
  Fq2Add(&result->y0, temp_e, temp_a);  // temp_e = e[0], reordered instruction
  Fq2Mul(temp_d, temp_d, temp_c);
  Fq2Sub(temp_d, temp_d, temp_a);
  Fq2Add(&result->y2, temp_d, temp_b);
}

void Fq6Inv(Fq6Elem* result, Fq6Elem const* in) {
  Fq2Elem tmpa;
  Fq2Elem tmpb;
  Fq2Elem tmpc;
  Fq2Elem tmpd;
  Fq2Elem* temp_a = &tmpa;
  Fq2Elem* temp_b = &tmpb;
  Fq2Elem* temp_c = &tmpc;
  Fq2Elem* temp_d = &tmpd;
  Fq2Square(temp_a, &in->y0);
  Fq2Mul(temp_d, &in->y1, &in->y2);
  Fq2MulXi(temp_d, temp_d);
  Fq2Sub(temp_a, temp_a, temp_d);
  Fq2Square(temp_b, &in->y2);
  Fq2Mul(temp_d, &in->y0, &in->y1);
  Fq2MulXi(temp_b, temp_b);
  Fq2Sub(temp_b, temp_b, temp_d);
  Fq2Square(temp_c, &in->y1);
  Fq2Mul(temp_d, &in->y0, &in->y2);
  Fq2Sub(temp_c, temp_c, temp_d);
  // using the results as temporary variables
  Fq2Mul(&result->y0, &in->y0, temp_a);
  Fq2Mul(&result->y1, &in->y1, temp_c);
  Fq2Mul(&result->y2, &in->y2, temp_b);
  Fq2MulXi(&result->y1, &result->y1);
  Fq2MulXi(&result->y2, &result->y2);
  Fq2Add(temp_d, &result->y0, &result->y1);
  Fq2Add(temp_d, temp_d, &result->y2);
  Fq2Inv(temp_d, temp_d);
  Fq2Mul(&result->y0, temp_a, temp_d);
  Fq2Mul(&result->y1, temp_b, temp_d);
  Fq2Mul(&result->y2, temp_c, temp_d);
}

void Fq6Neg(Fq6Elem* result, Fq6Elem const* in) {
  Fq2Neg(&result->y0, &in->y0);
  Fq2Neg(&result->y1, &in->y1);
  Fq2Neg(&result->y2, &in->y2);
}

void Fq6Clear(Fq6Elem* result) {
  Fq2Clear(&result->y0);
  Fq2Clear(&result->y1);
  Fq2Clear(&result->y2);
}

void Fq6MulScalar(Fq6Elem* result, Fq6Elem const* in, Fq2Elem const* scalar) {
  Fq2Mul(&result->y0, &in->y0, scalar);
  Fq2Mul(&result->y1, &in->y1, scalar);
  Fq2Mul(&result->y2, &in->y2, scalar);
}

void Fq6MulV(Fq6Elem* result, Fq6Elem const* in) {
  Fq2Elem tmp;
  Fq2Elem* temp = &tmp;
  Fq2MulXi(temp, &in->y2);
  Fq2Cp(&result->y2, &in->y1);
  Fq2Cp(&result->y1, &in->y0);
  Fq2Cp(&result->y0, temp);
}

int Fq6Eq(Fq6Elem const* left, Fq6Elem const* right) {
  return Fq2Eq(&left->y0, &right->y0) && Fq2Eq(&left->y1, &right->y1) &&
         Fq2Eq(&left->y2, &right->y2);
}

int Fq6IsZero(Fq6Elem const* in) {
  return Fq2IsZero(&in->y0) && Fq2IsZero(&in->y1) && Fq2IsZero(&in->y2);
}

void Fq6Square(Fq6Elem* result, Fq6Elem const* in) {
  Fq2Elem T0;
  Fq2Elem T2;
  Fq2Elem T3;
  Fq2Elem* t0 = &T0;
  Fq2Elem* t1 = &result->y1;
  Fq2Elem* t2 = &T2;
  Fq2Elem* t3 = &T3;
  Fq2Add(t0, &in->y1, &in->y2);
  Fq2Square(t3, &in->y1);
  Fq2Add(t1, &in->y0, &in->y1);
  Fq2Add(t2, &in->y0, &in->y2);
  Fq2Square(t0, t0);
  Fq2Square(t1, t1);
  Fq2Square(t2, t2);

  // using result from Fq2Square(t3, in->y1):
  Fq2Sub(t0, t0, t3);
  Fq2Sub(t1, t1, t3);
  Fq2Add(t2, t2, t3);

  Fq2Square(t3, &in->y2);
  Fq2Sub(t0, t0, t3);
  Fq2Sub(t2, t2, t3);
  Fq2MulXi(t3, t3);
  Fq2Add(t1, t1, t3);

  Fq2Square(t3, &in->y0);
  Fq2MulXi(t0, t0);
  Fq2Add(&result->y0, t0, t3);
  Fq2Sub(&result->y1, t1, t3);
  Fq2Sub(&result->y2, t2, t3);
}

void Fq6Cp(Fq6Elem* result, Fq6Elem const* in) {
  Fq2Cp(&result->y0, &in->y0);
  Fq2Cp(&result->y1, &in->y1);
  Fq2Cp(&result->y2, &in->y2);
}

void Fq6CondSet(Fq6Elem* result, Fq6Elem const* true_val,
                Fq6Elem const* false_val, int truth_val) {
  Fq2CondSet(&result->y0, &true_val->y0, &false_val->y0, truth_val);
  Fq2CondSet(&result->y1, &true_val->y1, &false_val->y1, truth_val);
  Fq2CondSet(&result->y2, &true_val->y2, &false_val->y2, truth_val);
}

void Fq6Set(Fq6Elem* result, uint32_t in) {
  Fq6Clear(result);
  Fq2Set(&result->y0, in);
}
