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
 * \brief TPM2_GetRandom command implementation.
 */

#include "epid/member/tpm2/getrandom.h"
#include "epid/member/tpm2/context.h"
#include "epid/member/tpm2/src/state.h"

EpidStatus Tpm2GetRandom(Tpm2Ctx* ctx, int const num_bits, void* random_data) {
  int rand_ret = -1;

  if (!ctx || !random_data) {
    return kEpidBadArgErr;
  }

  if (num_bits <= 0) {
    return kEpidBadArgErr;
  }

  rand_ret = ctx->rnd_func(random_data, num_bits, ctx->rnd_param);
  if (rand_ret != 0) {
    return kEpidErr;
  }

  return kEpidNoErr;
}
