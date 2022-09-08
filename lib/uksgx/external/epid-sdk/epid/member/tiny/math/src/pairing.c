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
/// Implementation of Tate pairing
/*! \file */

#include "epid/member/tiny/math/pairing.h"
#include "epid/member/tiny/math/efq2.h"
#include "epid/member/tiny/math/fq.h"
#include "epid/member/tiny/math/fq12.h"
#include "epid/member/tiny/math/fq2.h"
#include "epid/member/tiny/math/mathtypes.h"
#include "epid/member/tiny/math/vli.h"

static const VeryLargeInt epid_e = {{0xf2788803, 0x7886dcf9, 0x2dc401c0,
                                     0xd77a10ff, 0x27bd9b6f, 0x367ba865,
                                     0xaaaa2822, 0x2aaaaaaa}};
static const VeryLargeInt epid_t = {{0x30B0A801, 0x6882F5C0, 0, 0, 0, 0, 0, 0}};
static const Fq2Elem epid_xi = {
    {{{0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
       0x00000000, 0x00000000}}},
    {{{0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
       0x00000000, 0x00000000}}}};
static const int neg = 1;

void PairingInit(PairingState* state) {
  int i;
  Fq2Exp(&state->g[0][0], &epid_xi, &epid_e);
  for (i = 1; i < 5; i++) {
    Fq2Mul(&state->g[0][i], &state->g[0][i - 1], &state->g[0][0]);
  }
  for (i = 0; i < 5; i++) {
    FqSquare(&state->g[1][i].x0, &state->g[0][i].x0);
    FqSquare(&state->g[1][i].x1, &state->g[0][i].x1);
    FqAdd(&state->g[1][i].x0, &state->g[1][i].x0, &state->g[1][i].x1);
    FqClear(&state->g[1][i].x1);
    Fq2Mul(&state->g[2][i], &state->g[0][i], &state->g[1][i]);
  }
}

static void piOp(EccPointFq2* Qout, EccPointFq2 const* Qin, int e,
                 PairingState const* scratch) {
  if (e == 1) {
    Fq2Conj(&Qout->x, &Qin->x);
    Fq2Conj(&Qout->y, &Qin->y);
  } else {
    Fq2Cp(&Qout->x, &Qin->x);
    Fq2Cp(&Qout->y, &Qin->y);
  }
  Fq2Mul(&Qout->x, &Qout->x, &scratch->g[e - 1][1]);
  Fq2Mul(&Qout->y, &Qout->y, &scratch->g[e - 1][2]);
}

/*
 * Computes the Frobenius endomorphism Pout = Pin^(p^e)
 */
static void frob_op(Fq12Elem* Pout, Fq12Elem const* Pin, int e,
                    PairingState const* state) {
  if (e == 1 || e == 3) {
    Fq2Conj(&Pout->z0.y0, &Pin->z0.y0);
    Fq2Conj(&Pout->z1.y0, &Pin->z1.y0);
    Fq2Conj(&Pout->z0.y1, &Pin->z0.y1);
    Fq2Conj(&Pout->z1.y1, &Pin->z1.y1);
    Fq2Conj(&Pout->z0.y2, &Pin->z0.y2);
    Fq2Conj(&Pout->z1.y2, &Pin->z1.y2);
  } else {
    Fq2Cp(&Pout->z0.y0, &Pin->z0.y0);
    Fq2Cp(&Pout->z1.y0, &Pin->z1.y0);
    Fq2Cp(&Pout->z0.y1, &Pin->z0.y1);
    Fq2Cp(&Pout->z1.y1, &Pin->z1.y1);
    Fq2Cp(&Pout->z0.y2, &Pin->z0.y2);
    Fq2Cp(&Pout->z1.y2, &Pin->z1.y2);
  }
  Fq2Mul(&Pout->z1.y0, &Pout->z1.y0, &state->g[e - 1][0]);
  Fq2Mul(&Pout->z0.y1, &Pout->z0.y1, &state->g[e - 1][1]);
  Fq2Mul(&Pout->z1.y1, &Pout->z1.y1, &state->g[e - 1][2]);
  Fq2Mul(&Pout->z0.y2, &Pout->z0.y2, &state->g[e - 1][3]);
  Fq2Mul(&Pout->z1.y2, &Pout->z1.y2, &state->g[e - 1][4]);
}

