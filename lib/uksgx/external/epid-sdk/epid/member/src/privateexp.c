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
/// Member private exponentiation implementation
/*! \file */

#include "epid/member/src/privateexp.h"

#include "epid/common/math/ecgroup.h"
#include "epid/common/src/epid2params.h"
#include "epid/common/src/hashsize.h"
#include "epid/common/src/memory.h"
#include "epid/common/types.h"
#include "epid/member/src/context.h"
#include "epid/member/tpm2/commit.h"
#include "epid/member/tpm2/sign.h"

/// Handle Intel(R) EPID Error with Break
#define BREAK_ON_EPID_ERROR(ret) \
  if (kEpidNoErr != (ret)) {     \
    break;                       \
  }

EpidStatus EpidPrivateExp(MemberCtx* ctx, EcPoint const* a, EcPoint* r) {
  EpidStatus sts = kEpidErr;

  BigNumStr tmp_ff_str = {0};
  uint16_t counter = 0;

  EcPoint* k_pt = NULL;
  EcPoint* l_pt = NULL;
  EcPoint* e_pt = NULL;
  EcPoint* t1 = NULL;
  EcPoint* h = NULL;

  FfElement* k = NULL;
  FfElement* s = NULL;

  size_t digest_len = 0;
  uint8_t* digest = NULL;

  if (!ctx || !ctx->epid2_params || !a || !r) {
    return kEpidBadArgErr;
  }

  digest_len = EpidGetHashSize(ctx->hash_alg);
  digest = SAFE_ALLOC(digest_len);
  if (!digest) {
    return kEpidMemAllocErr;
  }

  memset(digest, 0, digest_len);
  digest[digest_len - 1] = 1;

  do {
    FiniteField* Fp = ctx->epid2_params->Fp;
    EcGroup* G1 = ctx->epid2_params->G1;

    if (!ctx->is_provisioned && !ctx->is_initially_provisioned) {
      sts = EpidMemberInitialProvision(ctx);
      BREAK_ON_EPID_ERROR(sts);
    }

    // (K_PT, L_PT, E_PT, counter) = TPM2_Commit(P1=B')
    sts = NewEcPoint(G1, &k_pt);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewEcPoint(G1, &l_pt);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewEcPoint(G1, &e_pt);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewEcPoint(G1, &t1);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewEcPoint(G1, &h);
    BREAK_ON_EPID_ERROR(sts);

    sts =
        Tpm2Commit(ctx->tpm2_ctx, a, NULL, 0, NULL, k_pt, l_pt, e_pt, &counter);
    BREAK_ON_EPID_ERROR(sts);

    // (k, s) = TPM2_Sign(c=1, counter)
    sts = NewFfElement(Fp, &k);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewFfElement(Fp, &s);
    BREAK_ON_EPID_ERROR(sts);

    sts = Tpm2Sign(ctx->tpm2_ctx, digest, digest_len, counter, k, s);
    BREAK_ON_EPID_ERROR(sts);

    // k = Fq.inv(k)
    sts = FfInv(Fp, k, k);
    BREAK_ON_EPID_ERROR(sts);

    // t1 = G1.sscmExp(B', s)
    sts = WriteFfElement(Fp, s, &tmp_ff_str, sizeof(tmp_ff_str));
    BREAK_ON_EPID_ERROR(sts);
    sts = EcSscmExp(G1, a, &tmp_ff_str, t1);
    BREAK_ON_EPID_ERROR(sts);

    // E_PT = G1.inv(E_PT)
    sts = EcInverse(G1, e_pt, e_pt);
    BREAK_ON_EPID_ERROR(sts);

    // h = G1.mul(t1, E_PT)
    sts = EcMul(G1, t1, e_pt, h);
    BREAK_ON_EPID_ERROR(sts);

    // h = G1.sscmExp(h, k)
    sts = WriteFfElement(Fp, k, &tmp_ff_str, sizeof(tmp_ff_str));
    BREAK_ON_EPID_ERROR(sts);
    sts = EcSscmExp(G1, h, &tmp_ff_str, r);
    BREAK_ON_EPID_ERROR(sts);
  } while (0);

  if (sts != kEpidNoErr) {
    (void)Tpm2ReleaseCounter(ctx->tpm2_ctx, counter);
  }
  DeleteFfElement(&s);
  DeleteFfElement(&k);

  DeleteEcPoint(&e_pt);
  DeleteEcPoint(&l_pt);
  DeleteEcPoint(&k_pt);
  DeleteEcPoint(&t1);
  DeleteEcPoint(&h);

  EpidZeroMemory(&tmp_ff_str, sizeof(tmp_ff_str));
  SAFE_FREE(digest);
  return sts;
}
