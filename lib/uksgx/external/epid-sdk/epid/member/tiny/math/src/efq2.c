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
/// Implementation of EFq2 math
/*! \file */

#include "epid/member/tiny/math/efq2.h"

#include "epid/member/tiny/math/fq.h"
#include "epid/member/tiny/math/fq2.h"
#include "epid/member/tiny/math/mathtypes.h"
#include "epid/member/tiny/math/vli.h"

static void EFq2CondSet(EccPointJacobiFq2* result,
                        EccPointJacobiFq2 const* true_val,
                        EccPointJacobiFq2 const* false_val, int truth_val) {
  Fq2CondSet(&result->X, &true_val->X, &false_val->X, truth_val);
  Fq2CondSet(&result->Y, &true_val->Y, &false_val->Y, truth_val);
  Fq2CondSet(&result->Z, &true_val->Z, &false_val->Z, truth_val);
}

static void EFq2Cp(EccPointJacobiFq2* result, EccPointJacobiFq2 const* in) {
  Fq2Cp(&result->X, &in->X);
  Fq2Cp(&result->Y, &in->Y);
  Fq2Cp(&result->Z, &in->Z);
}

static void EFq2Inf(EccPointJacobiFq2* result) {
  Fq2Set(&result->X, 0);
  Fq2Set(&result->Y, 1);
  Fq2Set(&result->Z, 0);
}

int EFq2IsInf(EccPointJacobiFq2 const* in) {
  return Fq2IsZero(&in->X) && Fq2IsZero(&in->Z) && (!Fq2IsZero(&in->Y));
}

void EFq2FromAffine(EccPointJacobiFq2* result, EccPointFq2 const* in) {
  Fq2Cp(&result->X, &in->x);
  Fq2Cp(&result->Y, &in->y);
  Fq2Set(&result->Z, 1);
}

int EFq2ToAffine(EccPointFq2* result, EccPointJacobiFq2 const* in) {
  Fq2Elem inverted_z;
  if (EFq2IsInf(in)) {
    return 0;
  }
  Fq2Inv(&inverted_z, &in->Z);
  Fq2Mul(&result->x, &in->X, &inverted_z);
  Fq2Mul(&result->x, &result->x, &inverted_z);
  Fq2Mul(&result->y, &in->Y, &inverted_z);
  Fq2Mul(&result->y, &result->y, &inverted_z);
  Fq2Mul(&result->y, &result->y, &inverted_z);
  return 1;
}

void EFq2Dbl(EccPointJacobiFq2* result, EccPointJacobiFq2 const* in) {
  Fq2Elem a;
  Fq2Elem b;
  // Z3 = 2Z1
  Fq2Add(&(result->Z), &(in->Z), &(in->Z));
  // Z3 = 2*Z1*Y1
  Fq2Mul(&(result->Z), &(result->Z), &(in->Y));
  // A = X1^2
  Fq2Square(&a, &(in->X));
  // B = 2(X1^2)
  Fq2Add(&b, &a, &a);
  // B = 3(X1^2)
  Fq2Add(&b, &b, &a);
  // A = Y1^2
  Fq2Square(&a, &(in->Y));
  // A = 2*(Y1^2)
  Fq2Add(&a, &a, &a);
  // Y3 = 4*(Y1^4)
  Fq2Square(&(result->Y), &a);
  // Y3 = 8*(Y1^4)
  Fq2Add(&(result->Y), &(result->Y), &(result->Y));
  // A = 4(Y1^2)
  Fq2Add(&a, &a, &a);
  // A = 4(Y1^2)*X1
  Fq2Mul(&a, &a, &(in->X));
  // X3 = B^2
  Fq2Square(&(result->X), &b);
  // X3 = (B^2) - A
  Fq2Sub(&(result->X), &(result->X), &a);
  // X3 = (B^2) - 2A
  Fq2Sub(&(result->X), &(result->X), &a);
  // A = A - X3
  Fq2Sub(&a, &a, &(result->X));
  // A = B*(A-X3)
  Fq2Mul(&a, &a, &b);
  // Y3 = B*(A-X3) - 8*(Y1^4)
  Fq2Sub(&(result->Y), &a, &(result->Y));
}

