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
 * \brief Member credentials storage helper API implementation.
 */

#include "epid/member/src/storage.h"

#include "epid/common/src/memory.h"
#include "epid/common/types.h"
#include "epid/member/tpm2/nv.h"

/// Handle Intel(R) EPID Error with Break
#define BREAK_ON_EPID_ERROR(ret) \
  if (kEpidNoErr != (ret)) {     \
    break;                       \
  }

EpidStatus EpidNvWriteMembershipCredential(
    Tpm2Ctx* ctx, GroupPubKey const* pub_key,
    MembershipCredential const* credential, uint32_t nv_index) {
  EpidStatus sts = kEpidErr;
  uint8_t tmp;
  if (!ctx || !pub_key || !credential) return kEpidBadArgErr;

  do {
    if (kEpidNoErr != Tpm2NvRead(ctx, nv_index, 1, 0, &tmp)) {
      sts = Tpm2NvDefineSpace(ctx, nv_index,
                              sizeof(*pub_key) + sizeof(*credential));
      BREAK_ON_EPID_ERROR(sts);
    }
    sts = Tpm2NvWrite(ctx, nv_index, sizeof(*pub_key), 0, pub_key);
    BREAK_ON_EPID_ERROR(sts);
    sts = Tpm2NvWrite(ctx, nv_index, sizeof(*credential), sizeof(*pub_key),
                      credential);
    BREAK_ON_EPID_ERROR(sts);
  } while (0);
  if (kEpidNoErr != sts) Tpm2NvUndefineSpace(ctx, nv_index);
  EpidZeroMemory(&tmp, sizeof(tmp));
  return sts;
}

EpidStatus EpidNvReadMembershipCredential(Tpm2Ctx* ctx, uint32_t nv_index,
                                          GroupPubKey* pub_key,
                                          MembershipCredential* credential) {
  EpidStatus sts = kEpidErr;
  if (!ctx || !pub_key || !credential) return kEpidBadArgErr;
  do {
    sts = Tpm2NvRead(ctx, nv_index, sizeof(*pub_key), 0, pub_key);
    BREAK_ON_EPID_ERROR(sts);
    sts = Tpm2NvRead(ctx, nv_index, sizeof(*credential), sizeof(*pub_key),
                     credential);
    BREAK_ON_EPID_ERROR(sts);
  } while (0);
  return sts;
}
