/*############################################################################
  # Copyright 2016-2017 Intel Corporation
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
/// EpidSignBasic implementation.
/*! \file */
#include "epid/member/src/signbasic.h"

#include <string.h>  // memset

#include "epid/common/math/ecgroup.h"
#include "epid/common/math/finitefield.h"
#include "epid/common/src/endian_convert.h"
#include "epid/common/src/epid2params.h"
#include "epid/common/src/hashsize.h"
#include "epid/common/src/memory.h"
#include "epid/member/api.h"
#include "epid/member/src/allowed_basenames.h"
#include "epid/member/src/context.h"
#include "epid/member/src/hash_basename.h"
#include "epid/member/src/presig-internal.h"
#include "epid/member/src/sign_commitment.h"
#include "epid/member/tpm2/commit.h"
#include "epid/member/tpm2/sign.h"

/// Handle SDK Error with Break
#define BREAK_ON_EPID_ERROR(ret) \
  if (kEpidNoErr != (ret)) {     \
    break;                       \
  }

/// Count of elements in array
#define COUNT_OF(A) (sizeof(A) / sizeof((A)[0]))

EpidStatus EpidSignBasic(MemberCtx const* ctx, void const* msg, size_t msg_len,
                         void const* basename, size_t basename_len,
                         BasicSignature* sig, BigNumStr* rnd_bsn) {
  EpidStatus sts = kEpidErr;

  EcPoint* B = NULL;
  EcPoint* t = NULL;  // temp value in G1
  EcPoint* k = NULL;
  EcPoint* e = NULL;
  FfElement* R2 = NULL;
  FfElement* p2y = NULL;
  FfElement* t1 = NULL;
  FfElement* t2 = NULL;

  FfElement* a = NULL;
  FfElement* b = NULL;
  FfElement* rx = NULL;
  FfElement* ra = NULL;
  FfElement* rb = NULL;

  struct p2x_t {
    uint32_t i;
    uint8_t bsn[1];
  }* p2x = NULL;

  FfElement* t3 = NULL;  // temporary for multiplication
  FfElement* c = NULL;
  uint8_t* digest = NULL;

  PreComputedSignature curr_presig = {0};

  if (!ctx || !sig) {
    return kEpidBadArgErr;
  }
  if (!msg && (0 != msg_len)) {
    // if message is non-empty it must have both length and content
    return kEpidBadArgErr;
  }
  if (!basename && (0 != basename_len)) {
    // if basename is non-empty it must have both length and content
    return kEpidBadArgErr;
  }
  if (!ctx->epid2_params) {
    return kEpidBadArgErr;
  }

  do {
    FiniteField* Fp = ctx->epid2_params->Fp;
    SignCommitOutput commit_out = {0};
    FpElemStr c_str = {0};
    EcGroup* G1 = ctx->epid2_params->G1;
    FiniteField* GT = ctx->epid2_params->GT;

    FiniteField* Fq = ctx->epid2_params->Fq;
    PairingState* ps_ctx = ctx->epid2_params->pairing_state;
    const BigNumStr kOne = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    BigNumStr t1_str = {0};
    BigNumStr t2_str = {0};
    size_t digest_size = 0;
    uint16_t* rf_ctr = (uint16_t*)&ctx->rf_ctr;
    FfElement const* x = ctx->x;

    if (basename) {
      if (!IsBasenameAllowed(ctx->allowed_basenames, basename, basename_len)) {
        sts = kEpidBadArgErr;
        BREAK_ON_EPID_ERROR(sts);
      }
    }

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
    sts = NewFfElement(Fq, &p2y);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewFfElement(Fp, &t1);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewFfElement(Fp, &t2);
    BREAK_ON_EPID_ERROR(sts);
    p2x = (struct p2x_t*)SAFE_ALLOC(sizeof(struct p2x_t) + basename_len - 1);
    if (!p2x) {
      sts = kEpidMemAllocErr;
      break;
    }

    sts = NewFfElement(Fp, &a);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewFfElement(Fp, &b);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewFfElement(Fp, &rx);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewFfElement(Fp, &ra);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewFfElement(Fp, &rb);
    BREAK_ON_EPID_ERROR(sts);

    sts = MemberGetPreSig((MemberCtx*)ctx, &curr_presig);
    BREAK_ON_EPID_ERROR(sts);

    // 3.  If the pre-computed signature pre-sigma exists, the member
    //     loads (B, K, T, a, b, rx, rf, ra, rb, R1, R2) from
    //     pre-sigma. Refer to Section 4.4 for the computation of
    //     these values.
    sts = ReadFfElement(Fp, &curr_presig.a, sizeof(curr_presig.a), a);
    BREAK_ON_EPID_ERROR(sts);
    sts = ReadFfElement(Fp, &curr_presig.b, sizeof(curr_presig.b), b);
    BREAK_ON_EPID_ERROR(sts);
    sts = ReadFfElement(Fp, &curr_presig.rx, sizeof(curr_presig.rx), rx);
    BREAK_ON_EPID_ERROR(sts);
    sts = ReadFfElement(Fp, &curr_presig.ra, sizeof(curr_presig.ra), ra);
    BREAK_ON_EPID_ERROR(sts);
    sts = ReadFfElement(Fp, &curr_presig.rb, sizeof(curr_presig.rb), rb);
    BREAK_ON_EPID_ERROR(sts);

    // If the basename is provided, use it, otherwise use presig B
    if (basename) {
      // 3.a. The member computes (B, i2, y2) = G1.tpmHash(bsn).
      sts = EcHash(G1, basename, basename_len, ctx->hash_alg, B, &p2x->i);
      BREAK_ON_EPID_ERROR(sts);
      p2x->i = htonl(p2x->i);
      sts = WriteEcPoint(G1, B, &commit_out.B, sizeof(commit_out.B));
      BREAK_ON_EPID_ERROR(sts);
      sts = ReadFfElement(Fq, &commit_out.B.y, sizeof(commit_out.B.y), p2y);
      BREAK_ON_EPID_ERROR(sts);

      // b.i. (KTPM, LTPM, ETPM, counterTPM) = TPM2_Commit(P1=h1,(s2, y2) = (i2
      // || bsn, y2)).
      // b.ii.K = KTPM.
      if (0 !=
          memcpy_S((void*)p2x->bsn, basename_len, basename, basename_len)) {
        sts = kEpidBadArgErr;
        break;
      }
      sts =
          Tpm2Commit(ctx->tpm2_ctx, ctx->h1, p2x, sizeof(p2x->i) + basename_len,
                     p2y, k, t, e, (uint16_t*)&ctx->rf_ctr);
      BREAK_ON_EPID_ERROR(sts);
      sts = WriteEcPoint(G1, k, &commit_out.K, sizeof(commit_out.K));
      BREAK_ON_EPID_ERROR(sts);
      // c.i. The member computes R1 = LTPM.
      sts = WriteEcPoint(G1, t, &commit_out.R1, sizeof(commit_out.R1));
      BREAK_ON_EPID_ERROR(sts);
      // c.ii. e12rf = pairing(ETPM, g2)
      sts = Pairing(ps_ctx, e, ctx->epid2_params->g2, R2);
      BREAK_ON_EPID_ERROR(sts);
      // c.iii. R2 = GT.sscmMultiExp(ea2, t1, e12rf, 1, e22, t2, e2w,ra).
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
        exponents[3] = (BigNumStr*)&curr_presig.ra;
        sts = FfMultiExp(GT, points, exponents, COUNT_OF(points), R2);
        BREAK_ON_EPID_ERROR(sts);
      }

      sts = WriteFfElement(GT, R2, &commit_out.R2, sizeof(commit_out.R2));
      BREAK_ON_EPID_ERROR(sts);
      // d. The member over-writes the counterTPM, B, K, R1 and R2 values.
    } else {
      if (!rnd_bsn) {
        sts = kEpidBadArgErr;
        break;
      }
      sts = ReadEcPoint(G1, &curr_presig.B, sizeof(curr_presig.B), B);
      BREAK_ON_EPID_ERROR(sts);
      commit_out.B = curr_presig.B;
      commit_out.K = curr_presig.K;
      commit_out.R1 = curr_presig.R1;
      ((MemberCtx*)ctx)->rf_ctr = curr_presig.rf_ctr;
      commit_out.R2 = curr_presig.R2;
      *rnd_bsn = curr_presig.rnd_bsn;
    }

    commit_out.T = curr_presig.T;

    sts = HashSignCommitment(Fp, ctx->hash_alg, &ctx->pub_key, &commit_out, msg,
                             msg_len, &c_str);
    BREAK_ON_EPID_ERROR(sts);

    digest_size = EpidGetHashSize(ctx->hash_alg);
    digest = (uint8_t*)SAFE_ALLOC(digest_size);
    if (!digest) {
      sts = kEpidNoMemErr;
      break;
    }
    memcpy_S(digest + digest_size - sizeof(c_str), sizeof(c_str), &c_str,
             sizeof(c_str));

    sts = NewFfElement(Fp, &t3);
    BREAK_ON_EPID_ERROR(sts);

    sts = NewFfElement(Fp, &c);
    BREAK_ON_EPID_ERROR(sts);

    sts = ReadFfElement(Fp, &c_str, sizeof(c_str), c);
    BREAK_ON_EPID_ERROR(sts);

    // 7.  The member computes sx = (rx + c * x) mod p.
    sts = FfMul(Fp, c, x, t3);
    BREAK_ON_EPID_ERROR(sts);
    sts = FfAdd(Fp, rx, t3, t3);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteFfElement(Fp, t3, &sig->sx, sizeof(sig->sx));
    BREAK_ON_EPID_ERROR(sts);

    // 8.  The member computes sf = (rf + c * f) mod p.
    sts = Tpm2Sign(ctx->tpm2_ctx, digest, digest_size, *rf_ctr, NULL, t3);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteFfElement(Fp, t3, &sig->sf, sizeof(sig->sf));
    BREAK_ON_EPID_ERROR(sts);

    // 9.  The member computes sa = (ra + c * a) mod p.
    sts = FfMul(Fp, c, a, t3);
    BREAK_ON_EPID_ERROR(sts);
    sts = FfAdd(Fp, ra, t3, t3);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteFfElement(Fp, t3, &sig->sa, sizeof(sig->sa));
    BREAK_ON_EPID_ERROR(sts);

    // 10. The member computes sb = (rb + c * b) mod p.
    sts = FfMul(Fp, c, b, t3);
    BREAK_ON_EPID_ERROR(sts);
    sts = FfAdd(Fp, rb, t3, t3);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteFfElement(Fp, t3, &sig->sb, sizeof(sig->sb));
    BREAK_ON_EPID_ERROR(sts);

    sig->B = commit_out.B;
    sig->K = commit_out.K;
    sig->T = commit_out.T;
    sig->c = c_str;

    sts = kEpidNoErr;
  } while (0);

  if (sts != kEpidNoErr) {
    (void)Tpm2ReleaseCounter(ctx->tpm2_ctx, (uint16_t)ctx->rf_ctr);
    (void)Tpm2ReleaseCounter(ctx->tpm2_ctx, curr_presig.rf_ctr);
  } else if (basename) {
    (void)Tpm2ReleaseCounter(ctx->tpm2_ctx, curr_presig.rf_ctr);
  }

  EpidZeroMemory(&curr_presig, sizeof(curr_presig));

  DeleteEcPoint(&B);
  DeleteEcPoint(&k);
  DeleteEcPoint(&t);
  DeleteEcPoint(&e);
  DeleteFfElement(&R2);
  DeleteFfElement(&p2y);
  DeleteFfElement(&t1);
  DeleteFfElement(&t2);

  DeleteFfElement(&a);
  DeleteFfElement(&b);
  DeleteFfElement(&rx);
  DeleteFfElement(&ra);
  DeleteFfElement(&rb);

  SAFE_FREE(p2x);

  DeleteFfElement(&t3);
  DeleteFfElement(&c);
  SAFE_FREE(digest);

  return sts;
}
