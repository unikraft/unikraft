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
/// Host join helper implementation
/*! \file */

#include "epid/member/src/join_commitment.h"

#include "epid/common/math/finitefield.h"
#include "epid/common/types.h"

/// Handle SDK Error with Break
#define BREAK_ON_EPID_ERROR(ret) \
  if (kEpidNoErr != (ret)) {     \
    break;                       \
  }

#pragma pack(1)
/// Storage for values to create commitment in Sign and Verify algorithms
typedef struct JoinPCommitValues {
  BigNumStr p;     ///< Intel(R) EPID 2.0 parameter p
  G1ElemStr g1;    ///< Intel(R) EPID 2.0 parameter g1
  G2ElemStr g2;    ///< Intel(R) EPID 2.0 parameter g2
  G1ElemStr h1;    ///< Group public key value h1
  G1ElemStr h2;    ///< Group public key value h2
  G2ElemStr w;     ///< Group public key value w
  G1ElemStr F;     ///< Variable F computed in algorithm
  G1ElemStr R;     ///< Variable R computed in algorithm
  IssuerNonce NI;  ///< Nonce
} JoinPCommitValues;
#pragma pack()

EpidStatus HashJoinCommitment(FiniteField* Fp, HashAlg hash_alg,
                              GroupPubKey const* pub_key,
                              G1ElemStr const* F_str, G1ElemStr const* R_str,
                              IssuerNonce const* NI, FpElemStr* c_str) {
  EpidStatus sts = kEpidErr;
  FfElement* c = NULL;

  if (!Fp || !pub_key || !F_str || !R_str || !NI || !c_str) {
    return kEpidBadArgErr;
  }

  do {
    JoinPCommitValues commit_values = {0};
    Epid2Params params = {
#include "epid/common/src/epid2params_ate.inc"
    };

    commit_values.p = params.p;
    commit_values.g1 = params.g1;
    commit_values.g2 = params.g2;
    commit_values.h1 = pub_key->h1;
    commit_values.h2 = pub_key->h2;
    commit_values.w = pub_key->w;
    commit_values.F = *F_str;
    commit_values.R = *R_str;
    commit_values.NI = *NI;

    sts = NewFfElement(Fp, &c);
    BREAK_ON_EPID_ERROR(sts);

    // Step 4. The member computes c = Fp.hash(p || g1 || g2 || h1 ||
    // h2 || w || F || R || NI).
    sts = FfHash(Fp, &commit_values, sizeof(commit_values), hash_alg, c);
    BREAK_ON_EPID_ERROR(sts);

    sts = WriteFfElement(Fp, c, c_str, sizeof(*c_str));
    BREAK_ON_EPID_ERROR(sts);

    sts = kEpidNoErr;
  } while (0);

  DeleteFfElement(&c);

  return sts;
}
