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
/// TPM Sign unit tests.
/*! \file */
#include <climits>

#include "gtest/gtest.h"

#include "epid/common-testhelper/epid2params_wrapper-testhelper.h"
#include "epid/common-testhelper/epid_params-testhelper.h"
#include "epid/common-testhelper/errors-testhelper.h"
#include "epid/common-testhelper/prng-testhelper.h"
#include "epid/member/tpm2/unittests/tpm2-testhelper.h"

extern "C" {
#include "epid/common/src/hashsize.h"
#include "epid/common/src/memory.h"
#include "epid/member/tpm2/commit.h"
#include "epid/member/tpm2/load_external.h"
#include "epid/member/tpm2/sign.h"
}

namespace {
//////////////////////////////////////////////////////////////////////////
// Tpm2Sign Tests
// Verify signature computed by TPM ECDAA scheme:
// sign_k ?= digest mod p
// point^sign_s ?= random_exp * private_exp^sign_k
bool IsSignatureValid(void const* digest, size_t digest_len,
                      FfElement const* sign_k, FfElement const* sign_s,
                      EcPoint const* point, EcPoint const* private_exp,
                      EcPoint const* random_exp, HashAlg hash_alg) {
  (void)hash_alg;

  Epid20Params params;

  BigNumObj digest_bn(digest_len);
  THROW_ON_EPIDERR(ReadBigNum(digest, digest_len, digest_bn));
  FfElementObj t(&params.fp);
  THROW_ON_EPIDERR(InitFfElementFromBn(params.fp, digest_bn, t));
  FpElemStr t_str, sign_k_str;
  THROW_ON_EPIDERR(WriteFfElement(params.fp, t, &t_str, sizeof(t_str)));
  THROW_ON_EPIDERR(
      WriteFfElement(params.fp, sign_k, &sign_k_str, sizeof(sign_k_str)));
  if (!(t_str == sign_k_str)) return false;

  BigNumStr exp;
  // v1 = p2^s
  EcPointObj v1(&params.G1);
  THROW_ON_EPIDERR(WriteFfElement(params.fp, sign_s, &exp, sizeof(exp)));
  THROW_ON_EPIDERR(EcExp(params.G1, point, &exp, v1));
  // v2 = k^sign_k
  EcPointObj v2(&params.G1);
  THROW_ON_EPIDERR(WriteFfElement(params.fp, sign_k, &exp, sizeof(exp)));
  THROW_ON_EPIDERR(EcExp(params.G1, private_exp, &exp, v2));
  // v2 = l * k^digest
  THROW_ON_EPIDERR(EcMul(params.G1, random_exp, v2, v2));
  // v1 ?= v2
  G1ElemStr v1_str, v2_str;
  THROW_ON_EPIDERR(WriteEcPoint(params.G1, v1, &v1_str, sizeof(v1_str)));
  THROW_ON_EPIDERR(WriteEcPoint(params.G1, v2, &v2_str, sizeof(v2_str)));
  return v1_str == v2_str;
}

TEST_F(EpidTpm2Test, SignProducesValidSignature) {
  Epid20Params params;

  // create TPM context
  Prng my_prng;
  Epid2ParamsObj epid2params;
  FpElemStr f = this->kMemberFValue;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, &f, epid2params);
  THROW_ON_EPIDERR(Tpm2SetHashAlg(tpm, kSha256));
  // load f value
  EXPECT_EQ(kEpidNoErr, Tpm2LoadExternal(tpm, &f));

  // commit(P1=p2, P2=p2) => k = p2^f, l = p2^r, e = p2^r
  FfElementObj y2(&params.fq, this->kY2Sha256Str);
  EcPointObj p2(&params.G1, kP2Sha256Str);
  EcPointObj p2_exp_f(&params.G1, kP2Sha256ExpF);