static void finalExp(Fq12Elem* f, PairingState const* state) {
  Fq12Elem t3;
  Fq12Elem t2;
  Fq12Elem t1;
  Fq12Elem t0;

  Fq12Conj(&t1, f);
  Fq12Inv(&t2, f);
  Fq12Mul(f, &t1, &t2);
  frob_op(&t2, f, 2, state);
  Fq12Mul(f, f, &t2);
  Fq12ExpCyc(&t1, f, &epid_t);
  if (neg) {
    Fq12Conj(&t1, &t1);
  }
  Fq12ExpCyc(&t3, &t1, &epid_t);
  if (neg) {
    Fq12Conj(&t3, &t3);
  }
  Fq12ExpCyc(&t0, &t3, &epid_t);
  if (neg) {
    Fq12Conj(&t0, &t0);
  }
  frob_op(&t2, &t0, 1, state);
  Fq12Mul(&t2, &t2, &t0);
  Fq12Conj(&t2, &t2);
  Fq12SqCyc(&t0, &t2);
  frob_op(&t2, &t3, 1, state);
  Fq12Mul(&t2, &t2, &t1);
  Fq12Conj(&t2, &t2);
  Fq12Mul(&t0, &t0, &t2);
  Fq12Conj(&t2, &t3);
  Fq12Mul(&t0, &t0, &t2);
  frob_op(&t1, &t1, 1, state);
  Fq12Conj(&t1, &t1);
  Fq12Mul(&t1, &t1, &t2);
  Fq12Mul(&t1, &t1, &t0);
  frob_op(&t2, &t3, 2, state);
  Fq12Mul(&t0, &t0, &t2);
  Fq12SqCyc(&t1, &t1);
  Fq12Mul(&t1, &t1, &t0);
  Fq12SqCyc(&t1, &t1);
  Fq12Conj(&t2, f);
  Fq12Mul(&t0, &t1, &t2);
  frob_op(&t2, f, 1, state);
  Fq12Mul(&t1, &t1, &t2);
  frob_op(&t2, f, 2, state);
  Fq12Mul(&t1, &t1, &t2);
  frob_op(&t2, f, 3, state);
  Fq12Mul(&t1, &t1, &t2);
  Fq12SqCyc(&t0, &t0);
  Fq12Mul(f, &t1, &t0);
}

