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
/// Basic signature computation.
/*! \file */

#include "epid/member/tiny/src/signbasic.h"

#include "epid/common/types.h"
#include "epid/member/tiny/math/efq.h"
#include "epid/member/tiny/math/fp.h"
#include "epid/member/tiny/math/hashwrap.h"
#include "epid/member/tiny/math/serialize.h"
#include "epid/member/tiny/src/context.h"
#include "epid/member/tiny/src/native_types.h"
#include "epid/member/tiny/src/presig_compute.h"
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

EpidStatus EpidSignBasic(MemberCtx const* ctx, void const* msg, size_t msg_len,
                         void const* basename, size_t basename_len,
                         NativeBasicSignature* sig) {
  EpidStatus sts = kEpidErr;
  PreComputedSignatureData presig;
  tiny_sha sha_state;
  sha_digest digest;
  G1ElemStr g1_str;
  Fq12ElemStr fq12_str;
  FpElemStr fp_str;
  FpElem x;

  FpDeserialize(&x, &ctx->credential.x);
  do {
    sts = EpidMemberComputePreSig(ctx, &presig);
    if (kEpidNoErr != sts) {
      break;
    }
    // B <- random
    if (basename) {
      if (!IsBasenameAllowed(ctx->allowed_basenames, basename, basename_len)) {
        sts = kEpidBadArgErr;
        break;
      }
      /* Basename, K is linked to fixed B */
      if (!EFqHash(&sig->B, (const unsigned char*)basename, basename_len,
                   ctx->hash_alg)) {
        break;
      }
    } else {
      /* No basename, B is random */
      if (!EFqRand(&sig->B, ctx->rnd_func, ctx->rnd_param)) {
        break;
      }
    }
    // K <- B^f
    // guaranteed not to fail, based on f nonzero, B not identity
    EFqAffineExp(&sig->K, &sig->B, &ctx->f);
    EFqCp(&sig->T, &presig.T);

    // R1 = B^rf
    // guaranteed not to fail, if rf != p or 0, but bad inputs could cause it to
    // fail
    if (!EFqAffineExp(&presig.R1, &sig->B, &presig.rf)) {
      break;
    }

    // 5.  The member computes
    // t3 = Fp.hash(p || g1 || g2 || h1 || h2 || w || B || K || T || R1 || R2).
    tinysha_init(ctx->hash_alg, &sha_state);

    tinysha_update(&sha_state, (void const*)&epid20_p_str,
                   sizeof(epid20_p_str));
    tinysha_update(&sha_state, (void const*)&epid20_g1_str,
                   sizeof(epid20_g1_str));
    tinysha_update(&sha_state, (void const*)&epid20_g2_str,
                   sizeof(epid20_g2_str));
    tinysha_update(&sha_state, (void const*)&ctx->pub_key.h1,
                   sizeof(ctx->pub_key.h1));
    tinysha_update(&sha_state, (void const*)&ctx->pub_key.h2,
                   sizeof(ctx->pub_key.h2));
    tinysha_update(&sha_state, (void const*)&ctx->pub_key.w,
                   sizeof(ctx->pub_key.w));
    EFqSerialize(&g1_str, &sig->B);
    tinysha_update(&sha_state, (void const*)&g1_str, sizeof(g1_str));
    EFqSerialize(&g1_str, &sig->K);
    tinysha_update(&sha_state, (void const*)&g1_str, sizeof(g1_str));
    EFqSerialize(&g1_str, &sig->T);
    tinysha_update(&sha_state, (void const*)&g1_str, sizeof(g1_str));
    EFqSerialize(&g1_str, &presig.R1);
    tinysha_update(&sha_state, (void const*)&g1_str, sizeof(g1_str));
    Fq12Serialize(&fq12_str, &presig.R2);
    tinysha_update(&sha_state, (void const*)&fq12_str, sizeof(fq12_str));
    tinysha_final(digest.digest, &sha_state);
    FpFromHash(&sig->c, digest.digest, tinysha_digest_size(&sha_state));

    // 6.  The member computes c = Fp.hash(t3 || m).
    tinysha_init(ctx->hash_alg, &sha_state);
    FpSerialize(&fp_str, &sig->c);
    tinysha_update(&sha_state, (void const*)&fp_str, sizeof(fp_str));
    tinysha_update(&sha_state, msg, msg_len);
    tinysha_final(digest.digest, &sha_state);

    FpFromHash(&sig->c, digest.digest, tinysha_digest_size(&sha_state));
    // The variables sx, sf, sa, sb are computed from x, f, a, b with random
    // elements
    // This randomness allows verification but means that the s variables reveal
    // no secret information
    FpMul(&sig->sx, &sig->c, &x);
    FpMul(&sig->sf, &sig->c, &ctx->f);
    FpMul(&sig->sa, &sig->c, &presig.a);
    FpMul(&sig->sb, &sig->c, &presig.b);
    FpAdd(&sig->sx, &sig->sx, &presig.rx);
    FpAdd(&sig->sf, &sig->sf, &presig.rf);
    FpAdd(&sig->sa, &sig->sa, &presig.ra);
    FpAdd(&sig->sb, &sig->sb, &presig.rb);
    sts = kEpidNoErr;
  } while (0);
  // clearing stack-allocated variables before function return
  (void)memset(&presig, 0, sizeof(presig));
  return sts;
}
