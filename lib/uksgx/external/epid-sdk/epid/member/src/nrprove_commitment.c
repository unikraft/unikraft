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
/// Host non-revoked proof helper implementation
/*! \file */

#include "epid/member/src/nrprove_commitment.h"

#include <stdint.h>
#include "epid/common/math/finitefield.h"
#include "epid/common/src/memory.h"

/// Handle SDK Error with Break
#define BREAK_ON_EPID_ERROR(ret) \
  if (kEpidNoErr != (ret)) {     \
    break;                       \
  }

#pragma pack(1)
/// Storage for values to create commitment in NrProve algorithm
typedef struct NrProveCommitValues {
  BigNumStr p;    //!< A large prime (256-bit)
  G1ElemStr g1;   //!< Generator of G1 (512-bit)
  G1ElemStr B;    //!< (element of G1): part of basic signature Sigma0
  G1ElemStr K;    //!< (element of G1): part of basic signature Sigma0
  G1ElemStr rlB;  //!< (element of G1): one entry in SigRL
  G1ElemStr rlK;  //!< (element of G1): one entry in SigRL
  NrProveCommitOutput commit_out;  //!< output of NrProveCommit
  uint8_t msg[1];                  //!< message
} NrProveCommitValues;
#pragma pack()

EpidStatus HashNrProveCommitment(FiniteField* Fp, HashAlg hash_alg,
                                 G1ElemStr const* B_str, G1ElemStr const* K_str,
                                 SigRlEntry const* sigrl_entry,
                                 NrProveCommitOutput const* commit_out,
                                 void const* msg, size_t msg_len,
                                 FpElemStr* c_str) {
  EpidStatus sts = kEpidErr;
  FfElement* c = NULL;
  NrProveCommitValues* commit_values = NULL;

  if (!Fp || !B_str || !K_str || !sigrl_entry || !commit_out ||
      (0 != msg_len && !msg) || !c_str) {
    return kEpidBadArgErr;
  }

  if (msg_len >
      ((SIZE_MAX - sizeof(*commit_values)) + sizeof(*commit_values->msg)))
    return kEpidBadArgErr;

  do {
    size_t const commit_len =
        sizeof(*commit_values) - sizeof(*commit_values->msg) + msg_len;
    Epid2Params params = {
#include "epid/common/src/epid2params_ate.inc"
    };

    commit_values = SAFE_ALLOC(commit_len);
    if (!commit_values) {
      sts = kEpidMemAllocErr;
      BREAK_ON_EPID_ERROR(sts);
    }

    commit_values->p = params.p;
    commit_values->g1 = params.g1;
    commit_values->B = *B_str;
    commit_values->K = *K_str;
    commit_values->rlB = sigrl_entry->b;
    commit_values->rlK = sigrl_entry->k;
    commit_values->commit_out = *commit_out;

    // commit_values is allocated such that there are msg_len bytes available
    // starting at commit_values->msg
    if (msg) {
      // Memory copy is used to copy a message of variable length
      if (0 != memcpy_S(&commit_values->msg[0], msg_len, msg, msg_len)) {
        sts = kEpidBadArgErr;
        BREAK_ON_EPID_ERROR(sts);
      }
    }

    sts = NewFfElement(Fp, &c);
    BREAK_ON_EPID_ERROR(sts);

    // 7.  The member computes c = Fp.hash(p || g1 || B || K || B' ||
    //     K' || T || R1 || R2 || m).
    sts = FfHash(Fp, commit_values, commit_len, hash_alg, c);
    BREAK_ON_EPID_ERROR(sts);

    sts = WriteFfElement(Fp, c, c_str, sizeof(*c_str));
    BREAK_ON_EPID_ERROR(sts);

    sts = kEpidNoErr;
  } while (0);

  SAFE_FREE(commit_values);
  DeleteFfElement(&c);

  return sts;
}
