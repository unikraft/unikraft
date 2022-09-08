/*############################################################################
  # Copyright 2016-2017 Intel Corporation
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
/// EpidNrProve implementation.
/*! \file */
#include "epid/member/src/nrprove.h"

#include <stddef.h>
#include <stdint.h>

#include "epid/common/src/endian_convert.h"
#include "epid/common/src/epid2params.h"
#include "epid/common/src/hashsize.h"
#include "epid/common/src/memory.h"
#include "epid/common/stdtypes.h"
#include "epid/common/types.h"
#include "epid/member/src/context.h"
#include "epid/member/src/nrprove_commitment.h"
#include "epid/member/src/privateexp.h"
#include "epid/member/tpm2/commit.h"
#include "epid/member/tpm2/sign.h"

/// Handle SDK Error with Break
#define BREAK_ON_EPID_ERROR(ret) \
  if (kEpidNoErr != (ret)) {     \
    break;                       \
  }

/// Count of elements in array
#define COUNT_OF(A) (sizeof(A) / sizeof((A)[0]))

static bool IsIdentity(G1ElemStr const* elem_str) {
  unsigned char* bytes = (unsigned char*)elem_str;
  if (!bytes) {
    return false;
  } else {
    size_t i = 0;
    for (i = 0; i < sizeof(*elem_str); i++) {
      if (0 != bytes[i]) return false;
    }
  }
  return true;
}