void EFq2Add(EccPointJacobiFq2* result, EccPointJacobiFq2 const* left,
             EccPointJacobiFq2 const* right) {
  Fq2Elem A;
  Fq2Elem B;
  Fq2Elem C;
  Fq2Elem D;
  Fq2Elem W;
  Fq2Elem V;

  if (Fq2IsZero(&left->Z)) {
    // If P = O, set R = Q and return
    EFq2Cp(result, right);
    return;
  }
  if (Fq2IsZero(&right->Z)) {
    // If Q = O, set R = P and return.
    EFq2Cp(result, left);
    return;
  }
  // A = P.X * Q.Z^2
  Fq2Square(&C, &right->Z);
  Fq2Mul(&A, &left->X, &C);
  // B = Q.X * P.Z^2
  Fq2Square(&D, &left->Z);
  Fq2Mul(&B, &right->X, &D);
  // C = P.Y * Q.Z^3
  Fq2Mul(&C, &right->Z, &C);
  Fq2Mul(&C, &left->Y, &C);
  // D = Q.Y * P.Z^3
  Fq2Mul(&D, &left->Z, &D);
  Fq2Mul(&D, &right->Y, &D);
  // W = B - A
  Fq2Sub(&W, &B, &A);
  // V = D - C
  Fq2Sub(&V, &D, &C);
  if (Fq2IsZero(&W)) {
    if (Fq2IsZero(&V)) {
      EFq2Dbl(result, left);
      return;
    } else {
      EFq2Inf(result);
      return;
    }
  }
  // R.Z = P.Z * Q.Z * W
  Fq2Mul(&result->Z, &left->Z, &right->Z);
  Fq2Mul(&result->Z, &result->Z, &W);
  // R.X = V^2 - (A + B) * W^2
  Fq2Square(&result->X, &V);
  Fq2Add(&B, &A, &B);
  // Before squaring W save (C * W) to use in compitation of R.Y
  Fq2Mul(&C, &C, &W);
  Fq2Square(&W, &W);
  Fq2Mul(&B, &B, &W);
  Fq2Sub(&result->X, &result->X, &B);
  // R.Y = V * (A * W^2 - R.X) - C * W^3
  Fq2Mul(&A, &A, &W);
  Fq2Sub(&A, &A, &result->X);
  Fq2Mul(&result->Y, &V, &A);
  Fq2Mul(&C, &C, &W);
  Fq2Sub(&result->Y, &result->Y, &C);
}

void EFq2Neg(EccPointJacobiFq2* result, EccPointJacobiFq2 const* in) {
  Fq2Cp(&result->X, &in->X);
  Fq2Neg(&result->Y, &in->Y);
  Fq2Cp(&result->Z, &in->Z);
}

void EFq2MulSSCM(EccPointJacobiFq2* result, EccPointJacobiFq2 const* left,
                 FpElem const* right) {
  int position;
  EccPointJacobiFq2 nv;
  EccPointJacobiFq2 mv;
  EFq2Inf(&nv);
  EFq2Cp(&mv, left);
  for (position = 32 * NUM_ECC_DIGITS - 1; position >= 0; position--) {
    EFq2Dbl(&nv, &nv);
    EFq2Add(&mv, left, &nv);
    EFq2CondSet(&nv, &mv, &nv,
                (int)(VliTestBit(&right->limbs, (uint32_t)position)));
  }
  EFq2Cp(result, &nv);
}

int EFq2Eq(EccPointJacobiFq2 const* left, EccPointJacobiFq2 const* right) {
  Fq2Elem t1;
  Fq2Elem t2;
  Fq2Elem t3;
  Fq2Elem t4;

  if (EFq2IsInf(left) && EFq2IsInf(right)) {
    return 1;
  }
  // if either left or right equals to inf return 0
  if (EFq2IsInf(left) || EFq2IsInf(right)) {
    return 0;
  }
  Fq2Square(&t1, &(left->Z));
  Fq2Square(&t2, &(right->Z));
  Fq2Mul(&t3, &t1, &(right->X));
  Fq2Mul(&t4, &t2, &(left->X));
  Fq2Mul(&t1, &t1, &(left->Z));
  Fq2Mul(&t2, &t2, &(right->Z));
  Fq2Mul(&t1, &t1, &(right->Y));
  Fq2Mul(&t2, &t2, &(left->Y));
  return Fq2Eq(&t1, &t2) && Fq2Eq(&t3, &t4);
}

int EFq2OnCurve(EccPointFq2 const* in) {
  // test that Y^2 mod q == (X^3 + a*Z^4*X + b'*Z^6) mod q
  // This simplifies to: Y^2 mod q == (X^3 + b') mod q
  //      since: Z = 1
  //             a = 0
  //             b = 3
  Fq2Elem t1;
  Fq2Elem t2;
  FqElem three;
  // Fq2xi
  Fq2Elem bp = {{{{0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                   0x00000000, 0x00000000, 0x00000000}}},
                {{{0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                   0x00000000, 0x00000000, 0x00000000}}}};
  Fq2Elem const* x = &in->x;
  Fq2Elem const* y = &in->y;

  // b' = b * inv(x1)
  FqSet(&three, 3);
  Fq2Inv(&bp, &bp);
  Fq2MulScalar(&bp, &bp, &three);

  // set t2 = X^3
  Fq2Square(&t1, x);
  Fq2Mul(&t2, x, &t1);
  // set t2 = X^3 + b'
  Fq2Add(&t2, &t2, &bp);

  // set t1 = Y^2
  Fq2Square(&t1, y);

  // set t1 = Y^2 - (X^3 + b')
  Fq2Sub(&t1, &t1, &t2);
  // return if t1 is zero
  return Fq2IsZero(&t1);
}
