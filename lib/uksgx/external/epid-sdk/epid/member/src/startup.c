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
/// EpidMemberStartup implementation.
/*!
 * \file
 */

#include <epid/member/api.h>

#include <string.h>
#include "epid/common/math/ecgroup.h"
#include "epid/common/math/finitefield.h"
#include "epid/common/src/epid2params.h"
#include "epid/common/types.h"  // MemberPrecomp
#include "epid/member/src/context.h"
#include "epid/member/src/precomp.h"
#include "epid/member/src/storage.h"

/// Handle SDK Error with Break
#define BREAK_ON_EPID_ERROR(ret) \
  if (kEpidNoErr != (ret)) {     \
    break;                       \
  }

static EpidStatus MemberReadPrecomputation(MemberCtx* ctx,
                                           MemberPrecomp const* precomp);

EpidStatus EpidMemberStartup(MemberCtx* ctx) {
  EpidStatus sts = kEpidErr;
  uint32_t const nv_index = 0x01c10100;
  if (!ctx) {
    return kEpidBadArgErr;
  }

  do {
    EcGroup* G1 = ctx->epid2_params->G1;
    EcGroup* G2 = ctx->epid2_params->G2;
    FiniteField* Fp = ctx->epid2_params->Fp;
    EcPoint* A = (EcPoint*)ctx->A;
    FfElement* x = (FfElement*)ctx->x;
    EcPoint* h1 = (EcPoint*)ctx->h1;
    EcPoint* h2 = (EcPoint*)ctx->h2;
    EcPoint* w = (EcPoint*)ctx->w;
    sts = EpidNvReadMembershipCredential(ctx->tpm2_ctx, nv_index, &ctx->pub_key,
                                         &ctx->credential);
    BREAK_ON_EPID_ERROR(sts);
    if (!ctx->precomp_ready) {
      sts = PrecomputeMemberPairing(ctx->epid2_params, &ctx->pub_key,
                                    &ctx->credential.A, &ctx->precomp);
      BREAK_ON_EPID_ERROR(sts);
      ctx->precomp_ready = true;
    }

    if (!ctx->is_provisioned && !ctx->is_initially_provisioned) {
      sts = EpidMemberInitialProvision(ctx);
      BREAK_ON_EPID_ERROR(sts);
    }

    sts = ReadEcPoint(G1, &ctx->credential.A, sizeof(ctx->credential.A), A);
    BREAK_ON_EPID_ERROR(sts);

    sts = ReadFfElement(Fp, &ctx->credential.x, sizeof(ctx->credential.x), x);
    BREAK_ON_EPID_ERROR(sts);

    sts = ReadEcPoint(G1, &ctx->pub_key.h1, sizeof(ctx->pub_key.h1), h1);
    BREAK_ON_EPID_ERROR(sts);

    sts = ReadEcPoint(G1, &ctx->pub_key.h2, sizeof(ctx->pub_key.h2), h2);
    BREAK_ON_EPID_ERROR(sts);

    sts = ReadEcPoint(G2, &ctx->pub_key.w, sizeof(ctx->pub_key.w), w);
    BREAK_ON_EPID_ERROR(sts);

    sts = MemberReadPrecomputation(ctx, &ctx->precomp);
    BREAK_ON_EPID_ERROR(sts);

    sts = kEpidNoErr;
  } while (0);

  return sts;
}

static EpidStatus MemberReadPrecomputation(MemberCtx* ctx,
                                           MemberPrecomp const* precomp) {
  EpidStatus sts = kEpidErr;

  if (!ctx || !precomp || !ctx->epid2_params || !ctx) {
    return kEpidBadArgErr;
  }

  do {
    FiniteField* GT = ctx->epid2_params->GT;
    FfElement* e12 = (FfElement*)ctx->e12;
    FfElement* e22 = (FfElement*)ctx->e22;
    FfElement* e2w = (FfElement*)ctx->e2w;
    FfElement* ea2 = (FfElement*)ctx->ea2;

    sts = ReadFfElement(GT, &precomp->e12, sizeof(precomp->e12), e12);
    BREAK_ON_EPID_ERROR(sts);

    sts = ReadFfElement(GT, &precomp->e22, sizeof(precomp->e22), e22);
    BREAK_ON_EPID_ERROR(sts);

    sts = ReadFfElement(GT, &precomp->e2w, sizeof(precomp->e2w), e2w);
    BREAK_ON_EPID_ERROR(sts);

    sts = ReadFfElement(GT, &precomp->ea2, sizeof(precomp->ea2), ea2);
    BREAK_ON_EPID_ERROR(sts);

    sts = kEpidNoErr;
  } while (0);

  return sts;
}
