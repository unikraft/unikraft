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
/// EpidPrivateExp unit tests.
/*! \file */
#include "epid/common-testhelper/epid_gtest-testhelper.h"
#include "gtest/gtest.h"

extern "C" {
#include "epid/member/src/privateexp.h"
}

#include "epid/common-testhelper/epid2params_wrapper-testhelper.h"
#include "epid/common-testhelper/epid_params-testhelper.h"
#include "epid/common-testhelper/errors-testhelper.h"
#include "epid/common-testhelper/prng-testhelper.h"
#include "epid/member/unittests/member-testhelper.h"

namespace {

////////////////////////////////////////////////
//  EpidPrivateExp
TEST_F(EpidMemberTest, EpidPrivateExpFailsGivenNullPointer) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  MemberCtxObj member(&Prng::Generate, &my_prng);
  THROW_ON_EPIDERR(EpidMemberSetHashAlg(member, kSha256));
  THROW_ON_EPIDERR(EpidProvisionKey(member, &this->kGroupPublicKey,
                                    &this->kMemberPrivateKey,
                                    &this->kMemberPrecomp));
  THROW_ON_EPIDERR(EpidMemberStartup(member));

  Epid20Params params;
  EcPointObj a(&params.G1, this->kGroupPublicKey.h1), r(&params.G1);

  EXPECT_EQ(kEpidBadArgErr, EpidPrivateExp(nullptr, a, r));
  EXPECT_EQ(kEpidBadArgErr, EpidPrivateExp(member, nullptr, r));
  EXPECT_EQ(kEpidBadArgErr, EpidPrivateExp(member, a, nullptr));
}

TEST_F(EpidMemberTest, EpidPrivateExpFailsArgumentsMismatch) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  MemberCtxObj member(&Prng::Generate, &my_prng);
  THROW_ON_EPIDERR(EpidMemberSetHashAlg(member, kSha256));
  THROW_ON_EPIDERR(EpidProvisionKey(member, &this->kGroupPublicKey,
                                    &this->kMemberPrivateKey,
                                    &this->kMemberPrecomp));
  THROW_ON_EPIDERR(EpidMemberStartup(member));

  Epid20Params params;
  EcPointObj a(&params.G1, this->kGroupPublicKey.h1), r(&params.G1);
  EcPointObj g2(&params.G2, this->kGroupPublicKey.w);

  EXPECT_EQ(kEpidBadArgErr, EpidPrivateExp(member, g2, r));
  EXPECT_EQ(kEpidBadArgErr, EpidPrivateExp(member, a, g2));
  EXPECT_EQ(kEpidBadArgErr, EpidPrivateExp(member, g2, g2));
}

TEST_F(EpidMemberTest, EpidPrivateExpSucceedsGivenValidParametersForSha256) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  MemberCtxObj member(&Prng::Generate, &my_prng);
  THROW_ON_EPIDERR(EpidMemberSetHashAlg(member, kSha256));
  THROW_ON_EPIDERR(EpidProvisionKey(member, &this->kGroupPublicKey,
                                    &this->kMemberPrivateKey,
                                    &this->kMemberPrecomp));
  THROW_ON_EPIDERR(EpidMemberStartup(member));

  Epid20Params params;
  EcPointObj a(&params.G1, this->kGroupPublicKey.h1), r(&params.G1),
      r_expected(&params.G1);

  G1ElemStr r_str, r_expected_str;

  EXPECT_EQ(kEpidNoErr, EpidPrivateExp(member, a, r));

  THROW_ON_EPIDERR(EcExp(
      params.G1, a, (BigNumStr const*)&this->kMemberPrivateKey.f, r_expected));

  THROW_ON_EPIDERR(WriteEcPoint(params.G1, r, &r_str, sizeof(r_str)));
  THROW_ON_EPIDERR(WriteEcPoint(params.G1, r_expected, &r_expected_str,
                                sizeof(r_expected_str)));
  EXPECT_EQ(r_expected_str, r_str);
}

#ifndef TPM_TSS
TEST_F(EpidMemberTest, EpidPrivateExpSucceedsGivenValidParametersForSha384) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  MemberCtxObj member(&Prng::Generate, &my_prng);
  THROW_ON_EPIDERR(EpidMemberSetHashAlg(member, kSha384));
  THROW_ON_EPIDERR(EpidProvisionKey(member, &this->kGroupPublicKey,
                                    &this->kMemberPrivateKey,
                                    &this->kMemberPrecomp));
  THROW_ON_EPIDERR(EpidMemberStartup(member));

  Epid20Params params;
  EcPointObj a(&params.G1, this->kGroupPublicKey.h1), r(&params.G1),
      r_expected(&params.G1);

  G1ElemStr r_str, r_expected_str;

  EXPECT_EQ(kEpidNoErr, EpidPrivateExp(member, a, r));

  THROW_ON_EPIDERR(EcExp(
      params.G1, a, (BigNumStr const*)&this->kMemberPrivateKey.f, r_expected));

  THROW_ON_EPIDERR(WriteEcPoint(params.G1, r, &r_str, sizeof(r_str)));
  THROW_ON_EPIDERR(WriteEcPoint(params.G1, r_expected, &r_expected_str,
                                sizeof(r_expected_str)));
  EXPECT_EQ(r_expected_str, r_str);
}
#endif

TEST_F(EpidMemberTest,
       PROTECTED_EpidPrivateExpSucceedsByCredentialForSha256_EPS0) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  MemberCtxObj member(&Prng::Generate, &my_prng);
  BigNumStr const f_str = {0x7a, 0x57, 0x41, 0x5b, 0x85, 0x44, 0x0e, 0x2b,
                           0xb3, 0xcc, 0xa7, 0x99, 0x6d, 0x19, 0x79, 0x45,
                           0x04, 0xb8, 0x94, 0x07, 0x47, 0x14, 0xed, 0x8d,
                           0xf4, 0x1e, 0x7d, 0xa0, 0x17, 0xc5, 0xc4, 0x10};
  THROW_ON_EPIDERR(EpidMemberSetHashAlg(member, kSha256));

  Epid20Params params;
  EcPointObj a(&params.G1, this->kGroupPublicKey.h1), r(&params.G1),
      r_expected(&params.G1);

  G1ElemStr r_str, r_expected_str;

  EXPECT_EQ(kEpidNoErr, EpidPrivateExp(member, a, r));

  THROW_ON_EPIDERR(EcExp(params.G1, a, (BigNumStr const*)&f_str, r_expected));

  THROW_ON_EPIDERR(WriteEcPoint(params.G1, r, &r_str, sizeof(r_str)));
  THROW_ON_EPIDERR(WriteEcPoint(params.G1, r_expected, &r_expected_str,
                                sizeof(r_expected_str)));
  EXPECT_EQ(r_expected_str, r_str);
}

}  // namespace
