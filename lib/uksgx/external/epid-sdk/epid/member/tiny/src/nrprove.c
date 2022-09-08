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
/// Tiny EpidNrProve implementation.
/*! \file */

#include "epid/member/tiny/src/nrprove.h"
#include "epid/common/errors.h"
#include "epid/member/tiny/math/efq.h"
#include "epid/member/tiny/math/fp.h"
#include "epid/member/tiny/math/hashwrap.h"
#include "epid/member/tiny/math/mathtypes.h"
#include "epid/member/tiny/math/serialize.h"
#include "epid/member/tiny/src/context.h"
#include "epid/member/tiny/src/native_types.h"
static const FpElem epid20_p = {
    {{0xD10B500D, 0xF62D536C, 0x1299921A, 0x0CDC65FB, 0xEE71A49E, 0x46E5F25E,
      0xFFFCF0CD, 0xFFFFFFFF}}};
static const EccPointFq epid20_g1 = {
    {{{0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
       0x00000000, 0x00000000}}},
    {{{0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
       0x00000000, 0x00000000}}}};
EpidStatus EpidNrProve(MemberCtx const* ctx, void const* msg, size_t msg_len,
                       NativeBasicSignature const* sig,
                       SigRlEntry const* sigrl_entry, NrProof* proof) {
  EpidStatus sts = kEpidBadArgErr;
  FpElem mu;
  FpElem rmu;
  FpElem smu;
  FpElem nu;
  FpElem rnu;
  FpElem snu;
  EccPointFq tmp_efq;
  EccPointFq B_tick;
  EccPointFq K_tick;
  BigNumStr p;
  G1ElemStr tmp_str;
  G1ElemStr T_str;
  G1ElemStr R1_str;
  G1ElemStr R2_str;
  FpElem c;
  tiny_sha sha_state;
  sha_digest digest;
  EFqDeserialize(&B_tick, &sigrl_entry->b);
  EFqDeserialize(&K_tick, &sigrl_entry->k);
  if (!EFqOnCurve(&B_tick) || !EFqOnCurve(&K_tick) || !EFqOnCurve(&sig->B) ||
      !EFqOnCurve(&sig->K)) {
    return kEpidBadArgErr;
  }
  // 1. The member chooses random mu from [1, p-1].
  if (!FpRandNonzero(&mu, ctx->rnd_func, ctx->rnd_param)) {
    return kEpidMathErr;
  }
  // 2. The member computes nu = (- f * mu) mod p.
  FpNeg(&rmu, &ctx->f);
  FpMul(&nu, &rmu, &mu);
  do {
    // 3. The member computes T = G1.sscmMultiExp(K', mu, B', nu). If
    // G1.isIdentity(T) = true, the member also outputs "failed".
    if (!EFqAffineMultiExp(&tmp_efq, &K_tick, &mu, &B_tick, &nu)) {
      sts = kEpidSigRevokedInSigRl;
      break;
    }
    EFqSerialize(&T_str, &tmp_efq);
    // 4. The member chooses rmu, rnu randomly from[1, p - 1].
    if (!FpRandNonzero(&rmu, ctx->rnd_func, ctx->rnd_param)) {
      sts = kEpidMathErr;
      break;
    }
    if (!FpRandNonzero(&rnu, ctx->rnd_func, ctx->rnd_param)) {
      sts = kEpidMathErr;
      break;
    }
    // 5. The member computes R1 = G1.sscmMultiExp(K, rmu, B, rnu).
    if (!EFqAffineMultiExp(&tmp_efq, &sig->K, &rmu, &sig->B, &rnu)) {
      break;
    }
    EFqSerialize(&R1_str, &tmp_efq);
    // 6. The member computes R2 = G1.sscmMultiExp(K', rmu, B', rnu).
    if (!EFqAffineMultiExp(&tmp_efq, &K_tick, &rmu, &B_tick, &rnu)) {
      break;
    }
    EFqSerialize(&R2_str, &tmp_efq);
    // 7. The member computes c = Fp.hash(p || g1 || B || K || B' || K' || T ||
    // R1 ||  R2 || m)
    tinysha_init(ctx->hash_alg, &sha_state);
    VliSerialize(&p, &(epid20_p.limbs));
    tinysha_update(&sha_state, (void const*)&p, sizeof(p));
    EFqSerialize(&tmp_str, &epid20_g1);
    tinysha_update(&sha_state, (void const*)&tmp_str, sizeof(tmp_str));
    EFqSerialize(&tmp_str, &sig->B);
    tinysha_update(&sha_state, (void const*)&tmp_str, sizeof(tmp_str));
    EFqSerialize(&tmp_str, &sig->K);
    tinysha_update(&sha_state, (void const*)&tmp_str, sizeof(tmp_str));
    tinysha_update(&sha_state, (void const*)&sigrl_entry->b,
                   sizeof(sigrl_entry->b));
    tinysha_update(&sha_state, (void const*)&sigrl_entry->k,
                   sizeof(sigrl_entry->k));
    tinysha_update(&sha_state, (void const*)&T_str, sizeof(T_str));
    tinysha_update(&sha_state, (void const*)&R1_str, sizeof(R1_str));
    tinysha_update(&sha_state, (void const*)&R2_str, sizeof(R2_str));
    tinysha_update(&sha_state, msg, msg_len);
    tinysha_final(digest.digest, &sha_state);
    FpFromHash(&c, digest.digest, tinysha_digest_size(&sha_state));
    // 8. The member computes smu = (rmu + c * mu) mod p.
    FpMul(&smu, &c, &mu);
    FpAdd(&smu, &rmu, &smu);
    // 9. The member computes snu = (rnu + c * nu) mod p.
    FpMul(&snu, &c, &nu);
    FpAdd(&snu, &rnu, &snu);
    // 10. The member outputs proof = (T, c, smu, snu), a non - revoked proof
    FpSerialize(&proof->c, &c);
    FpSerialize(&proof->smu, &smu);
    FpSerialize(&proof->snu, &snu);
    proof->T = T_str;
    sts = kEpidNoErr;
  } while (0);
  FpClear(&mu);
  FpClear(&nu);
  FpClear(&rmu);
  FpClear(&rnu);
  return sts;
}
