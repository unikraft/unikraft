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
/// Member pre-computation implementation
/*! \file */
#include "epid/member/src/precomp.h"

#include "epid/common/src/epid2params.h"
#include "epid/common/src/grouppubkey.h"
#include "epid/common/types.h"

/// Handle SDK Error with Break
#define BREAK_ON_EPID_ERROR(ret) \
  if (kEpidNoErr != (ret)) {     \
    break;                       \
  }

EpidStatus PrecomputeMemberPairing(Epid2Params_ const* epid2_params,
                                   GroupPubKey const* pub_key,
                                   G1ElemStr const* A_str,
                                   MemberPrecomp* precomp) {
  EpidStatus sts = kEpidErr;

  GroupPubKey_* pub_key_ = NULL;
  EcPoint* A = NULL;
  FfElement* e = NULL;

  if (!epid2_params || !pub_key || !A_str || !precomp) return kEpidBadArgErr;

  do {
    EcGroup* G1 = epid2_params->G1;
    EcGroup* G2 = epid2_params->G2;
    FiniteField* GT = epid2_params->GT;
    PairingState* ps_ctx = epid2_params->pairing_state;
    EcPoint* g2 = epid2_params->g2;

    sts = CreateGroupPubKey(pub_key, G1, G2, &pub_key_);
    BREAK_ON_EPID_ERROR(sts);

    sts = NewFfElement(GT, &e);
    BREAK_ON_EPID_ERROR(sts);

    // 1. The member computes e12 = pairing(h1, g2).
    sts = Pairing(ps_ctx, pub_key_->h1, g2, e);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteFfElement(GT, e, &precomp->e12, sizeof(precomp->e12));
    BREAK_ON_EPID_ERROR(sts);

    // 2.  The member computes e22 = pairing(h2, g2).
    sts = Pairing(ps_ctx, pub_key_->h2, g2, e);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteFfElement(GT, e, &precomp->e22, sizeof(precomp->e22));
    BREAK_ON_EPID_ERROR(sts);

    // 3.  The member computes e2w = pairing(h2, w).
    sts = Pairing(ps_ctx, pub_key_->h2, pub_key_->w, e);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteFfElement(GT, e, &precomp->e2w, sizeof(precomp->e2w));
    BREAK_ON_EPID_ERROR(sts);

    // 4.  The member computes ea2 = pairing(A, g2).
    sts = NewEcPoint(G1, &A);
    BREAK_ON_EPID_ERROR(sts);
    sts = ReadEcPoint(G1, A_str, sizeof(*A_str), A);
    BREAK_ON_EPID_ERROR(sts);
    sts = Pairing(ps_ctx, A, g2, e);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteFfElement(GT, e, &precomp->ea2, sizeof(precomp->ea2));
    BREAK_ON_EPID_ERROR(sts);

    sts = kEpidNoErr;
  } while (0);

  DeleteGroupPubKey(&pub_key_);
  DeleteEcPoint(&A);
  DeleteFfElement(&e);

  return sts;
}
