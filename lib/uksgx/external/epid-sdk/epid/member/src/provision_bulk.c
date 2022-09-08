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
/// EpidProvisionKey implementation.
/*!
 * \file
 */

#include <epid/member/api.h>

#include <string.h>
#include "epid/common/src/memory.h"
#include "epid/common/stdtypes.h"
#include "epid/common/types.h"
#include "epid/member/src/context.h"
#include "epid/member/src/storage.h"
#include "epid/member/tpm2/context.h"
#include "epid/member/tpm2/load_external.h"

EpidStatus EpidProvisionKey(MemberCtx* ctx, GroupPubKey const* pub_key,
                            PrivKey const* priv_key,
                            MemberPrecomp const* precomp_str) {
  EpidStatus sts = kEpidErr;
  uint32_t const nv_index = 0x01c10100;
  MembershipCredential credential = {0};

  if (!pub_key || !priv_key || !ctx) {
    return kEpidBadArgErr;
  }

  // The member verifies that gid in public key and in private key
  // match. If mismatch, abort and return operation failed.
  if (memcmp(&pub_key->gid, &priv_key->gid, sizeof(GroupId))) {
    return kEpidBadArgErr;
  }

  sts = Tpm2LoadExternal(ctx->tpm2_ctx, &priv_key->f);
  if (kEpidNoErr != sts) {
    return sts;
  }

  credential.A = priv_key->A;
  credential.x = priv_key->x;
  credential.gid = priv_key->gid;

  if (ctx->primary_key_set) {
    Tpm2ResetContext(&ctx->tpm2_ctx);
    ctx->primary_key_set = false;
  }
  sts = Tpm2LoadExternal(ctx->tpm2_ctx, &priv_key->f);
  if (kEpidNoErr != sts) {
    return sts;
  }
  ctx->primary_key_set = true;

  sts = EpidNvWriteMembershipCredential(ctx->tpm2_ctx, pub_key, &credential,
                                        nv_index);

  if (kEpidNoErr == sts) {
    if (precomp_str) {
      ctx->precomp = *precomp_str;
      ctx->precomp_ready = true;
    } else {
      EpidZeroMemory(&ctx->precomp, sizeof(ctx->precomp));
      ctx->precomp_ready = false;
    }

    ctx->pub_key = *pub_key;
    ctx->is_provisioned = true;

    ctx->credential.A = credential.A;
    ctx->credential.x = credential.x;
    ctx->credential.gid = credential.gid;
  }
  return sts;
}
