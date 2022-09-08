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
/// Host signing helper APIs
/*! \file */
#ifndef EPID_MEMBER_SRC_SIGN_COMMITMENT_H_
#define EPID_MEMBER_SRC_SIGN_COMMITMENT_H_

#include <stddef.h>
#include "epid/common/errors.h"
#include "epid/common/types.h"  // HashAlg

#pragma pack(1)
/// Result of Sign Commit
typedef struct SignCommitOutput {
  G1ElemStr B;   ///< B value for signature
  G1ElemStr K;   ///< K value for signature
  G1ElemStr T;   ///< T value for signature
  G1ElemStr R1;  ///< Serialized G1 element
  GtElemStr R2;  ///< Serialized GT element
} SignCommitOutput;
#pragma pack()

typedef struct SignCommitOutput SignCommitOutput;

/// \cond
typedef struct FiniteField FiniteField;
typedef struct FpElemStr FpElemStr;
/// \endcond

/// Calculates commitment hash of sign commit
/*!

  \param[in] Fp
  The finite field.

  \param[in] hash_alg
  The hash algorithm.

  \param[in] commit_out
  The output from the sign commit.

  \param[in] msg
  The message.

  \param[in] msg_len
  The size of message in bytes.

  \param[out] c_str
  The resulting commitment hash.

  \returns ::EpidStatus

 */
EpidStatus HashSignCommitment(FiniteField* Fp, HashAlg hash_alg,
                              GroupPubKey const* pub_key,
                              SignCommitOutput const* commit_out,
                              void const* msg, size_t msg_len,
                              FpElemStr* c_str);

#endif  // EPID_MEMBER_SRC_SIGN_COMMITMENT_H_