  EcPointObj k(&params.G1), l(&params.G1), e(&params.G1);
  uint16_t counter = 0;
  EXPECT_EQ(kEpidNoErr,
            Tpm2Commit(tpm, p2, this->kS2Sha256.data(), this->kS2Sha256.size(),
                       y2, k, l, e, &counter));

  // sign(digest) => sign_k = sign_k, sign_s = r + c * f,
  //   where c = H(sign_k||digest)
  FfElementObj sign_k(&params.fp), sign_s(&params.fp);
  EXPECT_EQ(kEpidNoErr,
            Tpm2Sign(tpm, this->kDigestSha256, sizeof(this->kDigestSha256),
                     counter, sign_k, sign_s));

  EXPECT_TRUE(IsSignatureValid(this->kDigestSha256, sizeof(this->kDigestSha256),
                               sign_k, sign_s, p2, k, l, kSha256));
}

TEST_F(EpidTpm2Test, SignProducesValidSignatureTwoTimes) {
  Epid20Params params;

  // create TPM context
  Prng my_prng;
  Epid2ParamsObj epid2params;
  FpElemStr f = this->kMemberFValue;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, &f, epid2params);
  THROW_ON_EPIDERR(Tpm2SetHashAlg(tpm, kSha256));
  // load f value
  EXPECT_EQ(kEpidNoErr, Tpm2LoadExternal(tpm, &f));

  // commit(P1=p2, P2=p2) => k = p2^f, l = p2^r, e = p2^r
  FfElementObj y2(&params.fq, this->kY2Sha256Str);
  EcPointObj p2(&params.G1, kP2Sha256Str);
  EcPointObj p2_exp_f(&params.G1, kP2Sha256ExpF);

  EcPointObj k1(&params.G1), l1(&params.G1), e1(&params.G1);
  EcPointObj k2(&params.G1), l2(&params.G1), e2(&params.G1);
  uint16_t ctr1 = 0, ctr2 = 0;
  EXPECT_EQ(kEpidNoErr,
            Tpm2Commit(tpm, p2, this->kS2Sha256.data(), this->kS2Sha256.size(),
                       y2, k1, l1, e1, &ctr1));
  EXPECT_EQ(kEpidNoErr,
            Tpm2Commit(tpm, p2, this->kS2Sha256.data(), this->kS2Sha256.size(),
                       y2, k2, l2, e2, &ctr2));

  // sign(digest) => sign_k = sign_k, sign_s = r + c * f,
  //   where c = H(sign_k||digest)
  FfElementObj sign_k1(&params.fp), sign_s1(&params.fp);
  FfElementObj sign_k2(&params.fp), sign_s2(&params.fp);
  EXPECT_EQ(kEpidNoErr,
            Tpm2Sign(tpm, this->kDigestSha256, sizeof(this->kDigestSha256),
                     ctr1, sign_k1, sign_s1));
  EXPECT_EQ(kEpidNoErr,
            Tpm2Sign(tpm, this->kDigestSha256, sizeof(this->kDigestSha256),
                     ctr2, sign_k2, sign_s2));

  EXPECT_TRUE(IsSignatureValid(this->kDigestSha256, sizeof(this->kDigestSha256),
                               sign_k1, sign_s1, p2, k1, l1, kSha256));
  EXPECT_TRUE(IsSignatureValid(this->kDigestSha256, sizeof(this->kDigestSha256),
                               sign_k2, sign_s2, p2, k2, l2, kSha256));
}

