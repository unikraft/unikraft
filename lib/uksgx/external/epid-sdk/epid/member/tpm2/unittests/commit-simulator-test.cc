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
/// Tpm2Commit unit tests.
/*! \file */

#include <cstring>

#include "gtest/gtest.h"

#include "epid/common-testhelper/epid2params_wrapper-testhelper.h"
#include "epid/common-testhelper/epid_params-testhelper.h"
#include "epid/common-testhelper/errors-testhelper.h"
#include "epid/common-testhelper/prng-testhelper.h"
#include "epid/member/tpm2/unittests/tpm2-testhelper.h"

extern "C" {
#include "epid/common/src/epid2params.h"
#include "epid/member/tpm2/commit.h"
#include "epid/member/tpm2/context.h"
#include "epid/member/tpm2/load_external.h"
#include "epid/member/tpm2/sign.h"
#include "epid/member/tpm2/src/state.h"
}

namespace {

TEST_F(EpidTpm2Test, CommitComputeKLESha256) {
  // Testing step i and j of the "C.2.3 Tpm2Commit()"
  Epid20Params params;
  EcPointObj k(&params.G1), l(&params.G1), e(&params.G1);
  EcPointObj p1(&params.G1, this->kP1Str);
  FfElementObj y2(&params.fq, this->kY2Sha256Str);
  uint16_t counter = 0;

  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, &this->kMemberFValue, epid2params);
  THROW_ON_EPIDERR(Tpm2SetHashAlg(tpm, kSha256));
  THROW_ON_EPIDERR(Tpm2LoadExternal(tpm, &this->kMemberFValue));

  EXPECT_EQ(kEpidNoErr,
            Tpm2Commit(tpm, p1, this->kS2Sha256.data(), this->kS2Sha256.size(),
                       y2, k, l, e, &counter));
  THROW_ON_EPIDERR(Tpm2ReleaseCounter(tpm, counter));
  EcPointObj p1_exp_r(&params.G1), p2_exp_r(&params.G1);
  G1ElemStr p1_exp_r_str, p2_exp_r_str;
  EcPointObj p2(&params.G1, this->kP2Sha256Str);
  Prng the_same_prng;
  FfElementObj r(&params.fp);
  BigNumStr zero = {0};
  FpElemStr r_str = {0};
  THROW_ON_EPIDERR(
      FfGetRandom(params.fp, &zero, &Prng::Generate, &the_same_prng, r));
  THROW_ON_EPIDERR(WriteFfElement(params.fp, r, &r_str, sizeof(r_str)));
  THROW_ON_EPIDERR(EcExp(params.G1, p1, (BigNumStr const*)&r_str, p1_exp_r));
  THROW_ON_EPIDERR(
      WriteEcPoint(params.G1, p1_exp_r, &p1_exp_r_str, sizeof(p1_exp_r_str)));
  THROW_ON_EPIDERR(EcExp(params.G1, p2, (BigNumStr const*)&r_str, p2_exp_r));
  THROW_ON_EPIDERR(
      WriteEcPoint(params.G1, p2_exp_r, &p2_exp_r_str, sizeof(p2_exp_r_str)));

  G1ElemStr k_str, l_str, e_str;
  THROW_ON_EPIDERR(WriteEcPoint(params.G1, k, &k_str, sizeof(k_str)));
  THROW_ON_EPIDERR(WriteEcPoint(params.G1, l, &l_str, sizeof(l_str)));
  THROW_ON_EPIDERR(WriteEcPoint(params.G1, e, &e_str, sizeof(e_str)));
  EXPECT_EQ(this->kP2Sha256ExpF, k_str);
  EXPECT_EQ(p2_exp_r_str, l_str);
  EXPECT_EQ(p1_exp_r_str, e_str);
}

