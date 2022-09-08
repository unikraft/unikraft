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

/*!
 * \file
 * \brief Member context implementation.
 */

#include <epid/member/api.h>

#include <string.h>
#include "epid/common/src/endian_convert.h"
#include "epid/common/src/epid2params.h"
#include "epid/common/src/memory.h"
#include "epid/common/src/sigrlvalid.h"
#include "epid/common/src/stack.h"
#include "epid/common/types.h"
#include "epid/member/software_member.h"
#include "epid/member/src/allowed_basenames.h"
#include "epid/member/src/context.h"
#include "epid/member/src/precomp.h"
#include "epid/member/tpm2/context.h"
#include "epid/member/tpm2/createprimary.h"
#include "epid/member/tpm2/load_external.h"
#include "epid/member/tpm2/sign.h"

/// Handle SDK Error with Break
#define BREAK_ON_EPID_ERROR(ret) \
  if (kEpidNoErr != (ret)) {     \
    break;                       \
  }

EpidStatus EpidMemberGetSize(MemberParams const* params, size_t* context_size) {
  if (!params || !context_size) {
    return kEpidBadArgErr;
  }
  *context_size = sizeof(MemberCtx);
  return kEpidNoErr;
}

EpidStatus EpidMemberInit(MemberParams const* params, MemberCtx* ctx) {
  EpidStatus sts = kEpidErr;

  if (!params || !ctx) {
    return kEpidBadArgErr;
  }
  memset(ctx, 0, sizeof(*ctx));
  do {
    const FpElemStr* f = NULL;

    // set the default hash algorithm to sha512
    ctx->hash_alg = kSha512;
#ifdef TPM_TSS  // if build for TSS, make Sha256 default
    ctx->hash_alg = kSha256;
#endif
    ctx->sig_rl = NULL;
    ctx->precomp_ready = false;
    ctx->is_initially_provisioned = false;
    ctx->is_provisioned = false;
    ctx->primary_key_set = false;

    sts = CreateBasenames(&ctx->allowed_basenames);
    BREAK_ON_EPID_ERROR(sts);
    // Internal representation of Epid2Params
    sts = CreateEpid2Params(&ctx->epid2_params);
    BREAK_ON_EPID_ERROR(sts);

    // create TPM2 context
    sts = Tpm2CreateContext(params, ctx->epid2_params, &ctx->rnd_func,
                            &ctx->rnd_param, &f, &ctx->tpm2_ctx);
    BREAK_ON_EPID_ERROR(sts);

    if (!CreateStack(sizeof(PreComputedSignature), &ctx->presigs)) {
      sts = kEpidMemAllocErr;
      BREAK_ON_EPID_ERROR(sts);
    }

    ctx->f = f;
    ctx->join_ctr = 0;
    ctx->rf_ctr = 0;
    ctx->rnu_ctr = 0;

    sts = NewEcPoint(ctx->epid2_params->G1, (EcPoint**)&ctx->A);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewFfElement(ctx->epid2_params->Fp, (FfElement**)&ctx->x);
    BREAK_ON_EPID_ERROR(sts);

    sts = NewEcPoint(ctx->epid2_params->G1, (EcPoint**)&ctx->h1);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewEcPoint(ctx->epid2_params->G1, (EcPoint**)&ctx->h2);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewEcPoint(ctx->epid2_params->G2, (EcPoint**)&ctx->w);
    BREAK_ON_EPID_ERROR(sts);

    sts = NewFfElement(ctx->epid2_params->GT, (FfElement**)&ctx->e12);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewFfElement(ctx->epid2_params->GT, (FfElement**)&ctx->e22);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewFfElement(ctx->epid2_params->GT, (FfElement**)&ctx->e2w);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewFfElement(ctx->epid2_params->GT, (FfElement**)&ctx->ea2);
    BREAK_ON_EPID_ERROR(sts);

    sts = Tpm2SetHashAlg(ctx->tpm2_ctx, ctx->hash_alg);
    BREAK_ON_EPID_ERROR(sts);
    ctx->primary_key_set = true;
    sts = kEpidNoErr;
  } while (0);
  if (kEpidNoErr != sts) {
    EpidMemberDeinit(ctx);
  }

  return (sts);
}

void EpidMemberDeinit(MemberCtx* ctx) {
  size_t i = 0;
  size_t presig_size = 0;
  PreComputedSignature* buf = NULL;
  if (!ctx) {
    return;
  }
  presig_size = StackGetSize(ctx->presigs);
  buf = StackGetBuf(ctx->presigs);
  for (i = 0; i < presig_size; ++i) {
    (void)Tpm2ReleaseCounter(ctx->tpm2_ctx, (buf++)->rf_ctr);
  }
  (void)Tpm2ReleaseCounter(ctx->tpm2_ctx, ctx->join_ctr);
  (void)Tpm2ReleaseCounter(ctx->tpm2_ctx, ctx->rf_ctr);
  (void)Tpm2ReleaseCounter(ctx->tpm2_ctx, ctx->rnu_ctr);
  DeleteStack(&ctx->presigs);
  ctx->rnd_param = NULL;
  DeleteEcPoint((EcPoint**)&(ctx->h1));
  DeleteEcPoint((EcPoint**)&(ctx->h2));
  DeleteEcPoint((EcPoint**)&(ctx->A));
  DeleteFfElement((FfElement**)&ctx->x);
  DeleteEcPoint((EcPoint**)&(ctx->w));
  DeleteFfElement((FfElement**)&ctx->e12);
  DeleteFfElement((FfElement**)&ctx->e22);
  DeleteFfElement((FfElement**)&ctx->e2w);
  DeleteFfElement((FfElement**)&ctx->ea2);
  Tpm2DeleteContext(&ctx->tpm2_ctx);
  DeleteEpid2Params(&ctx->epid2_params);
  DeleteBasenames(&ctx->allowed_basenames);
}

