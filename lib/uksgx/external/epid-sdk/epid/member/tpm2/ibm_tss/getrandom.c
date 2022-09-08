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
#include <limits.h>

#include "epid/common/src/memory.h"
#include "epid/member/tpm2/getrandom.h"
#include "epid/member/tpm2/ibm_tss/printtss.h"
#include "epid/member/tpm2/ibm_tss/state.h"

#include "tss2/TPM_Types.h"
#include "tss2/tss.h"

EpidStatus Tpm2GetRandom(Tpm2Ctx* ctx, int const num_bits, void* random_data) {
  EpidStatus sts = kEpidNoErr;
  TPM_RC rc = TPM_RC_FAILURE;
  int num_bytes = (num_bits + CHAR_BIT - 1) / CHAR_BIT;
  BYTE* buf = (BYTE*)random_data;

  if (!ctx || !random_data) {
    return kEpidBadArgErr;
  }

  if (num_bits <= 0) {
    return kEpidBadArgErr;
  }

  do {
    GetRandom_In in;
    GetRandom_Out out;
    size_t max_digest_size = sizeof(((TPM2B_DIGEST*)0)->t.buffer);
    UINT16 bytes_to_reqest = ((size_t)num_bytes > max_digest_size)
                                 ? (UINT16)max_digest_size
                                 : (UINT16)num_bytes;
    in.bytesRequested = bytes_to_reqest;

    rc = TSS_Execute(ctx->tss, (RESPONSE_PARAMETERS*)&out,
                     (COMMAND_PARAMETERS*)&in, NULL, TPM_CC_GetRandom,
                     TPM_RH_NULL, NULL, 0);
    if (rc != TPM_RC_SUCCESS) {
      print_tpm2_response_code("TPM2_GetRandom", rc);
      sts = kEpidErr;
      break;
    }
    if (!out.randomBytes.t.size || out.randomBytes.t.size > bytes_to_reqest) {
      sts = kEpidErr;
      break;
    }

    if (0 != memcpy_S(buf, (size_t)num_bytes, out.randomBytes.t.buffer,
                      out.randomBytes.t.size)) {
      sts = kEpidErr;
      break;
    }

    num_bytes -= out.randomBytes.t.size;
    buf += out.randomBytes.t.size;
  } while (num_bytes > 0);

  return sts;
}
