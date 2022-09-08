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
/// Host non-revoked proof helper APIs
/*! \file */
#ifndef EPID_MEMBER_SRC_NRPROVE_COMMITMENT_H_
#define EPID_MEMBER_SRC_NRPROVE_COMMITMENT_H_

#include <stddef.h>
#include "epid/common/errors.h"
#include "epid/common/types.h"  // HashAlg, G1ElemStr

/// \cond
typedef struct FiniteField FiniteField;
typedef struct FpElemStr FpElemStr;
typedef struct SigRlEntry SigRlEntry;
typedef struct NrProveCommitOutput NrProveCommitOutput;
/// \endcond

#pragma pack(1)
/// Result of NrProve Commit
typedef struct NrProveCommitOutput {
  G1ElemStr T;   ///< T value for NrProof
  G1ElemStr R1;  ///< Serialized G1 element
  G1ElemStr R2;  ///< Serialized G1 element
} NrProveCommitOutput;
#pragma pack()

/// Calculates commitment hash of NrProve commit
/*!

  \param[in] Fp
  The finite field.

  \param[in] hash_alg
  The hash algorithm.

  \param[in] B_str
  The B value from the ::BasicSignature.

  \param[in] K_str
  The K value from the ::BasicSignature.

  \param[in] sigrl_entry
  The signature based revocation list entry corresponding to this
  proof.

  \param[in] commit_out
  The output from the NrProve commit.

  \param[in] msg
  The message.

  \param[in] msg_len
  The size of message in bytes.

  \param[out] c_str
  The resulting commitment hash.

  \returns ::EpidStatus

 */
EpidStatus HashNrProveCommitment(FiniteField* Fp, HashAlg hash_alg,
                                 G1ElemStr const* B_str, G1ElemStr const* K_str,
                                 SigRlEntry const* sigrl_entry,
                                 NrProveCommitOutput const* commit_out,
                                 void const* msg, size_t msg_len,
                                 FpElemStr* c_str);

#endif  // EPID_MEMBER_SRC_NRPROVE_COMMITMENT_H_
