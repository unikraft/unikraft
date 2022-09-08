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
/// Precomputed signature computation.
/*! \file */

#ifndef EPID_MEMBER_TINY_SRC_PRESIG_COMPUTE_H_
#define EPID_MEMBER_TINY_SRC_PRESIG_COMPUTE_H_

#include "epid/common/errors.h"
#include "epid/member/tiny/math/mathtypes.h"

/// \cond
typedef struct MembershipCredentialData MembershipCredentialData;
typedef struct MemberPairPrecomp MemberPairPrecomp;
typedef struct MemberCtx MemberCtx;
/// \endcond

/*! Tiny pre-computed signature.
B and K values are not included into signeture pre-computation,
as compared with Intel(R) EPID 2.0 spec, to favor small size.
*/
typedef struct PreComputedSignatureData {
  EccPointFq T;   ///< an element in G1
  EccPointFq R1;  ///< an element in G1
  Fq12Elem R2;    ///< an element in G1
  FpElem a;       ///< an integer between [0, p-1]
  FpElem b;       ///< an integer between [0, p-1]
  FpElem rx;      ///< an integer between [0, p-1]
  FpElem rf;      ///< an integer between [0, p-1]
  FpElem ra;      ///< an integer between [0, p-1]
  FpElem rb;      ///< an integer between [0, p-1]
} PreComputedSignatureData;

/// Performs signature pre-computation
EpidStatus EpidMemberComputePreSig(MemberCtx const* ctx,
                                   PreComputedSignatureData* presig);

#endif  // EPID_MEMBER_TINY_SRC_PRESIG_COMPUTE_H_
