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
/// Host join helper APIs
/*! \file */
#ifndef EPID_MEMBER_SRC_JOIN_COMMITMENT_H_
#define EPID_MEMBER_SRC_JOIN_COMMITMENT_H_

#include "epid/common/errors.h"
#include "epid/common/types.h"  // HashAlg

/// \cond
typedef struct FiniteField FiniteField;
typedef struct G1ElemStr G1ElemStr;
typedef struct FpElemStr FpElemStr;
typedef struct OctStr256 OctStr256;
typedef struct OctStr256 IssuerNonce;
/// \endcond

/// Calculates commitment hash of join commit
/*!

  \param[in] Fp
  The finite field.

  \param[in] hash_alg
  The hash algorithm.

  \param[in] pub_key
  The public key of the group being joined.

  \param[in] F_str
  The F value of the join commit.

  \param[in] R_str
  The R value of the join commit.

  \param[in] NI
  The nonce chosen by issuer as part of join protocol.

  \param[out] c_str
  The resulting commitment hash.

  \returns ::EpidStatus

 */
EpidStatus HashJoinCommitment(FiniteField* Fp, HashAlg hash_alg,
                              GroupPubKey const* pub_key,
                              G1ElemStr const* F_str, G1ElemStr const* R_str,
                              IssuerNonce const* NI, FpElemStr* c_str);

#endif  // EPID_MEMBER_SRC_JOIN_COMMITMENT_H_
