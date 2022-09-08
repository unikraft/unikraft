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

#include "epid/member/tpm2/context.h"

#include <tss2/TPM_Types.h>
#include <tss2/tss.h>

#include "epid/common/math/finitefield.h"
#include "epid/common/src/epid2params.h"
#include "epid/common/src/memory.h"
#include "epid/member/tpm2/getrandom.h"
#include "epid/member/tpm2/ibm_tss/printtss.h"
#include "epid/member/tpm2/ibm_tss/state.h"
#include "epid/member/tpm_member.h"

/// Handle Intel(R) EPID Error with Break
#define BREAK_ON_EPID_ERROR(ret) \
  if (kEpidNoErr != (ret)) {     \
    break;                       \
  }

/// Deletes key from TPM
/*!
\param[in,out] ctx
TPM context.

\returns ::EpidStatus
*/
void Tpm2FlushKey(Tpm2Ctx* ctx);

/// Flag that indicates that context was already created
bool is_context_already_created = false;

/// Internal Random function as a BitSupplier
static int __STDCALL tpm2_rnd_func(unsigned int* rand_data, int num_bits,
                                   void* user_data) {
  return Tpm2GetRandom((Tpm2Ctx*)user_data, num_bits, rand_data);
}

EpidStatus Tpm2CreateContext(MemberParams const* params,
                             Epid2Params_ const* epid2_params,
                             BitSupplier* rnd_func, void** rnd_param,
                             const FpElemStr** f, Tpm2Ctx** ctx) {
  EpidStatus sts = kEpidNoErr;
  TPM_RC rc = TPM_RC_FAILURE;
  Tpm2Ctx* tpm_ctx = NULL;
  FfElement* ff_elem = NULL;
  if (!params || !epid2_params || !rnd_func || !rnd_param || !f || !ctx) {
    return kEpidBadArgErr;
  }

  if (is_context_already_created) {
    return kEpidBadArgErr;
  }
  is_context_already_created = true;

  tpm_ctx = SAFE_ALLOC(sizeof(Tpm2Ctx));
  if (!tpm_ctx) {
    return kEpidMemAllocErr;
  }

  do {
    if (params->f) {
      FiniteField* Fp = epid2_params->Fp;
      // Validate f
      sts = NewFfElement(Fp, &ff_elem);
      BREAK_ON_EPID_ERROR(sts);
      sts = ReadFfElement(Fp, params->f, sizeof(*params->f), ff_elem);
      BREAK_ON_EPID_ERROR(sts);
    }

    tpm_ctx->epid2_params = epid2_params;
    tpm_ctx->key_handle = 0;
    tpm_ctx->hash_alg = kInvalidHashAlg;

    rc = TSS_Create(&tpm_ctx->tss);
    if (rc != TPM_RC_SUCCESS) {
      sts = kEpidErr;
      break;
    }

    *ctx = tpm_ctx;
    *rnd_func = tpm2_rnd_func;
    *rnd_param = *ctx;
    *f = params->f;
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
  is_context_already_created = false;
  if (ctx && *ctx) {
    Tpm2FlushKey(*ctx);
    TSS_Delete((*ctx)->tss);
    SAFE_FREE(*ctx);
  }
}

EpidStatus Tpm2SetHashAlg(Tpm2Ctx* ctx, HashAlg hash_alg) {
  if (!ctx) return kEpidBadArgErr;
  if (kSha256 != hash_alg && kSha384 != hash_alg && kSha512 != hash_alg &&
      kSha512_256 != hash_alg)
    return kEpidHashAlgorithmNotSupported;
  // can not change hash alg of existing TPM2 key object
  if (ctx->key_handle) return kEpidOutOfSequenceError;
  ctx->hash_alg = hash_alg;
  return kEpidNoErr;
}

void Tpm2ResetContext(Tpm2Ctx** ctx) {
  if (ctx && *ctx) {
    Tpm2FlushKey(*ctx);
  }
}

void Tpm2FlushKey(Tpm2Ctx* ctx) {
  if (ctx->key_handle) {
    TPM_RC rc;
    FlushContext_In in;
    in.flushHandle = ctx->key_handle;
    rc = TSS_Execute(ctx->tss, NULL, (COMMAND_PARAMETERS*)&in, NULL,
                     TPM_CC_FlushContext, TPM_RH_NULL, NULL, 0);
    if (rc != TPM_RC_SUCCESS) {
      print_tpm2_response_code("TPM2_FlushContext", rc);
    }
    ctx->key_handle = 0;
  }
}
