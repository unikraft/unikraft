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
/// TPM context implementation.
/*! \file */

#include <stddef.h>

#include "epid/common/math/finitefield.h"
#include "epid/common/src/epid2params.h"
#include "epid/common/src/memory.h"
#include "epid/member/software_member.h"
#include "epid/member/tpm2/context.h"
#include "epid/member/tpm2/src/state.h"

/// Handle Intel(R) EPID Error with Break
#define BREAK_ON_EPID_ERROR(ret) \
  if (kEpidNoErr != (ret)) {     \
    break;                       \
  }

EpidStatus Tpm2CreateContext(MemberParams const* params,
                             Epid2Params_ const* epid2_params,
                             BitSupplier* rnd_func, void** rnd_param,
                             const FpElemStr** f, Tpm2Ctx** ctx) {
  Tpm2Ctx* tpm_ctx = NULL;
  EpidStatus sts = kEpidNoErr;
  FfElement* ff_elem = NULL;

  if (!params || !epid2_params || !rnd_func || !rnd_param || !f || !ctx) {
    return kEpidBadArgErr;
  }

  tpm_ctx = SAFE_ALLOC(sizeof(Tpm2Ctx));
  if (!tpm_ctx) {
    return kEpidMemAllocErr;
  }

  do {
    int i;

    if (params->f) {
      FiniteField* Fp = epid2_params->Fp;
      // Validate f
      sts = NewFfElement(Fp, &ff_elem);
      BREAK_ON_EPID_ERROR(sts);
      sts = ReadFfElement(Fp, params->f, sizeof(*params->f), ff_elem);
      BREAK_ON_EPID_ERROR(sts);
    }
    tpm_ctx->epid2_params = epid2_params;
    tpm_ctx->rnd_func = params->rnd_func;
    tpm_ctx->rnd_param = params->rnd_param;
    tpm_ctx->f = NULL;
    *rnd_func = params->rnd_func;
    *rnd_param = params->rnd_param;
    *f = params->f;

    for (i = 0; i < MAX_NV_NUMBER; ++i) {
      tpm_ctx->nv->nv_index = 0;
      tpm_ctx->nv->data = NULL;
      tpm_ctx->nv->data_size = 0;
    }

    memset(tpm_ctx->commit_data, 0, sizeof(tpm_ctx->commit_data));

    *ctx = tpm_ctx;
    sts = kEpidNoErr;
  } while (0);
  DeleteFfElement(&ff_elem);
  if (kEpidNoErr != sts) {
    Tpm2DeleteContext(&tpm_ctx);
    *ctx = NULL;
  }
  return sts;
}

void Tpm2DeleteContext(Tpm2Ctx** ctx) {
  if (ctx && *ctx) {
    int i;
    (*ctx)->rnd_param = NULL;
    DeleteFfElement(&(*ctx)->f);
    for (i = 0; i < MAX_COMMIT_COUNT; ++i) {
      DeleteFfElement(&(*ctx)->commit_data[i]);
    }
    for (i = 0; i < MAX_NV_NUMBER; ++i) {
      (*ctx)->nv->nv_index = 0;
      SAFE_FREE((*ctx)->nv->data);
      (*ctx)->nv->data_size = 0;
    }
    SAFE_FREE(*ctx);
  }
}

EpidStatus Tpm2SetHashAlg(Tpm2Ctx* ctx, HashAlg hash_alg) {
  if (!ctx) return kEpidBadArgErr;
  if (kSha256 != hash_alg && kSha384 != hash_alg && kSha512 != hash_alg &&
      kSha512_256 != hash_alg)
    return kEpidHashAlgorithmNotSupported;
  ctx->hash_alg = hash_alg;
  return kEpidNoErr;
}

void Tpm2ResetContext(Tpm2Ctx** ctx) {
  if (ctx && *ctx) {
    DeleteFfElement(&(*ctx)->f);
  }
}
