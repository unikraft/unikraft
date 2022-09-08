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
/// Host signing helper implementation
/*! \file */

#include "epid/member/src/sign_commitment.h"

#include "epid/common/math/ecgroup.h"
#include "epid/common/src/commitment.h"

/// Handle SDK Error with Break
#define BREAK_ON_EPID_ERROR(ret) \
  if (kEpidNoErr != (ret)) {     \
    break;                       \
  }

EpidStatus HashSignCommitment(FiniteField* Fp, HashAlg hash_alg,
                              GroupPubKey const* pub_key,
                              SignCommitOutput const* commit_out,
                              void const* msg, size_t msg_len,
                              FpElemStr* c_str) {
  EpidStatus sts = kEpidErr;
  FfElement* c = NULL;

  if (!Fp || !commit_out || (0 != msg_len && !msg) || !c_str) {
    return kEpidBadArgErr;
  }

  do {
    CommitValues values = {0};
    sts = SetKeySpecificCommitValues(pub_key, &values);
    BREAK_ON_EPID_ERROR(sts);

    values.B = commit_out->B;
    values.K = commit_out->K;
    values.T = commit_out->T;
    values.R1 = commit_out->R1;
    values.R2 = commit_out->R2;

    sts = NewFfElement(Fp, &c);
    BREAK_ON_EPID_ERROR(sts);

    // 5.  The member computes t3 = Fp.hash(p || g1 || g2 || h1 || h2
    //     || w || B || K || T || R1 || R2).
    // 6.  The member computes c = Fp.hash(t3 || m).
    sts = CalculateCommitmentHash(&values, Fp, hash_alg, msg, msg_len, c);
    BREAK_ON_EPID_ERROR(sts);

    sts = WriteFfElement(Fp, c, c_str, sizeof(*c_str));
    BREAK_ON_EPID_ERROR(sts);

    sts = kEpidNoErr;
  } while (0);

  DeleteFfElement(&c);

  return sts;
}
