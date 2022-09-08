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
/// Member pre-computation API
/*! \file */
#ifndef EPID_MEMBER_SRC_PRECOMP_H_
#define EPID_MEMBER_SRC_PRECOMP_H_

#include "epid/common/errors.h"

/// \cond
typedef struct Epid2Params_ Epid2Params_;
typedef struct GroupPubKey GroupPubKey;
typedef struct G1ElemStr G1ElemStr;
typedef struct MemberPrecomp MemberPrecomp;
/// \endcond

/// Precomputes pairing values for member
/*!

  The the result of the expensive pairing operations can be
  pre-computed. The pre-computation result can be saved for future
  sign operations using the same group and private key.

  \param[in] epid2_params
  The field and group parameters.

  \param[in] pub_key
  The public key of the group.

  \param[in] A_str
  The A value of the member private key.

  \param[out] precomp
  The member pre-computed data.

  \returns ::EpidStatus

  \see CreateEpid2Params

 */
EpidStatus PrecomputeMemberPairing(Epid2Params_ const* epid2_params,
                                   GroupPubKey const* pub_key,
                                   G1ElemStr const* A_str,
                                   MemberPrecomp* precomp);

#endif  // EPID_MEMBER_SRC_PRECOMP_H_