static void pair_tangent(Fq12Elem* f, Fq2Elem* X, Fq2Elem* Y, Fq2Elem* Z,
                         Fq2Elem* Z2, EccPointFq const* P) {
  Fq2Mul(&f->z0.y0, X, X);
  Fq2Add(&f->z0.y2, &f->z0.y0, &f->z0.y0);
  Fq2Add(&f->z0.y2, &f->z0.y2, &f->z0.y0);
  Fq2Add(&f->z1.y1, X, &f->z0.y2);
  Fq2Mul(&f->z1.y1, &f->z1.y1, &f->z1.y1);
  Fq2Sub(&f->z1.y1, &f->z1.y1, &f->z0.y0);
  Fq2Mul(&f->z0.y1, Y, Y);
  Fq2Add(&f->z1.y0, &f->z0.y1, X);
  Fq2Mul(&f->z1.y0, &f->z1.y0, &f->z1.y0);
  Fq2Sub(&f->z1.y0, &f->z1.y0, &f->z0.y0);
  Fq2Mul(&f->z0.y0, &f->z0.y1, &f->z0.y1);
  Fq2Sub(&f->z1.y0, &f->z1.y0, &f->z0.y0);
  Fq2Add(&f->z1.y0, &f->z1.y0, &f->z1.y0);
  Fq2Mul(&f->z1.y2, &f->z0.y2, &f->z0.y2);
  Fq2Add(Z, Y, Z);
  Fq2Mul(Z, Z, Z);
  Fq2Sub(Z, Z, &f->z0.y1);
  Fq2Sub(Z, Z, Z2);
  Fq2Add(&f->z0.y1, &f->z0.y1, &f->z0.y1);
  Fq2Add(&f->z0.y1, &f->z0.y1, &f->z0.y1);
  Fq2Sub(&f->z1.y1, &f->z1.y1, &f->z1.y2);
  Fq2Sub(&f->z1.y1, &f->z1.y1, &f->z0.y1);
  Fq2Sub(X, &f->z1.y2, &f->z1.y0);
  Fq2Sub(X, X, &f->z1.y0);
  Fq2Add(&f->z0.y0, &f->z0.y0, &f->z0.y0);
  Fq2Add(&f->z0.y0, &f->z0.y0, &f->z0.y0);
  Fq2Add(&f->z0.y0, &f->z0.y0, &f->z0.y0);
  Fq2Sub(Y, &f->z1.y0, X);
  Fq2Mul(Y, Y, &f->z0.y2);
  Fq2Sub(Y, Y, &f->z0.y0);
  Fq2Mul(&f->z1.y0, &f->z0.y2, Z2);
  Fq2Neg(&f->z1.y0, &f->z1.y0);
  Fq2Add(&f->z1.y0, &f->z1.y0, &f->z1.y0);
  Fq2MulScalar(&f->z1.y0, &f->z1.y0, &P->x);
  Fq2Mul(&f->z0.y0, Z, Z2);
  Fq2Add(&f->z0.y0, &f->z0.y0, &f->z0.y0);
  Fq2MulScalar(&f->z0.y0, &f->z0.y0, &P->y);
  Fq2Mul(Z2, Z, Z);
  Fq2Clear(&f->z0.y1);
  Fq2Clear(&f->z0.y2);
  Fq2Clear(&f->z1.y2);
  Fq2Square(Z2, Z);
}

static void pair_line(Fq12Elem* f, Fq2Elem* X, Fq2Elem* Y, Fq2Elem* Z,
                      Fq2Elem* Z2, EccPointFq const* P, EccPointFq2 const* Q) {
  Fq2Mul(&f->z0.y1, &Q->x, Z2);
  Fq2Add(&f->z1.y0, &Q->y, Z);
  Fq2Mul(&f->z1.y0, &f->z1.y0, &f->z1.y0);
  Fq2Square(&f->z0.y0, &Q->y);
  Fq2Sub(&f->z1.y0, &f->z1.y0, &f->z0.y0);
  Fq2Sub(&f->z1.y0, &f->z1.y0, Z2);
  Fq2Mul(&f->z1.y0, &f->z1.y0, Z2);
  Fq2Sub(&f->z0.y1, &f->z0.y1, X);
  Fq2Mul(&f->z0.y2, &f->z0.y1, &f->z0.y1);
  Fq2Add(Z, Z, &f->z0.y1);
  Fq2Square(Z, Z);
  Fq2Sub(Z, Z, Z2);
  Fq2Sub(Z, Z, &f->z0.y2);
  Fq2Mul(Z2, Z, Z);
  Fq2Add(&f->z1.y2, &Q->y, Z);
  Fq2Mul(&f->z1.y2, &f->z1.y2, &f->z1.y2);
  Fq2Sub(&f->z1.y2, &f->z1.y2, &f->z0.y0);
  Fq2Sub(&f->z1.y2, &f->z1.y2, Z2);
  Fq2Sub(&f->z1.y0, &f->z1.y0, Y);
  Fq2Sub(&f->z1.y0, &f->z1.y0, Y);
  Fq2Mul(&f->z1.y1, &f->z1.y0, &Q->x);
  Fq2Add(&f->z1.y1, &f->z1.y1, &f->z1.y1);
  Fq2Sub(&f->z1.y1, &f->z1.y1, &f->z1.y2);
  Fq2Add(&f->z0.y2, &f->z0.y2, &f->z0.y2);
  Fq2Add(&f->z0.y2, &f->z0.y2, &f->z0.y2);
  Fq2Mul(&f->z0.y1, &f->z0.y2, &f->z0.y1);
  Fq2Mul(&f->z0.y2, X, &f->z0.y2);
  Fq2Mul(X, &f->z1.y0, &f->z1.y0);
  Fq2Sub(X, X, &f->z0.y1);
  Fq2Sub(X, X, &f->z0.y2);
  Fq2Sub(X, X, &f->z0.y2);
  Fq2Sub(&f->z0.y2, &f->z0.y2, X);
  Fq2Mul(&f->z0.y2, &f->z0.y2, &f->z1.y0);
  Fq2Mul(&f->z0.y1, Y, &f->z0.y1);
  Fq2Add(&f->z0.y1, &f->z0.y1, &f->z0.y1);
  Fq2Sub(Y, &f->z0.y2, &f->z0.y1);
  Fq2MulScalar(&f->z0.y0, Z, &P->y);
  Fq2Add(&f->z0.y0, &f->z0.y0, &f->z0.y0);
  Fq2Neg(&f->z1.y0, &f->z1.y0);
  Fq2MulScalar(&f->z1.y0, &f->z1.y0, &P->x);
  Fq2Add(&f->z1.y0, &f->z1.y0, &f->z1.y0);
  Fq2Clear(&f->z0.y1);
  Fq2Clear(&f->z0.y2);
  Fq2Clear(&f->z1.y2);
}

