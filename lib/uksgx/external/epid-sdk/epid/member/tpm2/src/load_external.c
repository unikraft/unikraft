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

/*!
 * \file
 * \brief TPM2_LoadExternal command implementation.
 */

#include "epid/member/tpm2/load_external.h"

#include "epid/common/math/finitefield.h"
#include "epid/common/src/epid2params.h"
#include "epid/member/tpm2/src/state.h"

/// Handle Intel(R) EPID Error with Break
#define BREAK_ON_EPID_ERROR(ret) \
  if (kEpidNoErr != (ret)) {     \
    break;                       \
  }

EpidStatus Tpm2LoadExternal(Tpm2Ctx* ctx, FpElemStr const* f_str) {
  EpidStatus sts = kEpidErr;
  if (!ctx || !ctx->epid2_params || !f_str) {
    return kEpidBadArgErr;
  }

  do {
    FiniteField* Fp = ctx->epid2_params->Fp;

    if (ctx->f) {
      DeleteFfElement(&ctx->f);
    }

    sts = NewFfElement(Fp, &ctx->f);
    BREAK_ON_EPID_ERROR(sts);
    sts = ReadFfElement(Fp, f_str, sizeof(*f_str), ctx->f);
    BREAK_ON_EPID_ERROR(sts);
  } while (0);

  return sts;
}
