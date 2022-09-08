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
/// TPM-SDK data conversion implementation.
/*! \file */

#include "epid/member/tpm2/ibm_tss/conversion.h"
#include <string.h>
#include <tss2/TPM_Types.h>
#include "epid/common/math/ecgroup.h"
#include "epid/common/src/memory.h"
#include "epid/common/types.h"

TPMI_ALG_HASH EpidtoTpm2HashAlg(HashAlg hash_alg) {
  switch (hash_alg) {
    case kSha256:
      return TPM_ALG_SHA256;
    case kSha384:
      return TPM_ALG_SHA384;
    case kSha512:
      return TPM_ALG_SHA512;
    default:
      return TPM_ALG_NULL;
  }
}

HashAlg Tpm2toEpidHashAlg(TPMI_ALG_HASH tpm_hash_alg) {
  switch (tpm_hash_alg) {
    case TPM_ALG_SHA256:
      return kSha256;
    case TPM_ALG_SHA384:
      return kSha384;
    case TPM_ALG_SHA512:
      return kSha512;
    default:
      return kInvalidHashAlg;
  }
}

EpidStatus ReadTpm2FfElement(OctStr256 const* str,
                             TPM2B_ECC_PARAMETER* tpm_data) {
  if (!str || !tpm_data) {
    return kEpidBadArgErr;
  }
  if (0 !=
      memcpy_S(tpm_data->b.buffer, MAX_ECC_KEY_BYTES, str, sizeof(OctStr256))) {
    return kEpidBadArgErr;
  }
  tpm_data->b.size = (UINT16)sizeof(OctStr256);
  return kEpidNoErr;
}

EpidStatus WriteTpm2FfElement(TPM2B_ECC_PARAMETER const* tpm_data,
                              OctStr256* str) {
  if (!tpm_data || !str || tpm_data->b.size > (UINT16)sizeof(OctStr256)) {
    return kEpidBadArgErr;
  }
  uint8_t* buf = (uint8_t*)str;
  size_t real_size = sizeof(OctStr256);
  if (tpm_data->b.size < real_size) {
    memset(buf, 0x00, real_size - tpm_data->b.size);
    buf += real_size - tpm_data->b.size;
    real_size = tpm_data->b.size;
  }
  if (0 != memcpy_S(buf, real_size, tpm_data->b.buffer, tpm_data->b.size)) {
    return kEpidBadArgErr;
  }
  return kEpidNoErr;
}

EpidStatus ReadTpm2EcPoint(G1ElemStr const* p_str, TPM2B_ECC_POINT* tpm_point) {
  if (!p_str || !tpm_point) {
    return kEpidBadArgErr;
  }

  //  copy X
  if (0 != memcpy_S(tpm_point->point.x.t.buffer, MAX_ECC_KEY_BYTES, &p_str->x,
                    sizeof(G1ElemStr) / 2)) {
    return kEpidErr;
  }
  tpm_point->point.x.t.size = sizeof(G1ElemStr) / 2;

  //  copy Y
  if (0 != memcpy_S(tpm_point->point.y.t.buffer, MAX_ECC_KEY_BYTES, &p_str->y,
                    sizeof(G1ElemStr) / 2)) {
    return kEpidErr;
  }
  tpm_point->point.y.t.size = sizeof(G1ElemStr) / 2;

  tpm_point->size = sizeof(tpm_point->point);
  return kEpidNoErr;
}

EpidStatus WriteTpm2EcPoint(TPM2B_ECC_POINT const* tpm_point,
                            G1ElemStr* p_str) {
  if (!p_str || !tpm_point) {
    return kEpidBadArgErr;
  }

  if (tpm_point->point.x.t.size > sizeof(G1ElemStr) / 2 ||
      tpm_point->point.y.t.size > sizeof(G1ElemStr) / 2) {
    return kEpidBadArgErr;
  }

  memset(p_str, '\0', sizeof(G1ElemStr));

  //  copy X
  if (0 !=
      memcpy_S(&p_str->x + (sizeof(G1ElemStr) / 2 - tpm_point->point.x.t.size),
               tpm_point->point.x.t.size, tpm_point->point.x.t.buffer,
               tpm_point->point.x.t.size)) {
    return kEpidErr;
  }
  //  copy Y
  if (0 !=
      memcpy_S(&p_str->y + (sizeof(G1ElemStr) / 2 - tpm_point->point.y.t.size),
               tpm_point->point.y.t.size, tpm_point->point.y.t.buffer,
               tpm_point->point.y.t.size)) {
    return kEpidErr;
  }
  return kEpidNoErr;
}
