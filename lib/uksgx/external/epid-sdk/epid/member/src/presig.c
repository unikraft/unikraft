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
/// Sensitive pre-computed signature implementation
/*! \file */

#include <epid/member/api.h>

#include <string.h>

#include "epid/common/math/ecgroup.h"
#include "epid/common/math/finitefield.h"
#include "epid/common/src/endian_convert.h"
#include "epid/common/src/epid2params.h"
#include "epid/common/src/memory.h"
#include "epid/common/src/stack.h"
#include "epid/member/src/context.h"
#include "epid/member/tpm2/commit.h"
#include "epid/member/tpm2/context.h"
#include "epid/member/tpm2/getrandom.h"
#include "epid/member/tpm2/sign.h"

/// Handle SDK Error with Break
#define BREAK_ON_EPID_ERROR(ret) \
  if (kEpidNoErr != (ret)) {     \
    break;                       \
  }

/// Count of elements in array
#define COUNT_OF(A) (sizeof(A) / sizeof((A)[0]))

static EpidStatus MemberComputePreSig(MemberCtx const* ctx,
                                      PreComputedSignature* precompsig);

EpidStatus EpidAddPreSigs(MemberCtx* ctx, size_t number_presigs) {
  PreComputedSignature* new_presigs = NULL;
  size_t i = 0;
  if (!ctx || !ctx->presigs) return kEpidBadArgErr;

  if (0 == number_presigs) return kEpidNoErr;

  new_presigs =
      (PreComputedSignature*)StackPushN(ctx->presigs, number_presigs, NULL);
  if (!new_presigs) return kEpidMemAllocErr;

  for (i = 0; i < number_presigs; i++) {
    EpidStatus sts = MemberComputePreSig(ctx, &new_presigs[i]);
    if (kEpidNoErr != sts) {
      // roll back pre-computed-signature pool
      StackPopN(ctx->presigs, number_presigs, 0);
      return sts;
    }
  }

  return kEpidNoErr;
}

size_t EpidGetNumPreSigs(MemberCtx const* ctx) {
  return (ctx && ctx->presigs) ? StackGetSize(ctx->presigs) : (size_t)0;
}

EpidStatus MemberGetPreSig(MemberCtx* ctx, PreComputedSignature* presig) {
  if (!ctx || !presig) {
    return kEpidBadArgErr;
  }

  if (StackGetSize(ctx->presigs)) {
    // Use existing pre-computed signature
    if (!StackPopN(ctx->presigs, 1, presig)) {
      return kEpidErr;
    }
    return kEpidNoErr;
  }
  // generate a new pre-computed signature
  return MemberComputePreSig(ctx, presig);
}