TEST_F(EpidTpm2Test, CommitComputeKLESha384) {
  // Testing step i and j of the "C.2.3 Tpm2Commit()"
  Epid20Params params;
  EcPointObj k(&params.G1), l(&params.G1), e(&params.G1);
  EcPointObj p1(&params.G1, this->kP1Str);
  FfElementObj y2(&params.fq, this->kY2Sha384Str);
  uint16_t counter = 0;

  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, &this->kMemberFValue, epid2params);
  THROW_ON_EPIDERR(Tpm2SetHashAlg(tpm, kSha384));
  THROW_ON_EPIDERR(Tpm2LoadExternal(tpm, &this->kMemberFValue));

  EXPECT_EQ(kEpidNoErr,
            Tpm2Commit(tpm, p1, this->kS2Sha384.data(), this->kS2Sha384.size(),
                       y2, k, l, e, &counter));
  THROW_ON_EPIDERR(Tpm2ReleaseCounter(tpm, counter));

  EcPointObj p1_exp_r(&params.G1), p2_exp_r(&params.G1);
  G1ElemStr p1_exp_r_str, p2_exp_r_str;
  EcPointObj p2(&params.G1, this->kP2Sha384Str);
  Prng the_same_prng;
  FfElementObj r(&params.fp);
  BigNumStr zero = {0};
  FpElemStr r_str = {0};
  THROW_ON_EPIDERR(
      FfGetRandom(params.fp, &zero, &Prng::Generate, &the_same_prng, r));
  THROW_ON_EPIDERR(WriteFfElement(params.fp, r, &r_str, sizeof(r_str)));
  THROW_ON_EPIDERR(EcExp(params.G1, p1, (BigNumStr const*)&r_str, p1_exp_r));
  THROW_ON_EPIDERR(
      WriteEcPoint(params.G1, p1_exp_r, &p1_exp_r_str, sizeof(p1_exp_r_str)));
  THROW_ON_EPIDERR(EcExp(params.G1, p2, (BigNumStr const*)&r_str, p2_exp_r));
  THROW_ON_EPIDERR(
      WriteEcPoint(params.G1, p2_exp_r, &p2_exp_r_str, sizeof(p2_exp_r_str)));

  G1ElemStr k_str, l_str, e_str;
  THROW_ON_EPIDERR(WriteEcPoint(params.G1, k, &k_str, sizeof(k_str)));
  THROW_ON_EPIDERR(WriteEcPoint(params.G1, l, &l_str, sizeof(l_str)));
  THROW_ON_EPIDERR(WriteEcPoint(params.G1, e, &e_str, sizeof(e_str)));
  EXPECT_EQ(this->kP2Sha384ExpF, k_str);
  EXPECT_EQ(p2_exp_r_str, l_str);
  EXPECT_EQ(p1_exp_r_str, e_str);
}

TEST_F(EpidTpm2Test, CommitComputeKLESha512) {
  // Testing step i and j of the "C.2.3 Tpm2Commit()"
  Epid20Params params;
  EcPointObj k(&params.G1), l(&params.G1), e(&params.G1);
  EcPointObj p1(&params.G1, this->kP1Str);
  FfElementObj y2(&params.fq, this->kY2Sha512Str);
  uint16_t counter = 0;

  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, &this->kMemberFValue, epid2params);
  THROW_ON_EPIDERR(Tpm2SetHashAlg(tpm, kSha512));
  THROW_ON_EPIDERR(Tpm2LoadExternal(tpm, &this->kMemberFValue));

  EXPECT_EQ(kEpidNoErr,
            Tpm2Commit(tpm, p1, this->kS2Sha512.data(), this->kS2Sha512.size(),
                       y2, k, l, e, &counter));
  THROW_ON_EPIDERR(Tpm2ReleaseCounter(tpm, counter));

  EcPointObj p1_exp_r(&params.G1), p2_exp_r(&params.G1);
  G1ElemStr p1_exp_r_str, p2_exp_r_str;
  EcPointObj p2(&params.G1, this->kP2Sha512Str);
  Prng the_same_prng;
  FfElementObj r(&params.fp);
  BigNumStr zero = {0};
  FpElemStr r_str = {0};
  THROW_ON_EPIDERR(
      FfGetRandom(params.fp, &zero, &Prng::Generate, &the_same_prng, r));
  THROW_ON_EPIDERR(WriteFfElement(params.fp, r, &r_str, sizeof(r_str)));
  THROW_ON_EPIDERR(EcExp(params.G1, p1, (BigNumStr const*)&r_str, p1_exp_r));
  THROW_ON_EPIDERR(
      WriteEcPoint(params.G1, p1_exp_r, &p1_exp_r_str, sizeof(p1_exp_r_str)));
  THROW_ON_EPIDERR(EcExp(params.G1, p2, (BigNumStr const*)&r_str, p2_exp_r));
  THROW_ON_EPIDERR(
      WriteEcPoint(params.G1, p2_exp_r, &p2_exp_r_str, sizeof(p2_exp_r_str)));

  G1ElemStr k_str, l_str, e_str;
  THROW_ON_EPIDERR(WriteEcPoint(params.G1, k, &k_str, sizeof(k_str)));
  THROW_ON_EPIDERR(WriteEcPoint(params.G1, l, &l_str, sizeof(l_str)));
  THROW_ON_EPIDERR(WriteEcPoint(params.G1, e, &e_str, sizeof(e_str)));
  EXPECT_EQ(this->kP2Sha512ExpF, k_str);
  EXPECT_EQ(p2_exp_r_str, l_str);
  EXPECT_EQ(p1_exp_r_str, e_str);
}

