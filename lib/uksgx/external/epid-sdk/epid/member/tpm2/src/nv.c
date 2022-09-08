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
 * \brief TPM NV API implementation.
 */

#include "epid/member/tpm2/nv.h"

#include "epid/common/src/memory.h"
#include "epid/member/tpm2/src/state.h"

/// Find nv_index in nv array
int EpidFindNvIndex(Tpm2Ctx* ctx, uint32_t nv_index) {
  int i;
  for (i = 0; i < MAX_NV_NUMBER; ++i) {
    if (ctx->nv[i].nv_index == nv_index) return i;
  }
  return -1;
}

/// Find empty node in nv array
int EpidFindFirstEmptyNvIndex(Tpm2Ctx* ctx) {
  int i;
  for (i = 0; i < MAX_NV_NUMBER; ++i) {
    if (!ctx->nv[i].data_size) return i;
  }
  return -1;
}

EpidStatus Tpm2NvDefineSpace(Tpm2Ctx* ctx, uint32_t nv_index, size_t size) {
  int index = 0;
  if (!ctx || size <= 0) {
    return kEpidBadArgErr;
  }
  if (nv_index < MIN_NV_INDEX) {
    return kEpidBadArgErr;
  }
  if (EpidFindNvIndex(ctx, nv_index) != -1) {
    return kEpidDuplicateErr;
  }
  index = EpidFindFirstEmptyNvIndex(ctx);
  if (index == -1) {
    return kEpidBadArgErr;
  }
  ctx->nv[index].nv_index = nv_index;
  // memory will be allocated on first NvWrite call
  ctx->nv[index].data_size = size;
  return kEpidNoErr;
}

EpidStatus Tpm2NvUndefineSpace(Tpm2Ctx* ctx, uint32_t nv_index) {
  int index = 0;
  if (!ctx) {
    return kEpidBadArgErr;
  }
  if (nv_index < MIN_NV_INDEX) {
    return kEpidBadArgErr;
  }
  index = EpidFindNvIndex(ctx, nv_index);
  if (index == -1) {
    return kEpidBadArgErr;
  }
  ctx->nv[index].nv_index = 0;
  SAFE_FREE(ctx->nv[index].data);
  ctx->nv[index].data_size = 0;
  return kEpidNoErr;
}

EpidStatus Tpm2NvRead(Tpm2Ctx* ctx, uint32_t nv_index, size_t size,
                      uint16_t offset, void* data) {
  uint8_t* buf = NULL;
  int index = 0;
  if (!ctx || !data || size <= 0) {
    return kEpidBadArgErr;
  }
  if (nv_index < MIN_NV_INDEX) {
    return kEpidBadArgErr;
  }
  index = EpidFindNvIndex(ctx, nv_index);
  if (index == -1 || !ctx->nv[index].data) {
    return kEpidBadArgErr;
  }
  if (offset + size > ctx->nv[index].data_size) {
    return kEpidBadArgErr;
  }
  buf = (uint8_t*)ctx->nv[index].data + offset;
  if (0 != memcpy_S(data, size, buf, size)) {
    return kEpidErr;
  }
  return kEpidNoErr;
}

EpidStatus Tpm2NvWrite(Tpm2Ctx* ctx, uint32_t nv_index, size_t size,
                       uint16_t offset, void const* data) {
  uint8_t* buf = NULL;
  int index = 0;
  if (!ctx || !data || size <= 0) {
    return kEpidBadArgErr;
  }
  if (nv_index < MIN_NV_INDEX) {
    return kEpidBadArgErr;
  }
  index = EpidFindNvIndex(ctx, nv_index);
  if (index == -1) {
    return kEpidBadArgErr;
  }
  if (offset + size > ctx->nv[index].data_size) {
    return kEpidBadArgErr;
  }
  if (!ctx->nv[index].data) {
    ctx->nv[index].data = SAFE_ALLOC(ctx->nv[index].data_size);
    if (!ctx->nv[index].data) return kEpidMemAllocErr;
  }
  buf = (uint8_t*)ctx->nv[index].data + offset;
  if (0 != memcpy_S(buf, size, data, size)) {
    return kEpidErr;
  }
  return kEpidNoErr;
}