/// Performs Pre-computation that can be used to speed up signing
EpidStatus MemberComputePreSig(MemberCtx const* ctx,
                               PreComputedSignature* precompsig) {
  EpidStatus sts = kEpidErr;

  EcPoint* B = NULL;
  EcPoint* k = NULL;
  EcPoint* t = NULL;  // temporary, used for K, T, R1
  EcPoint* e = NULL;

  FfElement* R2 = NULL;

  FfElement* a = NULL;
  FfElement* rx = NULL;  // reused for rf
  FfElement* rb = NULL;  // reused for ra

  FfElement* t1 = NULL;
  FfElement* t2 = NULL;
  BigNumStr t1_str = {0};
  BigNumStr t2_str = {0};
  struct {
    uint32_t i;
    BigNumStr bsn;
  } p2x = {0};
  FfElement* p2y = NULL;

  if (!ctx || !precompsig || !ctx->epid2_params) {
    return kEpidBadArgErr;
  }

  do {
    // handy shorthands:
    Tpm2Ctx* tpm = ctx->tpm2_ctx;
    EcGroup* G1 = ctx->epid2_params->G1;
    FiniteField* GT = ctx->epid2_params->GT;
    FiniteField* Fp = ctx->epid2_params->Fp;
    FiniteField* Fq = ctx->epid2_params->Fq;
    EcPoint const* h2 = ctx->h2;
    EcPoint const* A = ctx->A;
    FfElement const* x = ctx->x;
    PairingState* ps_ctx = ctx->epid2_params->pairing_state;

    const BigNumStr kOne = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    // 1. The member expects the pre-computation is done (e12, e22, e2w,
    //    ea2). Refer to Section 3.5 for the computation of these
    //    values.

    sts = NewFfElement(Fq, &p2y);
    // The following variables B, K, T, R1 (elements of G1), R2
    // (elements of GT), a, b, rx, rf, ra, rb, t1, t2 (256-bit
    // integers) are used.
    BREAK_ON_EPID_ERROR(sts);
    sts = NewEcPoint(G1, &B);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewEcPoint(G1, &k);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewEcPoint(G1, &t);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewEcPoint(G1, &e);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewFfElement(GT, &R2);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewFfElement(Fp, &a);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewFfElement(Fp, &rx);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewFfElement(Fp, &rb);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewFfElement(Fp, &t1);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewFfElement(Fp, &t2);
    BREAK_ON_EPID_ERROR(sts);

    // 3. The member computes B = G1.getRandom().
    // 4.a. If bsn is not provided, the member chooses randomly an integer bsn
    // from [1, p-1].
    sts = Tpm2GetRandom(tpm, sizeof(p2x.bsn) * 8, &p2x.bsn);
    BREAK_ON_EPID_ERROR(sts);
    precompsig->rnd_bsn = p2x.bsn;

    // 4.b. The member computes (B, i2, y2) = G1.tpmHash(bsn).
    sts = EcHash(G1, (const void*)&p2x.bsn, sizeof(p2x.bsn), ctx->hash_alg, B,
                 &p2x.i);
    BREAK_ON_EPID_ERROR(sts);
    p2x.i = htonl(p2x.i);
    sts = WriteEcPoint(G1, B, &precompsig->B, sizeof(precompsig->B));
    BREAK_ON_EPID_ERROR(sts);
    sts = ReadFfElement(Fq, &precompsig->B.y, sizeof(precompsig->B.y), p2y);
    BREAK_ON_EPID_ERROR(sts);

    // 4.c. (KTPM, LTPM, ETPM, counterTPM) = TPM2_Commit(P1=h1, (s2, y2) = (i1
    // || bsn, y2)), K = KTPM
    sts = Tpm2Commit(tpm, ctx->h1, &p2x, sizeof(p2x), p2y, k, t, e,
                     &precompsig->rf_ctr);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteEcPoint(G1, k, &precompsig->K, sizeof(precompsig->K));
    BREAK_ON_EPID_ERROR(sts);
    // 4.k. The member computes R1 = LTPM.
    sts = WriteEcPoint(G1, t, &precompsig->R1, sizeof(precompsig->R1));
    BREAK_ON_EPID_ERROR(sts);

    // 4.d. The member chooses randomly an integer a from [1, p-1].
    sts = FfGetRandom(Fp, &kOne, ctx->rnd_func, ctx->rnd_param, a);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteFfElement(Fp, a, &precompsig->a, sizeof(precompsig->a));
    BREAK_ON_EPID_ERROR(sts);
    // 4.e. The member computes T = G1.sscmExp(h2, a).
    sts = EcExp(G1, h2, (BigNumStr*)&precompsig->a, t);
    BREAK_ON_EPID_ERROR(sts);
    // 4.k. The member computes T = G1.mul(T, A).
    sts = EcMul(G1, t, A, t);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteEcPoint(G1, t, &precompsig->T, sizeof(precompsig->T));
    BREAK_ON_EPID_ERROR(sts);

    // 4.h. The member chooses rx, ra, rb randomly from [1, p-1].

    // note : rb are reused as ra
    sts = FfGetRandom(Fp, &kOne, ctx->rnd_func, ctx->rnd_param, rx);
    BREAK_ON_EPID_ERROR(sts);
    sts = FfGetRandom(Fp, &kOne, ctx->rnd_func, ctx->rnd_param, rb);
    BREAK_ON_EPID_ERROR(sts);

    sts = WriteFfElement(Fp, rx, &precompsig->rx, sizeof(precompsig->rx));
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteFfElement(Fp, rb, &precompsig->rb, sizeof(precompsig->rb));
    BREAK_ON_EPID_ERROR(sts);

    // 4.i. The member computes t1 = (- rx) mod p.
    sts = FfNeg(Fp, rx, t1);
    BREAK_ON_EPID_ERROR(sts);

    // 4.j. The member computes t2 = (rb - a * rx) mod p.
    sts = FfMul(Fp, a, rx, t2);
    BREAK_ON_EPID_ERROR(sts);
    sts = FfNeg(Fp, t2, t2);
    BREAK_ON_EPID_ERROR(sts);
    sts = FfAdd(Fp, rb, t2, t2);
    BREAK_ON_EPID_ERROR(sts);

    // 4.g. The member computes b = (a * x) mod p.
    sts = FfMul(Fp, a, x, a);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteFfElement(Fp, a, &precompsig->b, sizeof(precompsig->b));
    BREAK_ON_EPID_ERROR(sts);

    // reusing rb as ra
    sts = FfGetRandom(Fp, &kOne, ctx->rnd_func, ctx->rnd_param, rb);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteFfElement(Fp, rb, &precompsig->ra, sizeof(precompsig->ra));
    BREAK_ON_EPID_ERROR(sts);

    // 4.l.i e12rf = pairing(ETPM, g2)
    sts = Pairing(ps_ctx, e, ctx->epid2_params->g2, R2);
    BREAK_ON_EPID_ERROR(sts);

    // 4.l.ii. The member computes R2 = GT.sscmMultiExp(ea2, t1, e12rf, 1,
    // e22, t2, e2w, ra).
    sts = WriteFfElement(Fp, t1, &t1_str, sizeof(t1_str));
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteFfElement(Fp, t2, &t2_str, sizeof(t2_str));
    BREAK_ON_EPID_ERROR(sts);
    {
      FfElement const* points[4];
      BigNumStr const* exponents[4];
      points[0] = ctx->ea2;
      points[1] = R2;
      points[2] = ctx->e22;
      points[3] = ctx->e2w;
      exponents[0] = &t1_str;
      exponents[1] = &kOne;
      exponents[2] = &t2_str;
      exponents[3] = (BigNumStr*)&precompsig->ra;
      sts = FfMultiExp(GT, points, exponents, COUNT_OF(points), R2);
      BREAK_ON_EPID_ERROR(sts);
    }

    sts = WriteFfElement(GT, R2, &precompsig->R2, sizeof(precompsig->R2));
    BREAK_ON_EPID_ERROR(sts);

    sts = kEpidNoErr;
  } while (0);

  if (sts != kEpidNoErr) {
    (void)Tpm2ReleaseCounter(ctx->tpm2_ctx, precompsig->rf_ctr);
  }

  EpidZeroMemory(&t1_str, sizeof(t1_str));
  EpidZeroMemory(&t2_str, sizeof(t2_str));
  EpidZeroMemory(&p2x, sizeof(p2x));

  DeleteFfElement(&p2y);
  DeleteEcPoint(&B);
  DeleteEcPoint(&k);
  DeleteEcPoint(&t);
  DeleteEcPoint(&e);
  DeleteFfElement(&R2);
  DeleteFfElement(&a);
  DeleteFfElement(&rx);
  DeleteFfElement(&rb);
  DeleteFfElement(&t1);
  DeleteFfElement(&t2);

  return sts;
}