TEST_F(EpidTpm2Test, CommitComputeKLESha512256) {
  // Testing step i and j of the "C.2.3 Tpm2Commit()"
  Epid20Params params;
  EcPointObj k(&params.G1), l(&params.G1), e(&params.G1);
  EcPointObj p1(&params.G1, this->kP1Str);
  FfElementObj y2(&params.fq, this->kY2Sha512256Str);
  uint16_t counter = 0;

  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, &this->kMemberFValue, epid2params);
  THROW_ON_EPIDERR(Tpm2SetHashAlg(tpm, kSha512_256));
  THROW_ON_EPIDERR(Tpm2LoadExternal(tpm, &this->kMemberFValue));

  EXPECT_EQ(kEpidNoErr,
            Tpm2Commit(tpm, p1, this->kS2Sha512256.data(),
                       this->kS2Sha512256.size(), y2, k, l, e, &counter));
  THROW_ON_EPIDERR(Tpm2ReleaseCounter(tpm, counter));

  EcPointObj p1_exp_r(&params.G1), p2_exp_r(&params.G1);
  G1ElemStr p1_exp_r_str, p2_exp_r_str;
  EcPointObj p2(&params.G1, this->kP2Sha512256Str);
  Prng the_same_prng;
  FfElementObj r(&params.fp);
  BigNumStr zero = {0};
  FpElemStr r_str = {0};
  THROW_ON_EPIDERR(
      FfGetRandom(params.fp, &zero, &Prng::Generate, &the_same_prng, r));
  THROW_ON_EPIDERR(WriteFfElement(params.fp, r, &r_str, sizeof(r_str)));
  THROW_ON_EPIDERR(EcExp(params.G1, p1, (BigNumStr const*)&r_str, p1_exp_r));
  THROW_ON_EPIDERR(
      WriteEcPoint(params.G1, p1_exp_r, &p1_exp_r_str, sizeof(p1_exp_r_str)));
  THROW_ON_EPIDERR(EcExp(params.G1, p2, (BigNumStr const*)&r_str, p2_exp_r));
  THROW_ON_EPIDERR(
      WriteEcPoint(params.G1, p2_exp_r, &p2_exp_r_str, sizeof(p2_exp_r_str)));

  G1ElemStr k_str, l_str, e_str;
  THROW_ON_EPIDERR(WriteEcPoint(params.G1, k, &k_str, sizeof(k_str)));
  THROW_ON_EPIDERR(WriteEcPoint(params.G1, l, &l_str, sizeof(l_str)));
  THROW_ON_EPIDERR(WriteEcPoint(params.G1, e, &e_str, sizeof(e_str)));
  EXPECT_EQ(this->kP2Sha512256ExpF, k_str);
  EXPECT_EQ(p2_exp_r_str, l_str);
  EXPECT_EQ(p1_exp_r_str, e_str);
}