TEST_F(EpidTpm2Test, SignFailsGivenNullParameters) {
  Epid20Params params;
  EcPointObj k(&params.G1), l(&params.G1), e(&params.G1);
  FfElementObj sig_k(&params.fp), sig_s(&params.fp);
  uint16_t counter = 0;

  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, &this->kMemberFValue, epid2params);
  THROW_ON_EPIDERR(Tpm2SetHashAlg(tpm, kSha256));
  THROW_ON_EPIDERR(Tpm2LoadExternal(tpm, &this->kMemberFValue));
  THROW_ON_EPIDERR(
      Tpm2Commit(tpm, nullptr, nullptr, 0, nullptr, k, l, e, &counter));

  EXPECT_EQ(kEpidBadArgErr,
            Tpm2Sign(nullptr, this->kDigestSha256, sizeof(this->kDigestSha256),
                     counter, sig_k, sig_s));
  EXPECT_EQ(kEpidBadArgErr, Tpm2Sign(tpm, nullptr, sizeof(this->kDigestSha256),
                                     counter, sig_k, sig_s));
  EXPECT_EQ(kEpidBadArgErr,
            Tpm2Sign(tpm, this->kDigestSha256, sizeof(this->kDigestSha256),
                     counter, sig_k, nullptr));
}

TEST_F(EpidTpm2Test, SignFailsGivenInvalidDigestLen) {
  Epid20Params params;
  EcPointObj k(&params.G1), l(&params.G1), e(&params.G1);
  FfElementObj sig_k(&params.fp), sig_s(&params.fp);
  uint16_t counter = 0;

  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, &this->kMemberFValue, epid2params);
  THROW_ON_EPIDERR(Tpm2SetHashAlg(tpm, kSha256));
  THROW_ON_EPIDERR(Tpm2LoadExternal(tpm, &this->kMemberFValue));
  THROW_ON_EPIDERR(
      Tpm2Commit(tpm, nullptr, nullptr, 0, nullptr, k, l, e, &counter));

  uint8_t digest[EPID_SHA256_DIGEST_BITSIZE / CHAR_BIT + 1] = {0};
  EXPECT_EQ(kEpidBadArgErr, Tpm2Sign(tpm, digest, 0, counter, sig_k, sig_s));
  EXPECT_EQ(kEpidBadArgErr,
            Tpm2Sign(tpm, digest, EPID_SHA256_DIGEST_BITSIZE / CHAR_BIT + 1,
                     counter, sig_k, sig_s));
  EXPECT_EQ(kEpidBadArgErr,
            Tpm2Sign(tpm, digest, EPID_SHA256_DIGEST_BITSIZE / CHAR_BIT - 1,
                     counter, sig_k, sig_s));
}

TEST_F(EpidTpm2Test, SignFailsGivenUnrecognizedCounter) {
  Epid20Params params;
  EcPointObj k(&params.G1), l(&params.G1), e(&params.G1);
  FfElementObj sig_k(&params.fp), sig_s(&params.fp);
  uint16_t counter = 0;
  uint16_t zero = 0;
  uint16_t one = 1;
  uint16_t minus_one = (uint16_t)-1;

  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, &this->kMemberFValue, epid2params);
  THROW_ON_EPIDERR(Tpm2SetHashAlg(tpm, kSha256));
  THROW_ON_EPIDERR(Tpm2LoadExternal(tpm, &this->kMemberFValue));

  EXPECT_EQ(kEpidBadArgErr,
            Tpm2Sign(tpm, this->kDigestSha256, sizeof(this->kDigestSha256),
                     zero, sig_k, sig_s));
  EXPECT_EQ(kEpidBadArgErr,
            Tpm2Sign(tpm, this->kDigestSha256, sizeof(this->kDigestSha256), one,
                     sig_k, sig_s));
  EXPECT_EQ(kEpidBadArgErr,
            Tpm2Sign(tpm, this->kDigestSha256, sizeof(this->kDigestSha256),
                     minus_one, sig_k, sig_s));

  THROW_ON_EPIDERR(
      Tpm2Commit(tpm, nullptr, nullptr, 0, nullptr, k, l, e, &counter));

  uint16_t counter_plus_1 = counter + 1;
  EXPECT_EQ(kEpidBadArgErr,
            Tpm2Sign(tpm, this->kDigestSha256, sizeof(this->kDigestSha256),
                     counter_plus_1, sig_k, sig_s));
  THROW_ON_EPIDERR(Tpm2ReleaseCounter(tpm, counter));
}

