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
 * \brief TSS NV API implementation.
 */

#include "epid/member/tpm2/nv.h"
#include <tss2/tss.h>
#include "epid/common/src/memory.h"
#include "epid/member/tpm2/ibm_tss/printtss.h"
#include "epid/member/tpm2/ibm_tss/state.h"

EpidStatus Tpm2NvUndefineSpace(Tpm2Ctx* ctx, uint32_t nv_index) {
  TPM_RC rc = 0;
  NV_UndefineSpace_In in;
  TPMI_SH_AUTH_SESSION sessionHandle0 = TPM_RS_PW;
  if (!ctx) {
    return kEpidBadArgErr;
  }
  if ((nv_index >> 24) != TPM_HT_NV_INDEX) {
    return kEpidBadArgErr;
  }

  in.authHandle = TPM_RH_OWNER;
  // the NV Index to remove from NV space
  in.nvIndex = nv_index;
  rc = TSS_Execute(ctx->tss, NULL, (COMMAND_PARAMETERS*)&in, NULL,
                   TPM_CC_NV_UndefineSpace, sessionHandle0, NULL, 0,
                   TPM_RH_NULL, NULL, 0);
  if (rc != TPM_RC_SUCCESS) {
    print_tpm2_response_code("TPM2_NV_UndefineSpace", rc);
    return kEpidBadArgErr;
  }
  return kEpidNoErr;
}

EpidStatus Tpm2NvDefineSpace(Tpm2Ctx* ctx, uint32_t nv_index, size_t size) {
  TPM_RC rc = 0;
  NV_DefineSpace_In in = {0};
  TPMI_SH_AUTH_SESSION sessionHandle0 = TPM_RS_PW;
  if (!ctx || size == 0 || size > MAX_NV_BUFFER_SIZE) {
    return kEpidBadArgErr;
  }
  if ((nv_index >> 24) != TPM_HT_NV_INDEX) {
    return kEpidBadArgErr;
  }

  in.authHandle = TPM_RH_OWNER;
  // the handle of the data area
  in.publicInfo.nvPublic.nvIndex = nv_index;
  // hash algorithm used to compute the name of the Index and used for the
  // authPolicy
  in.publicInfo.nvPublic.nameAlg = TPM_ALG_SHA256;
  in.publicInfo.nvPublic.attributes.val = TPMA_NVA_NO_DA | TPMA_NVA_AUTHWRITE |
                                          TPMA_NVA_AUTHREAD | TPMA_NVA_ORDINARY;
  // the size of the data area
  in.publicInfo.nvPublic.dataSize = (uint16_t)size;
  rc = TSS_Execute(ctx->tss, NULL, (COMMAND_PARAMETERS*)&in, NULL,
                   TPM_CC_NV_DefineSpace, sessionHandle0, NULL, 0, TPM_RH_NULL,
                   NULL, 0);
  if (rc != TPM_RC_SUCCESS) {
    print_tpm2_response_code("TPM2_NV_DefineSpace", rc);
    if (rc == TPM_RC_NV_DEFINED) {
      return kEpidDuplicateErr;
    }
    return kEpidBadArgErr;
  }
  return kEpidNoErr;
}

EpidStatus Tpm2NvRead(Tpm2Ctx* ctx, uint32_t nv_index, size_t size,
                      uint16_t offset, void* data) {
  TPM_RC rc = 0;
  NV_Read_In in;
  NV_Read_Out out;
  TPMI_SH_AUTH_SESSION sessionHandle0 = TPM_RS_PW;
  int done = FALSE;
  // bytes read so far
  uint16_t bytes_read_so_far = 0;

  if (!ctx || !data || size == 0 || size > MAX_NV_BUFFER_SIZE) {
    return kEpidBadArgErr;
  }
  if ((nv_index >> 24) != TPM_HT_NV_INDEX) {
    return kEpidBadArgErr;
  }

  if ((nv_index >> 24) != TPM_HT_NV_INDEX) {
    return kEpidBadArgErr;
  }
  // Authorization handle
  in.authHandle = nv_index;
  in.nvIndex = nv_index;
  in.offset = offset;
  while ((rc == TPM_RC_SUCCESS) && !done) {
    in.offset = offset + bytes_read_so_far;
    if ((uint32_t)(size - bytes_read_so_far) < MAX_NV_BUFFER_SIZE) {
      // last chunk
      in.size = (uint16_t)size - bytes_read_so_far;
    } else {
      // next chunk
      in.size = MAX_NV_BUFFER_SIZE;
    }
    rc = TSS_Execute(ctx->tss, (RESPONSE_PARAMETERS*)&out,
                     (COMMAND_PARAMETERS*)&in, NULL, TPM_CC_NV_Read,
                     sessionHandle0, NULL, 0, TPM_RH_NULL, NULL, 0);
    if (rc == TPM_RC_SUCCESS) {
      // copy the results to the read buffer
      memcpy_S((uint8_t*)data + bytes_read_so_far, size - bytes_read_so_far,
               out.data.b.buffer, out.data.b.size);
      bytes_read_so_far += out.data.b.size;
      if (bytes_read_so_far == size) {
        done = TRUE;
      }
    } else {
      print_tpm2_response_code("TPM2_NV_Read", rc);
    }
  }
  if (rc != TPM_RC_SUCCESS) {
    return kEpidBadArgErr;
  }
  return kEpidNoErr;
}

EpidStatus Tpm2NvWrite(Tpm2Ctx* ctx, uint32_t nv_index, size_t size,
                       uint16_t offset, void const* data) {
  TPM_RC rc = TPM_RC_SUCCESS;
  NV_Write_In in = {0};
  TPMI_SH_AUTH_SESSION sessionHandle0 = TPM_RS_PW;
  if (!ctx || !data || size == 0 || size > MAX_NV_BUFFER_SIZE) {
    return kEpidBadArgErr;
  }
  if ((nv_index >> 24) != TPM_HT_NV_INDEX) {
    return kEpidBadArgErr;
  }

  in.authHandle = nv_index;
  in.data.b.size = (uint16_t)size;
  memcpy(in.data.b.buffer, data, size);
  in.nvIndex = nv_index;
  // beginning offset
  in.offset = offset;
  rc = TSS_Execute(ctx->tss, NULL, (COMMAND_PARAMETERS*)&in, NULL,
                   TPM_CC_NV_Write, sessionHandle0, NULL, 0, TPM_RH_NULL, NULL,
                   0);
  if (rc != TPM_RC_SUCCESS) {
    print_tpm2_response_code("TPM2_NV_Write", rc);
    return kEpidBadArgErr;
  }
  return kEpidNoErr;
}