EpidStatus EpidMemberCreate(MemberParams const* params, MemberCtx** ctx) {
  size_t context_size = 0;
  EpidStatus sts = kEpidErr;
  MemberCtx* member_ctx = NULL;
  if (!params || !ctx) {
    return kEpidBadArgErr;
  }
  do {
    sts = EpidMemberGetSize(params, &context_size);
    BREAK_ON_EPID_ERROR(sts);
    member_ctx = SAFE_ALLOC(context_size);
    if (!member_ctx) {
      BREAK_ON_EPID_ERROR(kEpidMemAllocErr);
    }
    sts = EpidMemberInit(params, member_ctx);
    BREAK_ON_EPID_ERROR(sts);
  } while (0);
  if (kEpidNoErr != sts) {
    SAFE_FREE(member_ctx);
    member_ctx = NULL;
  }
  *ctx = member_ctx;
  return sts;
}

EpidStatus EpidMemberInitialProvision(MemberCtx* ctx) {
  EpidStatus sts = kEpidErr;

  if (!ctx) {
    return kEpidBadArgErr;
  }
  if (ctx->is_initially_provisioned) {
    return kEpidOutOfSequenceError;
  }
  do {
    if (ctx->f) {
      sts = Tpm2LoadExternal(ctx->tpm2_ctx, ctx->f);
      BREAK_ON_EPID_ERROR(sts);
    } else {
      G1ElemStr f;
      sts = Tpm2CreatePrimary(ctx->tpm2_ctx, &f);
      BREAK_ON_EPID_ERROR(sts);
    }

    ctx->is_initially_provisioned = true;
    // f value was set into TPM
    ctx->primary_key_set = true;
    sts = kEpidNoErr;
  } while (0);

  return (sts);
}

void EpidMemberDelete(MemberCtx** ctx) {
  if (!ctx) {
    return;
  }
  EpidMemberDeinit(*ctx);
  SAFE_FREE(*ctx);
  *ctx = NULL;
}

EpidStatus EpidMemberSetHashAlg(MemberCtx* ctx, HashAlg hash_alg) {
  EpidStatus sts = kEpidErr;
  if (!ctx) return kEpidBadArgErr;
  if (kSha256 != hash_alg && kSha384 != hash_alg && kSha512 != hash_alg &&
      kSha512_256 != hash_alg)
    return kEpidBadArgErr;
  do {
    sts = Tpm2SetHashAlg(ctx->tpm2_ctx, hash_alg);
    BREAK_ON_EPID_ERROR(sts);
    ctx->hash_alg = hash_alg;
  } while (0);
  return sts;
}

EpidStatus EpidMemberSetSigRl(MemberCtx* ctx, SigRl const* sig_rl,
                              size_t sig_rl_size) {
  if (!ctx || !sig_rl) {
    return kEpidBadArgErr;
  }
  if (!ctx->is_provisioned) {
    return kEpidOutOfSequenceError;
  }
  if (!IsSigRlValid(&ctx->pub_key.gid, sig_rl, sig_rl_size)) {
    return kEpidBadArgErr;
  }
  // Do not set an older version of sig rl
  if (ctx->sig_rl) {
    unsigned int current_ver = 0;
    unsigned int incoming_ver = 0;
    current_ver = ntohl(ctx->sig_rl->version);
    incoming_ver = ntohl(sig_rl->version);
    if (current_ver >= incoming_ver) {
      return kEpidBadArgErr;
    }
  }
  ctx->sig_rl = sig_rl;

  return kEpidNoErr;
}

EpidStatus EpidRegisterBasename(MemberCtx* ctx, void const* basename,
                                size_t basename_len) {
  EpidStatus sts = kEpidErr;
  if (basename_len == 0) {
    return kEpidBadArgErr;
  }
  if (!ctx || !basename) {
    return kEpidBadArgErr;
  }

  if (IsBasenameAllowed(ctx->allowed_basenames, basename, basename_len)) {
    return kEpidDuplicateErr;
  }

  sts = AllowBasename(ctx->allowed_basenames, basename, basename_len);

  return sts;
}

EpidStatus EpidClearRegisteredBasenames(MemberCtx* ctx) {
  EpidStatus sts = kEpidErr;
  if (!ctx) {
    return kEpidBadArgErr;
  }
  DeleteBasenames(&ctx->allowed_basenames);
  sts = CreateBasenames(&ctx->allowed_basenames);
  return sts;
}
