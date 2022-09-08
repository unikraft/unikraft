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
 * \brief TPM2_CreatePrimary command implementation.
 * \file
 */
#include "epid/member/tpm2/createprimary.h"
#include "epid/common/math/finitefield.h"
#include "epid/common/src/epid2params.h"
#include "epid/member/software_member.h"
#include "epid/member/tpm2/load_external.h"
#include "epid/member/tpm2/src/state.h"

/// Handle Intel(R) EPID Error with Break
#define BREAK_ON_EPID_ERROR(ret) \
  if (kEpidNoErr != (ret)) {     \
    break;                       \
  }

EpidStatus Tpm2CreatePrimary(Tpm2Ctx* ctx, G1ElemStr* p_str) {
  EpidStatus sts = kEpidErr;
  FfElement* ff_elem;
  FpElemStr ff_elem_str;
  if (!ctx || !ctx->epid2_params) {
    return kEpidBadArgErr;
  }
  (void)p_str;
  do {
    const BigNumStr kOne = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    FiniteField* Fp = ctx->epid2_params->Fp;
    sts = NewFfElement(Fp, &ff_elem);
    BREAK_ON_EPID_ERROR(sts);
    sts = FfGetRandom(Fp, &kOne, ctx->rnd_func, ctx->rnd_param, ff_elem);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteFfElement(Fp, ff_elem, &ff_elem_str, sizeof(ff_elem_str));
    BREAK_ON_EPID_ERROR(sts);
  } while (0);
  DeleteFfElement(&ff_elem);
  if (kEpidNoErr == sts) {
    sts = Tpm2LoadExternal(ctx, &ff_elem_str);
  }
  return sts;
}