TEST_F(EpidTpm2Test, CommitComputeEOnly) {
  // Testing step j excuding i of the "C.2.3 Tpm2Commit()"
  Epid20Params params;
  EcPointObj k(&params.G1), l(&params.G1), e(&params.G1);
  EcPointObj p1(&params.G1, this->kP1Str);
  FfElementObj y2(&params.fq, this->kY2Sha512Str);
  uint16_t counter = 0;

  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, &this->kMemberFValue, epid2params);
  THROW_ON_EPIDERR(Tpm2SetHashAlg(tpm, kSha512));
  THROW_ON_EPIDERR(Tpm2LoadExternal(tpm, &this->kMemberFValue));

  EXPECT_EQ(kEpidNoErr,
            Tpm2Commit(tpm, p1, nullptr, 0, nullptr, k, l, e, &counter));
  THROW_ON_EPIDERR(Tpm2ReleaseCounter(tpm, counter));

  EcPointObj p1_exp_r(&params.G1);
  G1ElemStr p1_exp_r_str;
  Prng the_same_prng;
  FfElementObj r(&params.fp);
  BigNumStr zero = {0};
  FpElemStr r_str = {0};
  THROW_ON_EPIDERR(
      FfGetRandom(params.fp, &zero, &Prng::Generate, &the_same_prng, r));
  THROW_ON_EPIDERR(WriteFfElement(params.fp, r, &r_str, sizeof(r_str)));
  THROW_ON_EPIDERR(EcExp(params.G1, p1, (BigNumStr const*)&r_str, p1_exp_r));
  THROW_ON_EPIDERR(
      WriteEcPoint(params.G1, p1_exp_r, &p1_exp_r_str, sizeof(p1_exp_r_str)));

  G1ElemStr e_str;
  THROW_ON_EPIDERR(WriteEcPoint(params.G1, e, &e_str, sizeof(e_str)));
  EXPECT_EQ(p1_exp_r_str, e_str);
}

TEST_F(EpidTpm2Test, CommitComputeEOnlyWithDefaultP1) {
  // Testing step k excuding i of the "C.2.3 Tpm2Commit()"
  Epid20Params params;
  EcPointObj e(&params.G1);
  EcPointObj p1(&params.G1, this->kP1Str);
  FfElementObj y2(&params.fq, this->kY2Sha512Str);
  uint16_t counter = 0;

  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, &this->kMemberFValue, epid2params);
  THROW_ON_EPIDERR(Tpm2SetHashAlg(tpm, kSha512));
  THROW_ON_EPIDERR(Tpm2LoadExternal(tpm, &this->kMemberFValue));

  EXPECT_EQ(kEpidNoErr, Tpm2Commit(tpm, nullptr, nullptr, 0, nullptr, nullptr,
                                   nullptr, e, &counter));
  THROW_ON_EPIDERR(Tpm2ReleaseCounter(tpm, counter));

  EcPointObj g1(&params.G1, this->kg1Str);
  EcPointObj g1_exp_r(&params.G1);
  G1ElemStr g1_exp_r_str;
  Prng the_same_prng;
  FfElementObj r(&params.fp);
  BigNumStr zero = {0};
  FpElemStr r_str = {0};
  THROW_ON_EPIDERR(
      FfGetRandom(params.fp, &zero, &Prng::Generate, &the_same_prng, r));
  THROW_ON_EPIDERR(WriteFfElement(params.fp, r, &r_str, sizeof(r_str)));
  THROW_ON_EPIDERR(EcExp(params.G1, g1, (BigNumStr const*)&r_str, g1_exp_r));
  THROW_ON_EPIDERR(
      WriteEcPoint(params.G1, g1_exp_r, &g1_exp_r_str, sizeof(g1_exp_r_str)));

  G1ElemStr e_str;
  THROW_ON_EPIDERR(WriteEcPoint(params.G1, e, &e_str, sizeof(e_str)));
  EXPECT_EQ(g1_exp_r_str, e_str);
}

}  // namespace