EpidStatus EpidNrProve(MemberCtx const* ctx, void const* msg, size_t msg_len,
                       void const* basename, size_t basename_len,
                       BasicSignature const* sig, SigRlEntry const* sigrl_entry,
                       NrProof* proof) {
  EpidStatus sts = kEpidErr;

  EcPoint* B = NULL;
  EcPoint* K = NULL;
  EcPoint* rlB = NULL;
  EcPoint* rlK = NULL;
  EcPoint* t = NULL;  // temp value in G1 either T, R1, R2
  EcPoint* k_tpm = NULL;
  EcPoint* l_tpm = NULL;
  EcPoint* e_tpm = NULL;
  EcPoint* D = NULL;
  FfElement* y2 = NULL;
  uint8_t* s2 = NULL;
  FfElement* mu = NULL;
  FfElement* nu = NULL;
  FfElement* rmu = NULL;

  FfElement* t2 = NULL;  // temporary for multiplication
  FfElement* c = NULL;
  uint8_t* digest = NULL;

  BigNumStr mu_str = {0};
  BigNumStr nu_str = {0};
  BigNumStr rmu_str = {0};

  if (!ctx || (0 != msg_len && !msg) || !sig || !sigrl_entry || !proof)
    return kEpidBadArgErr;
  if (!basename || 0 == basename_len) {
    // basename should not be empty
    return kEpidBadArgErr;
  }
  if (!ctx->epid2_params) return kEpidBadArgErr;

  do {
    NrProveCommitOutput commit_out = {0};
    FiniteField* Fp = ctx->epid2_params->Fp;
    FiniteField* Fq = ctx->epid2_params->Fq;
    EcGroup* G1 = ctx->epid2_params->G1;
    BitSupplier rnd_func = ctx->rnd_func;
    void* rnd_param = ctx->rnd_param;
    uint32_t i = 0;
    G1ElemStr B_str = {0};
    const BigNumStr kOne = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    FpElemStr c_str = {0};
    uint16_t rnu_ctr =
        0;  ///< TPM counter pointing to Nr Proof related random value
    size_t digest_len = EpidGetHashSize(ctx->hash_alg);

    sts = NewEcPoint(G1, &B);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewEcPoint(G1, &K);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewEcPoint(G1, &rlB);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewEcPoint(G1, &rlK);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewEcPoint(G1, &D);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewEcPoint(G1, &t);
    BREAK_ON_EPID_ERROR(sts);

    sts = NewFfElement(Fp, &y2);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewEcPoint(G1, &k_tpm);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewEcPoint(G1, &l_tpm);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewEcPoint(G1, &e_tpm);
    BREAK_ON_EPID_ERROR(sts);

    sts = NewFfElement(Fp, &mu);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewFfElement(Fp, &nu);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewFfElement(Fp, &rmu);
    BREAK_ON_EPID_ERROR(sts);

    s2 = SAFE_ALLOC(basename_len + sizeof(i));
    if (!s2) {
      sts = kEpidMemAllocErr;
      break;
    }
    sts = ReadEcPoint(G1, &sig->K, sizeof(sig->K), K);
    BREAK_ON_EPID_ERROR(sts);

    sts = ReadEcPoint(G1, &(sigrl_entry->b), sizeof(sigrl_entry->b), rlB);
    BREAK_ON_EPID_ERROR(sts);
    sts = ReadEcPoint(G1, &(sigrl_entry->k), sizeof(sigrl_entry->k), rlK);
    BREAK_ON_EPID_ERROR(sts);

    // 1.  The member chooses random mu from [1, p-1].
    sts = FfGetRandom(Fp, &kOne, rnd_func, rnd_param, mu);
    BREAK_ON_EPID_ERROR(sts);
    // 2.  The member computes nu = -mu mod p.
    sts = FfNeg(Fp, mu, nu);
    BREAK_ON_EPID_ERROR(sts);

    // 3.1. The member computes D = G1.privateExp(B', f)
    sts = EpidPrivateExp((MemberCtx*)ctx, rlB, D);
    BREAK_ON_EPID_ERROR(sts);
    // 3.2.The member computes T = G1.sscmMultiExp(K', mu, D, nu).
    sts = WriteFfElement(Fp, mu, &mu_str, sizeof(mu_str));
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteFfElement(Fp, nu, &nu_str, sizeof(nu_str));
    BREAK_ON_EPID_ERROR(sts);
    {
      EcPoint const* points[2];
      BigNumStr const* exponents[2];
      points[0] = rlK;
      points[1] = D;
      exponents[0] = &mu_str;
      exponents[1] = &nu_str;
      sts = EcSscmMultiExp(G1, points, exponents, COUNT_OF(points), t);
      BREAK_ON_EPID_ERROR(sts);
      sts = WriteEcPoint(G1, t, &commit_out.T, sizeof(commit_out.T));
      BREAK_ON_EPID_ERROR(sts);
    }
    // 4.1. The member chooses rmu randomly from[1, p - 1].
    sts = FfGetRandom(Fp, &kOne, rnd_func, rnd_param, rmu);
    BREAK_ON_EPID_ERROR(sts);
    // 4.2. (KTPM, LTPM, ETPM, counterTPM) = TPM2_Commit(P1 = B', P2 = B)
    sts = EcHash(G1, basename, basename_len, ctx->hash_alg, B, &i);
    BREAK_ON_EPID_ERROR(sts);
    *(uint32_t*)s2 = ntohl(i);
    sts = WriteEcPoint(G1, B, &B_str, sizeof(B_str));
    BREAK_ON_EPID_ERROR(sts);
    sts = ReadFfElement(Fq, &B_str.y, sizeof(B_str.y), y2);
    BREAK_ON_EPID_ERROR(sts);
    if (0 != memcpy_S(s2 + sizeof(i), basename_len, basename, basename_len)) {
      sts = kEpidErr;
      break;
    }
    sts = Tpm2Commit(ctx->tpm2_ctx, rlB, s2, basename_len + sizeof(i), y2,
                     k_tpm, l_tpm, e_tpm, &rnu_ctr);
    BREAK_ON_EPID_ERROR(sts);

    // 5.1. The member computes R1 = G1.sscmExp(K, rmu).
    sts = WriteFfElement(Fp, rmu, &rmu_str, sizeof(rmu_str));
    BREAK_ON_EPID_ERROR(sts);
    sts = EcSscmExp(G1, K, &rmu_str, t);
    BREAK_ON_EPID_ERROR(sts);
    // 5.2. The member computes R1 = G1.mul(R1, LTPM).
    sts = EcMul(G1, t, l_tpm, t);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteEcPoint(G1, t, &commit_out.R1, sizeof(commit_out.R1));
    BREAK_ON_EPID_ERROR(sts);

    // 6.1. The member computes R2 = G1.sscmExp(K', rmu).
    sts = EcSscmExp(G1, rlK, &rmu_str, t);
    BREAK_ON_EPID_ERROR(sts);
    // 6.2. The member computes R2 = G1.mul(R2, ETPM).
    sts = EcMul(G1, t, e_tpm, t);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteEcPoint(G1, t, &commit_out.R2, sizeof(commit_out.R2));
    BREAK_ON_EPID_ERROR(sts);

    sts = HashNrProveCommitment(Fp, ctx->hash_alg, &sig->B, &sig->K,
                                sigrl_entry, &commit_out, msg, msg_len, &c_str);
    BREAK_ON_EPID_ERROR(sts);

    digest = SAFE_ALLOC(digest_len);
    if (!digest) {
      sts = kEpidMemAllocErr;
      break;
    }

    sts = NewFfElement(Fp, &t2);
    BREAK_ON_EPID_ERROR(sts);

    sts = NewFfElement(Fp, &c);
    BREAK_ON_EPID_ERROR(sts);

    sts = ReadFfElement(Fp, &c_str, sizeof(c_str), c);
    BREAK_ON_EPID_ERROR(sts);

    // 8.  The member computes smu = (rmu + c * mu) mod p.
    sts = FfMul(Fp, c, mu, t2);
    BREAK_ON_EPID_ERROR(sts);
    sts = FfAdd(Fp, rmu, t2, t2);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteFfElement(Fp, t2, &proof->smu, sizeof(proof->smu));
    BREAK_ON_EPID_ERROR(sts);

    // 9.1. The member computes c' = (c * nu) mod p
    sts = FfMul(Fp, c, nu, t2);
    BREAK_ON_EPID_ERROR(sts);
    // 9.2. snu = TPM2_Sign(c = c', counterTPM)
    sts = WriteFfElement(Fp, t2, digest, digest_len);
    BREAK_ON_EPID_ERROR(sts);
    sts = Tpm2Sign(ctx->tpm2_ctx, digest, digest_len, rnu_ctr, NULL, t2);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteFfElement(Fp, t2, &proof->snu, sizeof(proof->snu));
    BREAK_ON_EPID_ERROR(sts);

    // 10. The member outputs sigma = (T, c, smu, snu), a non-revoked
    //     proof. If G1.is_identity(T) = true, the member also outputs
    //     "failed".

    proof->T = commit_out.T;
    proof->c = c_str;

    if (IsIdentity(&proof->T)) {
      sts = kEpidSigRevokedInSigRl;
      BREAK_ON_EPID_ERROR(sts);
    }

    sts = kEpidNoErr;
  } while (0);

  SAFE_FREE(s2);
  EpidZeroMemory(&mu_str, sizeof(mu_str));
  EpidZeroMemory(&nu_str, sizeof(nu_str));
  EpidZeroMemory(&rmu_str, sizeof(rmu_str));
  DeleteFfElement(&y2);
  DeleteEcPoint(&B);
  DeleteEcPoint(&K);
  DeleteEcPoint(&rlB);
  DeleteEcPoint(&rlK);
  DeleteEcPoint(&D);
  DeleteEcPoint(&t);
  DeleteEcPoint(&e_tpm);
  DeleteEcPoint(&l_tpm);
  DeleteEcPoint(&k_tpm);
  DeleteFfElement(&mu);
  DeleteFfElement(&nu);
  DeleteFfElement(&rmu);
  DeleteFfElement(&t2);
  DeleteFfElement(&c);
  SAFE_FREE(digest);

  return sts;
}
