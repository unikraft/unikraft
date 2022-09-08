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
/// Join Request related implementation.
/*! \file */

#include <epid/member/api.h>

#include "epid/common/src/epid2params.h"
#include "epid/common/src/grouppubkey.h"
#include "epid/common/src/hashsize.h"
#include "epid/common/src/memory.h"
#include "epid/common/types.h"
#include "epid/member/src/context.h"
#include "epid/member/src/join_commitment.h"
#include "epid/member/src/privateexp.h"
#include "epid/member/src/resize.h"
#include "epid/member/tpm2/commit.h"
#include "epid/member/tpm2/sign.h"

/// Handle SDK Error with Break
#define BREAK_ON_EPID_ERROR(ret) \
  if (kEpidNoErr != (ret)) {     \
    break;                       \
  }

EpidStatus EpidCreateJoinRequest(MemberCtx* ctx, GroupPubKey const* pub_key,
                                 IssuerNonce const* ni,
                                 JoinRequest* join_request) {
  EpidStatus sts = kEpidErr;
  GroupPubKey_* pub_key_ = NULL;
  EcPoint* t = NULL;  // temporary used for F and R
  EcPoint* h1 = NULL;
  EcPoint* K = NULL;
  EcPoint* l = NULL;
  EcPoint* e = NULL;

  FfElement* k = NULL;
  FfElement* s = NULL;
  uint8_t* digest = NULL;

  if (!ctx || !pub_key || !ni || !join_request || !ctx->epid2_params) {
    return kEpidBadArgErr;
  }

  if (kSha256 != ctx->hash_alg && kSha384 != ctx->hash_alg &&
      kSha512 != ctx->hash_alg && kSha512_256 != ctx->hash_alg) {
    return kEpidBadArgErr;
  }

  do {
    JoinRequest request = {0};
    G1ElemStr R = {0};
    EcGroup* G1 = ctx->epid2_params->G1;
    FiniteField* Fp = ctx->epid2_params->Fp;
    size_t digest_size = 0;

    if (!ctx->is_provisioned && !ctx->is_initially_provisioned) {
      sts = EpidMemberInitialProvision(ctx);
      BREAK_ON_EPID_ERROR(sts);
    }

    // validate public key by creating
    sts = CreateGroupPubKey(pub_key, ctx->epid2_params->G1,
                            ctx->epid2_params->G2, &pub_key_);
    BREAK_ON_EPID_ERROR(sts);

    sts = NewEcPoint(G1, &t);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewEcPoint(G1, &h1);
    BREAK_ON_EPID_ERROR(sts);
    sts = ReadEcPoint(G1, &pub_key->h1, sizeof(pub_key->h1), h1);
    BREAK_ON_EPID_ERROR(sts);

    // 2. The member computes F = G1.sscmExp(h1, f).
    sts = EpidPrivateExp(ctx, h1, t);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteEcPoint(G1, t, &request.F, sizeof(request.F));
    BREAK_ON_EPID_ERROR(sts);

    // 1. The member chooses a random integer r from [1, p-1].
    // 3. The member computes R = G1.sscmExp(h1, r).
    sts = NewEcPoint(G1, &K);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewEcPoint(G1, &l);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewEcPoint(G1, &e);
    BREAK_ON_EPID_ERROR(sts);
    sts =
        Tpm2Commit(ctx->tpm2_ctx, h1, NULL, 0, NULL, K, l, e, &(ctx->join_ctr));
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteEcPoint(G1, e, &R, sizeof(R));
    BREAK_ON_EPID_ERROR(sts);

    sts = HashJoinCommitment(ctx->epid2_params->Fp, ctx->hash_alg, pub_key,
                             &request.F, &R, ni, &request.c);
    BREAK_ON_EPID_ERROR(sts);

    // Extend value c to be of a digest size.
    digest_size = EpidGetHashSize(ctx->hash_alg);
    digest = (uint8_t*)SAFE_ALLOC(digest_size);
    if (!digest) {
      sts = kEpidMemAllocErr;
      break;
    }
    sts = ResizeOctStr(&request.c, sizeof(request.c), digest, digest_size);
    BREAK_ON_EPID_ERROR(sts);

    // Step 5. The member computes s = (r + c * f) mod p.
    sts = NewFfElement(Fp, &k);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewFfElement(Fp, &s);
    BREAK_ON_EPID_ERROR(sts);
    sts = Tpm2Sign(ctx->tpm2_ctx, digest, digest_size, ctx->join_ctr, k, s);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteFfElement(Fp, s, &request.s, sizeof(request.s));
    BREAK_ON_EPID_ERROR(sts);

    // Step 6. The output join request is (F, c, s).
    *join_request = request;

    sts = kEpidNoErr;
  } while (0);

  if (sts != kEpidNoErr) {
    (void)Tpm2ReleaseCounter(ctx->tpm2_ctx, ctx->join_ctr);
  }

  DeleteEcPoint(&t);
  DeleteEcPoint(&h1);
  DeleteEcPoint(&K);
  DeleteEcPoint(&l);
  DeleteEcPoint(&e);
  DeleteFfElement(&k);
  DeleteFfElement(&s);
  SAFE_FREE(digest);
  DeleteGroupPubKey(&pub_key_);

  return sts;
}
