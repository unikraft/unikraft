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
/// Tiny member CreateJoinRequest implementation.
/*! \file */

#define EXPORT_EPID_APIS
#include <epid/member/api.h>

#include "epid/common/types.h"
#include "epid/member/tiny/math/efq.h"
#include "epid/member/tiny/math/fp.h"
#include "epid/member/tiny/math/hashwrap.h"
#include "epid/member/tiny/math/mathtypes.h"
#include "epid/member/tiny/math/serialize.h"
#include "epid/member/tiny/src/context.h"
#include "epid/member/tiny/src/native_types.h"
#include "epid/member/tiny/src/serialize.h"
#include "epid/member/tiny/src/validate.h"
#include "epid/member/tiny/stdlib/tiny_stdlib.h"

static const FpElemStr epid20_p_str = {
    {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xF0, 0xCD, 0x46, 0xE5, 0xF2,
      0x5E, 0xEE, 0x71, 0xA4, 0x9E, 0x0C, 0xDC, 0x65, 0xFB, 0x12, 0x99,
      0x92, 0x1A, 0xF6, 0x2D, 0x53, 0x6C, 0xD1, 0x0B, 0x50, 0x0D}}};

static const G2ElemStr epid20_g2_str = {
    {{{{0xE2, 0x01, 0x71, 0xC5, 0x4A, 0xA3, 0xDA, 0x05, 0x21, 0x67, 0x04,
        0x13, 0x74, 0x3C, 0xCF, 0x22, 0xD2, 0x5D, 0x52, 0x68, 0x3D, 0x32,
        0x47, 0x0E, 0xF6, 0x02, 0x13, 0x43, 0xBF, 0x28, 0x23, 0x94}}},
     {{{0x59, 0x2D, 0x1E, 0xF6, 0x53, 0xA8, 0x5A, 0x80, 0x46, 0xCC, 0xDC,
        0x25, 0x4F, 0xBB, 0x56, 0x56, 0x43, 0x43, 0x3B, 0xF6, 0x28, 0x96,
        0x53, 0xE2, 0x7D, 0xF7, 0xB2, 0x12, 0xBA, 0xA1, 0x89, 0xBE}}}},
    {{{{0xAE, 0x60, 0xA4, 0xE7, 0x51, 0xFF, 0xD3, 0x50, 0xC6, 0x21, 0xE7,
        0x03, 0x31, 0x28, 0x26, 0xBD, 0x55, 0xE8, 0xB5, 0x9A, 0x4D, 0x91,
        0x68, 0x38, 0x41, 0x4D, 0xB8, 0x22, 0xDD, 0x23, 0x35, 0xAE}}},
     {{{0x1A, 0xB4, 0x42, 0xF9, 0x89, 0xAF, 0xE5, 0xAD, 0xF8, 0x02, 0x74,
        0xF8, 0x76, 0x45, 0xE2, 0x53, 0x2C, 0xDC, 0x61, 0x81, 0x90, 0x93,
        0xD6, 0x13, 0x2C, 0x90, 0xFE, 0x89, 0x51, 0xB9, 0x24, 0x21}}}}};

static const G1ElemStr epid20_g1_str = {
    {{{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}}},
    {{{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}}}};

EpidStatus EPID_API EpidCreateJoinRequest(MemberCtx* ctx,
                                          GroupPubKey const* pub_key,
                                          IssuerNonce const* ni,
                                          JoinRequest* join_request) {
  EccPointFq F;
  EccPointFq R_str;
  FpElem r;
  FpElem c;
  FpElem t;
  tiny_sha sha_state;
  sha_digest digest;
  G1ElemStr g1_str;
  NativeGroupPubKey deserialized_pkey;

  if (!ctx || !pub_key || !ni || !join_request) {
    return kEpidBadArgErr;
  }
  if (!ctx->f_is_set) {
    return kEpidBadArgErr;
  }

  GroupPubKeyDeserialize(&deserialized_pkey, pub_key);
  if (!GroupPubKeyIsInRange(&deserialized_pkey)) {
    return kEpidBadArgErr;
  }
  // pick number r between 1 - (p-1)
  if (!FpRandNonzero(&r, ctx->rnd_func, ctx->rnd_param)) {
    return kEpidMathErr;
  }

  // F = h1^f
  EFqAffineExp(&F, &deserialized_pkey.h1, &ctx->f);
  // R_str = h1^r
  EFqAffineExp(&R_str, &deserialized_pkey.h1, &r);

  // c = hash(p || g1 || g2 || h1 || h2 || w || F || R || NI) mod p
  tinysha_init(ctx->hash_alg, &sha_state);
  tinysha_update(&sha_state, (void const*)&epid20_p_str, sizeof(epid20_p_str));
  tinysha_update(&sha_state, (void const*)&epid20_g1_str,
                 sizeof(epid20_g1_str));
  tinysha_update(&sha_state, (void const*)&epid20_g2_str,
                 sizeof(epid20_g2_str));
  tinysha_update(&sha_state, (void const*)&pub_key->h1, sizeof(pub_key->h1));
  tinysha_update(&sha_state, (void const*)&pub_key->h2, sizeof(pub_key->h2));
  tinysha_update(&sha_state, (void const*)&pub_key->w, sizeof(pub_key->w));
  EFqSerialize(&join_request->F, &F);
  tinysha_update(&sha_state, (void const*)&join_request->F,
                 sizeof(join_request->F));
  EFqSerialize(&g1_str, &R_str);
  tinysha_update(&sha_state, (void const*)&g1_str, sizeof(g1_str));
  tinysha_update(&sha_state, (void const*)ni, sizeof(*ni));
  tinysha_final(digest.digest, &sha_state);
  FpFromHash(&c, digest.digest, tinysha_digest_size(&sha_state));
  FpSerialize(&join_request->c, &c);

  // computes s = (r + c * f) mod p
  FpMul(&t, &c, &ctx->f);
  FpAdd(&t, &t, &r);
  FpSerialize(&join_request->s, &t);
  return kEpidNoErr;
}
