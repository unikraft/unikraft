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
/// EpidProvisionCredential implementation.
/*!
 * \file
 */

#include <epid/member/api.h>

#include <string.h>
#include "epid/common/src/memory.h"
#include "epid/common/types.h"
#include "epid/member/src/context.h"
#include "epid/member/src/storage.h"
#include "epid/member/src/validatekey.h"
#include "epid/member/tpm2/context.h"
#include "epid/member/tpm2/createprimary.h"

EpidStatus EpidProvisionCredential(MemberCtx* ctx, GroupPubKey const* pub_key,
                                   MembershipCredential const* credential,
                                   MemberPrecomp const* precomp_str) {
  EpidStatus sts = kEpidErr;
  uint32_t const nv_index = 0x01c10100;
  G1ElemStr f_str;

  if (!pub_key || !credential || !ctx) {
    return kEpidBadArgErr;
  }

  if (memcmp(&pub_key->gid, &credential->gid, sizeof(GroupId))) {
    return kEpidBadArgErr;
  }

  if (!ctx->is_provisioned && !ctx->is_initially_provisioned) {
    sts = EpidMemberInitialProvision(ctx);
    if (kEpidNoErr != sts) return sts;
  }

  if (!EpidMemberIsKeyValid(ctx, &credential->A, &credential->x, &pub_key->h1,
                            &pub_key->w)) {
    return kEpidBadArgErr;
  }

  sts = EpidNvWriteMembershipCredential(ctx->tpm2_ctx, pub_key, credential,
                                        nv_index);

  if (ctx->primary_key_set) {
    Tpm2ResetContext(&ctx->tpm2_ctx);
    ctx->primary_key_set = false;
  }
  sts = Tpm2CreatePrimary(ctx->tpm2_ctx, &f_str);
  if (kEpidNoErr != sts) {
    return sts;
  }
  ctx->primary_key_set = true;
  if (kEpidNoErr == sts) {
    if (precomp_str) {
      ctx->precomp = *precomp_str;
      ctx->precomp_ready = true;
    } else {
      EpidZeroMemory(&ctx->precomp, sizeof(ctx->precomp));
      ctx->precomp_ready = false;
    }

    ctx->credential.A = credential->A;
    ctx->credential.x = credential->x;
    ctx->credential.gid = credential->gid;
    ctx->pub_key = *pub_key;
    ctx->is_provisioned = true;
  }
  return sts;
}