void PairingCompute(Fq12Elem* d, EccPointFq const* P, EccPointFq2 const* Q,
                    PairingState const* state) {
  Fq2Elem X;
  Fq2Elem Y;
  Fq2Elem Z;
  Fq2Elem Z2;
  EccPointFq2 Qp;
  Fq12Elem f;
  VeryLargeInt s;
  const VeryLargeInt two = {{2}};
  uint32_t i;

  VliAdd(&s, &epid_t, &epid_t);  // s = 2*t
  VliAdd(&s, &s, &epid_t);       // s = 3*t
  VliAdd(&s, &s, &s);            // s = 6*t
  if (neg) {
    VliSub(&s, &s, &two);
  } else {
    VliAdd(&s, &s, &two);
  }
  Fq2Cp(&X, &Q->x);
  Fq2Cp(&Y, &Q->y);
  Fq2Set(&Z, 1);
  Fq2Set(&Z2, 1);
  Fq12Set(d, 1);

  Fq12Clear(&f);
  // s has 66 bits, 0 through 65, so starting point is bit 64
  i = 65;
  while (i > 0) {
    i -= 1;
    pair_tangent(&f, &X, &Y, &Z, &Z2, P);
    Fq12Square(d, d);
    Fq12MulSpecial(d, d, &f);
    if (VliTestBit(&s, i)) {
      pair_line(&f, &X, &Y, &Z, &Z2, P, Q);
      Fq12MulSpecial(d, d, &f);
    }
  }
  if (neg) {
    Fq2Neg(&Y, &Y);
    Fq12Conj(d, d);
  }
  piOp(&Qp, Q, 1, state);
  pair_line(&f, &X, &Y, &Z, &Z2, P, &Qp);
  Fq12MulSpecial(d, d, &f);
  piOp(&Qp, Q, 2, state);
  Fq2Neg(&Qp.y, &Qp.y);
  pair_line(&f, &X, &Y, &Z, &Z2, P, &Qp);
  Fq12MulSpecial(d, d, &f);
  finalExp(d, state);
  // s doesn't have secret information; no need to clear it.
  Fq12Clear(&f);
  Fq2Clear(&X);
  Fq2Clear(&Y);
  Fq2Clear(&Z);
  Fq2Clear(&Z2);
  Fq2Clear(&Qp.x);
  Fq2Clear(&Qp.y);
}