TEST_F(EpidTpm2Test, SignFailsGivenPreviouslyUsedCounter) {
  Epid20Params params;
  EcPointObj k(&params.G1), l(&params.G1), e(&params.G1);
  FfElementObj sig_k(&params.fp), sig_s(&params.fp);
  uint16_t counter = 0;

  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, &this->kMemberFValue, epid2params);
  THROW_ON_EPIDERR(Tpm2SetHashAlg(tpm, kSha256));
  THROW_ON_EPIDERR(Tpm2LoadExternal(tpm, &this->kMemberFValue));
  THROW_ON_EPIDERR(
      Tpm2Commit(tpm, nullptr, nullptr, 0, nullptr, k, l, e, &counter));

  EXPECT_EQ(kEpidNoErr,
            Tpm2Sign(tpm, this->kDigestSha256, sizeof(this->kDigestSha256),
                     counter, sig_k, sig_s));
  EXPECT_EQ(kEpidBadArgErr,
            Tpm2Sign(tpm, this->kDigestSha256, sizeof(this->kDigestSha256),
                     counter, sig_k, sig_s));
}

TEST_F(EpidTpm2Test, SignFailsIfKeyNotSet) {
  Epid20Params params;
  EcPointObj k(&params.G1), l(&params.G1), e(&params.G1);
  FfElementObj sig_k(&params.fp), sig_s(&params.fp);
  uint16_t counter = 0;

  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);

  EXPECT_EQ(kEpidBadArgErr,
            Tpm2Sign(tpm, this->kDigestSha256, sizeof(this->kDigestSha256),
                     counter, sig_k, sig_s));
}

//////////////////////////////////////////////////////////////////////////
// Tpm2ReleaseCounter Tests
TEST_F(EpidTpm2Test, ReleaseCounterFailsGivenNullPtr) {
  // create TPM context
  Prng my_prng;
  Epid2ParamsObj epid2params;
  FpElemStr f = this->kMemberFValue;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, &f, epid2params);
  uint16_t ctr = 0;

  EXPECT_EQ(kEpidBadArgErr, Tpm2ReleaseCounter(nullptr, ctr));
}
TEST_F(EpidTpm2Test, ReleaseCounterSuccessfullyReleasesCounter) {
  Epid20Params params;

  // create TPM context
  Prng my_prng;
  Epid2ParamsObj epid2params;
  FpElemStr f = this->kMemberFValue;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, &f, epid2params);
  THROW_ON_EPIDERR(Tpm2SetHashAlg(tpm, kSha256));
  // load f value
  EXPECT_EQ(kEpidNoErr, Tpm2LoadExternal(tpm, &f));

  // commit(P1=p2, P2=p2) => k = p2^f, l = p2^r, e = p2^r
  FfElementObj y2(&params.fq, this->kY2Sha256Str);
  EcPointObj p2(&params.G1, kP2Sha256Str);
  EcPointObj p2_exp_f(&params.G1, kP2Sha256ExpF);

  EcPointObj k(&params.G1), l(&params.G1), e(&params.G1);
  uint16_t counter = 0;
  EXPECT_EQ(kEpidNoErr,
            Tpm2Commit(tpm, p2, this->kS2Sha256.data(), this->kS2Sha256.size(),
                       y2, k, l, e, &counter));
  EXPECT_EQ(kEpidNoErr, Tpm2ReleaseCounter(tpm, counter));

  // sign(digest) => sign_k = sign_k, sign_s = r + c * f,
  //   where c = H(sign_k||digest)
  FfElementObj sign_k(&params.fp), sign_s(&params.fp);
  EXPECT_EQ(kEpidBadArgErr,
            Tpm2Sign(tpm, this->kDigestSha256, sizeof(this->kDigestSha256),
                     counter, sign_k, sign_s));
}

}  // namespace
