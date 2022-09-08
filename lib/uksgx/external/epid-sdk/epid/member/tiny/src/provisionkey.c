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
/// Tiny member ProvisionKey implementation.
/*! \file */

#define EXPORT_EPID_APIS
#include "epid/member/api.h"
#include "epid/member/software_member.h"
#include "epid/member/tiny/math/efq.h"
#include "epid/member/tiny/math/efq2.h"
#include "epid/member/tiny/math/fp.h"
#include "epid/member/tiny/math/mathtypes.h"
#include "epid/member/tiny/math/pairing.h"
#include "epid/member/tiny/math/serialize.h"
#include "epid/member/tiny/src/context.h"
#include "epid/member/tiny/src/native_types.h"
#include "epid/member/tiny/src/serialize.h"
#include "epid/member/tiny/src/validate.h"
#include "epid/member/tiny/stdlib/tiny_stdlib.h"

static EccPointFq2 const epid20_g2 = {
    {{{{0xBF282394, 0xF6021343, 0x3D32470E, 0xD25D5268, 0x743CCF22, 0x21670413,
        0x4AA3DA05, 0xE20171C5}}},
     {{{0xBAA189BE, 0x7DF7B212, 0x289653E2, 0x43433BF6, 0x4FBB5656, 0x46CCDC25,
        0x53A85A80, 0x592D1EF6}}}},
    {{{{0xDD2335AE, 0x414DB822, 0x4D916838, 0x55E8B59A, 0x312826BD, 0xC621E703,
        0x51FFD350, 0xAE60A4E7}}},
     {{{0x51B92421, 0x2C90FE89, 0x9093D613, 0x2CDC6181, 0x7645E253, 0xF80274F8,
        0x89AFE5AD, 0x1AB442F9}}}}};

EpidStatus EPID_API EpidProvisionKey(MemberCtx* ctx, GroupPubKey const* pub_key,
                                     PrivKey const* priv_key,
                                     MemberPrecomp const* precomp_str) {
  NativeGroupPubKey native_pub_key;
  NativePrivKey native_priv_key;
  if (!pub_key || !priv_key || !ctx) {
    return kEpidBadArgErr;
  }
  if (0 != memcmp(&pub_key->gid, &priv_key->gid, sizeof(GroupId))) {
    return kEpidBadArgErr;
  }
  GroupPubKeyDeserialize(&native_pub_key, pub_key);
  if (!GroupPubKeyIsInRange(&native_pub_key)) {
    return kEpidBadArgErr;
  }

  PrivKeyDeserialize(&native_priv_key, priv_key);
  if (!PrivKeyIsInRange(&native_priv_key) ||
      !PrivKeyIsInGroup(&native_priv_key, &native_pub_key,
                        &ctx->pairing_state)) {
    memset(&native_priv_key.f, 0, sizeof(native_priv_key.f));
    return kEpidBadArgErr;
  }
  ctx->f = native_priv_key.f;
  memset(&native_priv_key.f, 0, sizeof(native_priv_key.f));
  ctx->f_is_set = 1;

  ctx->credential.gid = priv_key->gid;
  ctx->credential.A = priv_key->A;
  ctx->credential.x = priv_key->x;
  ctx->pub_key = *pub_key;
  ctx->is_provisioned = 1;

  if (precomp_str) {
    PreCompDeserialize(&ctx->precomp, precomp_str);
  } else {
    PairingCompute(&ctx->precomp.ea2, &native_priv_key.cred.A, &epid20_g2,
                   &ctx->pairing_state);
    PairingCompute(&ctx->precomp.e12, &native_pub_key.h1, &epid20_g2,
                   &ctx->pairing_state);
    PairingCompute(&ctx->precomp.e22, &native_pub_key.h2, &epid20_g2,
                   &ctx->pairing_state);
    PairingCompute(&ctx->precomp.e2w, &native_pub_key.h2, &native_pub_key.w,
                   &ctx->pairing_state);
  }
  return kEpidNoErr;
}
