/*############################################################################
  # Copyright 2016-2017 Intel Corporation
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
/// SignBasic unit tests.
/*! \file */

#include <cstring>
#include "epid/common-testhelper/epid_gtest-testhelper.h"
#include "gtest/gtest.h"

extern "C" {
#include "epid/member/api.h"
#include "epid/member/src/signbasic.h"
#include "epid/verifier/api.h"
}

#include "epid/common-testhelper/errors-testhelper.h"
#include "epid/common-testhelper/prng-testhelper.h"
#include "epid/common-testhelper/verifier_wrapper-testhelper.h"
#include "epid/member/unittests/member-testhelper.h"

bool operator==(BigNumStr const& lhs, BigNumStr const& rhs) {
  return 0 == std::memcmp(&lhs, &rhs, sizeof(lhs));
}
namespace {

/// Count of elements in array
#define COUNT_OF(A) (sizeof(A) / sizeof((A)[0]))

/////////////////////////////////////////////////////////////////////////
// Simple error cases
TEST_F(EpidMemberTest, SignBasicFailsGivenNullParameters) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  auto& bsn = this->kBsn0;
  BasicSignature basic_sig;
  EXPECT_EQ(kEpidBadArgErr,
            EpidSignBasic(nullptr, msg.data(), msg.size(), bsn.data(),
                          bsn.size(), &basic_sig, nullptr));
  EXPECT_EQ(kEpidBadArgErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn.data(),
                          bsn.size(), nullptr, nullptr));
  EXPECT_EQ(kEpidBadArgErr,
            EpidSignBasic(member, nullptr, msg.size(), bsn.data(), bsn.size(),
                          &basic_sig, nullptr));
  EXPECT_EQ(kEpidBadArgErr,
            EpidSignBasic(member, msg.data(), msg.size(), nullptr, bsn.size(),
                          &basic_sig, nullptr));
}
TEST_F(EpidMemberTest, SignBasicFailsGivenNullBasenameAndNullRandomBasename) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  BasicSignature basic_sig;
  EXPECT_EQ(kEpidBadArgErr, EpidSignBasic(member, msg.data(), msg.size(),
                                          nullptr, 0, &basic_sig, nullptr));
}
TEST_F(EpidMemberTest, SignBasicDoesNotComputeRandomBasenameGivenBasename) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  auto& bsn = this->kBsn0;
  BigNumStr rnd_bsn = {0};
  BigNumStr zero = {0};
  BasicSignature basic_sig;
  THROW_ON_EPIDERR(EpidRegisterBasename(member, bsn.data(), bsn.size()));
  EXPECT_EQ(kEpidNoErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn.data(),
                          bsn.size(), &basic_sig, &rnd_bsn));
  EXPECT_EQ(zero, rnd_bsn);
  // verify basic signature
  VerifierCtxObj ctx(this->kGroupPublicKey);
  THROW_ON_EPIDERR(EpidVerifierSetBasename(ctx, bsn.data(), bsn.size()));
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}
TEST_F(EpidMemberTest, SignBasicFailsForBasenameWithoutRegisteredBasenames) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  auto& bsn = this->kBsn0;
  BasicSignature basic_sig;
  EXPECT_EQ(kEpidBadArgErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn.data(),
                          bsn.size(), &basic_sig, nullptr));
}
TEST_F(EpidMemberTest, SignBasicFailsForUnregisteredBasename) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  auto& bsn0 = this->kBsn0;
  auto& bsn1 = this->kBsn1;
  THROW_ON_EPIDERR(EpidRegisterBasename(member, bsn0.data(), bsn0.size()));
  BasicSignature basic_sig;
  EXPECT_EQ(kEpidBadArgErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn1.data(),
                          bsn1.size(), &basic_sig, nullptr));
}
/////////////////////////////////////////////////////////////////////////
// Anonymity
TEST_F(EpidMemberTest, BasicSignaturesOfSameMessageAreDifferent) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  BasicSignature basic_sig1 = {0};
  BasicSignature basic_sig2 = {0};
  BigNumStr rnd_bsn = {0};
  EXPECT_EQ(kEpidNoErr, EpidSignBasic(member, msg.data(), msg.size(), nullptr,
                                      0, &basic_sig1, &rnd_bsn));
  EXPECT_EQ(kEpidNoErr, EpidSignBasic(member, msg.data(), msg.size(), nullptr,
                                      0, &basic_sig2, &rnd_bsn));
  EXPECT_NE(0, memcmp(&basic_sig1, &basic_sig2, sizeof(BasicSignature)));
}
TEST_F(EpidMemberTest,
       BasicSignaturesOfSameMessageWithSameBasenameAreDifferent) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  auto& bsn = this->kBsn0;
  BasicSignature basic_sig1;
  BasicSignature basic_sig2;
  THROW_ON_EPIDERR(EpidRegisterBasename(member, bsn.data(), bsn.size()));
  EXPECT_EQ(kEpidNoErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn.data(),
                          bsn.size(), &basic_sig1, nullptr));
  EXPECT_EQ(kEpidNoErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn.data(),
                          bsn.size(), &basic_sig2, nullptr));
  EXPECT_NE(0, memcmp(&basic_sig1, &basic_sig2, sizeof(BasicSignature)));
}
/////////////////////////////////////////////////////////////////////////
// Variable basename
TEST_F(EpidMemberTest, SignBasicSucceedsUsingRandomBase) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  BasicSignature basic_sig;
  BigNumStr rnd_bsn = {0};
  BigNumStr zero = {0};
  EXPECT_EQ(kEpidNoErr, EpidSignBasic(member, msg.data(), msg.size(), nullptr,
                                      0, &basic_sig, &rnd_bsn));
  EXPECT_NE(0, memcmp(&rnd_bsn, &zero, sizeof(BigNumStr)));
  // verify basic signature
  VerifierCtxObj ctx(this->kGroupPublicKey);
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}
TEST_F(EpidMemberTest,
       PROTECTED_SignBasicSucceedsUsingRandomBaseWithCredential_EPS0) {
  Prng my_prng;
  MemberCtxObj member(
      this->kEps0GroupPublicKey,
      *(MembershipCredential const*)&this->kEps0MemberPrivateKey,
      &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  BasicSignature basic_sig;
  BigNumStr rnd_bsn = {0};
  BigNumStr zero = {0};
  EXPECT_EQ(kEpidNoErr, EpidSignBasic(member, msg.data(), msg.size(), nullptr,
                                      0, &basic_sig, &rnd_bsn));
  EXPECT_NE(0, memcmp(&rnd_bsn, &zero, sizeof(BigNumStr)));
  // verify basic signature
  VerifierCtxObj ctx(this->kEps0GroupPublicKey);
  THROW_ON_EPIDERR(EpidVerifierSetHashAlg(ctx, kSha256));
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}
TEST_F(EpidMemberTest, SignBasicSucceedsUsingBasename) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  auto& bsn = this->kBsn0;
  BasicSignature basic_sig;
  THROW_ON_EPIDERR(EpidRegisterBasename(member, bsn.data(), bsn.size()));
  EXPECT_EQ(kEpidNoErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn.data(),
                          bsn.size(), &basic_sig, nullptr));
  // verify basic signature
  VerifierCtxObj ctx(this->kGroupPublicKey);
  THROW_ON_EPIDERR(EpidVerifierSetBasename(ctx, bsn.data(), bsn.size()));
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}

TEST_F(EpidMemberTest,
       PROTECTED_SignBasicSucceedsUsingBasenameWithCredential_EPS0) {
  Prng my_prng;
  MemberCtxObj member(
      this->kEps0GroupPublicKey,
      *(MembershipCredential const*)&this->kEps0MemberPrivateKey,
      &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  auto& bsn = this->kBsn0;
  BasicSignature basic_sig;
  THROW_ON_EPIDERR(EpidRegisterBasename(member, bsn.data(), bsn.size()));
  EXPECT_EQ(kEpidNoErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn.data(),
                          bsn.size(), &basic_sig, nullptr));
  // verify basic signature
  VerifierCtxObj ctx(this->kEps0GroupPublicKey);
  THROW_ON_EPIDERR(EpidVerifierSetBasename(ctx, bsn.data(), bsn.size()));
  THROW_ON_EPIDERR(EpidVerifierSetHashAlg(ctx, kSha256));
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}

TEST_F(EpidMemberTest, SignBasicSucceedsUsingBasenameUsingIKGFData) {
  Prng my_prng;
  GroupPubKey grp_public_key = *reinterpret_cast<const GroupPubKey*>(
      this->kGroupPublicKeyDataIkgf.data());
  PrivKey mbr_private_key =
      *reinterpret_cast<const PrivKey*>(this->kMemberPrivateKeyDataIkgf.data());
  MemberCtxObj member(grp_public_key, mbr_private_key, &Prng::Generate,
                      &my_prng);
  auto& msg = this->kMsg0;
  auto& bsn = this->kBsn0;
  BasicSignature basic_sig;
  THROW_ON_EPIDERR(EpidRegisterBasename(member, bsn.data(), bsn.size()));
  EXPECT_EQ(kEpidNoErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn.data(),
                          bsn.size(), &basic_sig, nullptr));
  // verify basic signature
  VerifierCtxObj ctx(grp_public_key);
  THROW_ON_EPIDERR(EpidVerifierSetBasename(ctx, bsn.data(), bsn.size()));
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}
TEST_F(EpidMemberTest,
       SignBasicSucceedsUsingRandomBaseWithRegisteredBasenames) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  auto& bsn = this->kBsn0;
  THROW_ON_EPIDERR(EpidRegisterBasename(member, bsn.data(), bsn.size()));
  BasicSignature basic_sig;
  BigNumStr rnd_bsn = {0};
  EXPECT_EQ(kEpidNoErr, EpidSignBasic(member, msg.data(), msg.size(), nullptr,
                                      0, &basic_sig, &rnd_bsn));
  // verify basic signature
  VerifierCtxObj ctx(this->kGroupPublicKey);
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}
TEST_F(EpidMemberTest,
       SignBasicSucceedsUsingRandomBaseWithoutRegisteredBasenames) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  BasicSignature basic_sig;
  BigNumStr rnd_bsn = {0};
  EXPECT_EQ(kEpidNoErr, EpidSignBasic(member, msg.data(), msg.size(), nullptr,
                                      0, &basic_sig, &rnd_bsn));
  // verify basic signature
  VerifierCtxObj ctx(this->kGroupPublicKey);
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}
#ifndef TPM_TSS
TEST_F(EpidMemberTest, SignBasicSucceedsUsingBsnContainingAllPossibleBytes) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  auto& bsn = this->kData_0_255;
  THROW_ON_EPIDERR(EpidRegisterBasename(member, bsn.data(), bsn.size()));
  BasicSignature basic_sig;
  EXPECT_EQ(kEpidNoErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn.data(),
                          bsn.size(), &basic_sig, nullptr));
  VerifierCtxObj ctx(this->kGroupPublicKey);
  THROW_ON_EPIDERR(EpidVerifierSetBasename(ctx, bsn.data(), bsn.size()));
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}
#endif

TEST_F(EpidMemberTest,
       PROTECTED_SignBasicSucceedsAllPossibleBytesForCredential_EPS0) {
  Prng my_prng;
  MemberCtxObj member(
      this->kEps0GroupPublicKey,
      *(MembershipCredential const*)&this->kEps0MemberPrivateKey,
      &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  auto& bsn = this->kData_0_255;
  // 0 - 123
  THROW_ON_EPIDERR(EpidRegisterBasename(member, bsn.data(), 124));
  BasicSignature basic_sig;
  EXPECT_EQ(kEpidNoErr, EpidSignBasic(member, msg.data(), msg.size(),
                                      bsn.data(), 124, &basic_sig, nullptr));
  VerifierCtxObj ctx1(this->kEps0GroupPublicKey);
  THROW_ON_EPIDERR(EpidVerifierSetBasename(ctx1, bsn.data(), 124));
  THROW_ON_EPIDERR(EpidVerifierSetHashAlg(ctx1, kSha256));
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx1, &basic_sig, msg.data(), msg.size()));

  // 124 - 247
  THROW_ON_EPIDERR(EpidRegisterBasename(member, bsn.data() + 124, 124));
  EXPECT_EQ(kEpidNoErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn.data() + 124, 124,
                          &basic_sig, nullptr));
  VerifierCtxObj ctx2(this->kEps0GroupPublicKey);
  THROW_ON_EPIDERR(EpidVerifierSetBasename(ctx2, bsn.data() + 124, 124));
  THROW_ON_EPIDERR(EpidVerifierSetHashAlg(ctx2, kSha256));
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx2, &basic_sig, msg.data(), msg.size()));

  // 248 - 255
  THROW_ON_EPIDERR(
      EpidRegisterBasename(member, bsn.data() + 124 * 2, 256 - 124 * 2));
  EXPECT_EQ(kEpidNoErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn.data() + 124 * 2,
                          256 - 124 * 2, &basic_sig, nullptr));
  VerifierCtxObj ctx3(this->kEps0GroupPublicKey);
  THROW_ON_EPIDERR(
      EpidVerifierSetBasename(ctx3, bsn.data() + 124 * 2, 256 - 124 * 2));
  THROW_ON_EPIDERR(EpidVerifierSetHashAlg(ctx3, kSha256));
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx3, &basic_sig, msg.data(), msg.size()));
}
/////////////////////////////////////////////////////////////////////////
// Variable hash alg
TEST_F(EpidMemberTest, SignBasicSucceedsUsingSha256HashAlg) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey, kSha256,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  auto& bsn = this->kBsn0;
  THROW_ON_EPIDERR(EpidRegisterBasename(member, bsn.data(), bsn.size()));
  BasicSignature basic_sig;
  EXPECT_EQ(kEpidNoErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn.data(),
                          bsn.size(), &basic_sig, nullptr));
  // verify basic signature
  VerifierCtxObj ctx(this->kGroupPublicKey);
  THROW_ON_EPIDERR(EpidVerifierSetHashAlg(ctx, kSha256));
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}
TEST_F(EpidMemberTest,
       PROTECTED_SignBasicSucceedsUsingSha256HashAlgWithCredential_EPS0) {
  Prng my_prng;
  MemberCtxObj member(
      this->kEps0GroupPublicKey,
      *(MembershipCredential const*)&this->kEps0MemberPrivateKey, kSha256,
      &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  auto& bsn = this->kBsn0;
  THROW_ON_EPIDERR(EpidRegisterBasename(member, bsn.data(), bsn.size()));
  BasicSignature basic_sig;
  EXPECT_EQ(kEpidNoErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn.data(),
                          bsn.size(), &basic_sig, nullptr));
  // verify basic signature
  VerifierCtxObj ctx(this->kEps0GroupPublicKey);
  THROW_ON_EPIDERR(EpidVerifierSetHashAlg(ctx, kSha256));
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}
TEST_F(EpidMemberTest, SignBasicSucceedsUsingSha384HashAlg) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey, kSha384,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  auto& bsn = this->kBsn0;
  THROW_ON_EPIDERR(EpidRegisterBasename(member, bsn.data(), bsn.size()));
  BasicSignature basic_sig;
  EXPECT_EQ(kEpidNoErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn.data(),
                          bsn.size(), &basic_sig, nullptr));
  // verify basic signature
  VerifierCtxObj ctx(this->kGroupPublicKey);
  THROW_ON_EPIDERR(EpidVerifierSetHashAlg(ctx, kSha384));
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}
TEST_F(EpidMemberTest, SignBasicSucceedsUsingSha512HashAlg) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey, kSha512,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  auto& bsn = this->kBsn0;
  THROW_ON_EPIDERR(EpidRegisterBasename(member, bsn.data(), bsn.size()));
  BasicSignature basic_sig;
  EXPECT_EQ(kEpidNoErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn.data(),
                          bsn.size(), &basic_sig, nullptr));
  // verify basic signature
  VerifierCtxObj ctx(this->kGroupPublicKey);
  THROW_ON_EPIDERR(EpidVerifierSetHashAlg(ctx, kSha512));
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}
TEST_F(EpidMemberTest, SignBasicSucceedsUsingSha512256HashAlg) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      kSha512_256, this->kMemberPrecomp, &Prng::Generate,
                      &my_prng);
  auto& msg = this->kMsg0;
  auto& bsn = this->kBsn0;
  THROW_ON_EPIDERR(EpidRegisterBasename(member, bsn.data(), bsn.size()));
  BasicSignature basic_sig;
  EXPECT_EQ(kEpidNoErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn.data(),
                          bsn.size(), &basic_sig, nullptr));
  // verify basic signature
  VerifierCtxObj ctx(this->kGroupPublicKey);
  THROW_ON_EPIDERR(EpidVerifierSetHashAlg(ctx, kSha512_256));
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}
/////////////////////////////////////////////////////////////////////////
TEST_F(EpidMemberTest, SignBasicConsumesPrecomputedSignatures) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  THROW_ON_EPIDERR(EpidAddPreSigs(member, 3));
  auto& msg = this->kMsg0;
  BasicSignature basic_sig;
  auto& bsn = this->kBsn0;
  THROW_ON_EPIDERR(EpidRegisterBasename(member, bsn.data(), bsn.size()));
  // use 1 precomputed signature
  ASSERT_EQ(kEpidNoErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn.data(),
                          bsn.size(), &basic_sig, nullptr));
  EXPECT_EQ((size_t)2, EpidGetNumPreSigs(member));
}
TEST_F(EpidMemberTest, SignBasicSucceedsWithPrecomputedSignatures) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  BasicSignature basic_sig;
  BigNumStr rnd_bsn = {0};
  EXPECT_EQ(kEpidNoErr, EpidSignBasic(member, msg.data(), msg.size(), nullptr,
                                      0, &basic_sig, &rnd_bsn));
  // verify basic signature
  VerifierCtxObj ctx(this->kGroupPublicKey);
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}
TEST_F(EpidMemberTest,
       PROTECTED_SignBasicSucceedsUsingPrecompSigWithCredential_EPS0) {
  Prng my_prng;
  MemberCtxObj member(
      this->kEps0GroupPublicKey,
      *(MembershipCredential const*)&this->kEps0MemberPrivateKey,
      &Prng::Generate, &my_prng);

  THROW_ON_EPIDERR(EpidAddPreSigs(member, 1));

  auto& msg = this->kMsg0;

  BasicSignature basic_sig;
  BigNumStr rnd_bsn = {0};

  EXPECT_EQ(kEpidNoErr, EpidSignBasic(member, msg.data(), msg.size(), nullptr,
                                      0, &basic_sig, &rnd_bsn));

  VerifierCtxObj ctx(this->kEps0GroupPublicKey);
  THROW_ON_EPIDERR(EpidVerifierSetHashAlg(ctx, kSha256));
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}
TEST_F(EpidMemberTest, SignBasicSucceedsWithoutPrecomputedSignatures) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  THROW_ON_EPIDERR(EpidAddPreSigs(member, 1));
  auto& msg = this->kMsg0;
  BasicSignature basic_sig;
  auto& bsn = this->kBsn0;
  THROW_ON_EPIDERR(EpidRegisterBasename(member, bsn.data(), bsn.size()));
  ASSERT_EQ(kEpidNoErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn.data(),
                          bsn.size(), &basic_sig, nullptr));
  // test sign without precomputed signatures
  EXPECT_EQ(kEpidNoErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn.data(),
                          bsn.size(), &basic_sig, nullptr));
  // verify basic signature
  VerifierCtxObj ctx(this->kGroupPublicKey);
  THROW_ON_EPIDERR(EpidVerifierSetBasename(ctx, bsn.data(), bsn.size()));
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}
/////////////////////////////////////////////////////////////////////////
// Variable messages
TEST_F(EpidMemberTest, SignBasicSucceedsGivenEmptyMessage) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  auto& bsn = this->kBsn0;
  THROW_ON_EPIDERR(EpidRegisterBasename(member, bsn.data(), bsn.size()));
  BasicSignature basic_sig;
  EXPECT_EQ(kEpidNoErr, EpidSignBasic(member, msg.data(), 0, bsn.data(),
                                      bsn.size(), &basic_sig, nullptr));
  VerifierCtxObj ctx(this->kGroupPublicKey);
  THROW_ON_EPIDERR(EpidVerifierSetBasename(ctx, bsn.data(), bsn.size()));
  EXPECT_EQ(kEpidSigValid, EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), 0));
}
TEST_F(EpidMemberTest, SignBasicSucceedsWithShortMessage) {
  // check: 1, 13, 128, 256, 512, 1021, 1024 bytes
  // 13 and 1021 are primes
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  BasicSignature basic_sig;
  BigNumStr rnd_bsn = {0};
  VerifierCtxObj ctx(this->kGroupPublicKey);
  size_t lengths[] = {1,   13,   128, 256,
                      512, 1021, 1024};  // have desired lengths to loop over
  std::vector<uint8_t> msg(
      lengths[COUNT_OF(lengths) - 1]);  // allocate message for max size
  for (size_t n = 0; n < msg.size(); n++) {
    msg.at(n) = (uint8_t)n;
  }
  for (auto length : lengths) {
    EXPECT_EQ(kEpidNoErr, EpidSignBasic(member, msg.data(), length, nullptr, 0,
                                        &basic_sig, &rnd_bsn))
        << "EpidSignBasic for message_len: " << length << " failed";
    EXPECT_EQ(kEpidNoErr,
              EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), length))
        << "EpidVerifyBasicSig for message_len: " << length << " failed";
  }
}
TEST_F(EpidMemberTest, SignBasicSucceedsWithLongMessage) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  BasicSignature basic_sig;
  BigNumStr rnd_bsn = {0};
  VerifierCtxObj ctx(this->kGroupPublicKey);
  {                                     // 1000000
    std::vector<uint8_t> msg(1000000);  // allocate message for max size
    for (size_t n = 0; n < msg.size(); n++) {
      msg.at(n) = (uint8_t)n;
    }
    EXPECT_EQ(kEpidNoErr, EpidSignBasic(member, msg.data(), msg.size(), nullptr,
                                        0, &basic_sig, &rnd_bsn))
        << "EpidSignBasic for message_len: " << 1000000 << " failed";
    EXPECT_EQ(kEpidNoErr,
              EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()))
        << "EpidVerifyBasicSig for message_len: " << 1000000 << " failed";
  }
}
TEST_F(EpidMemberTest, SignBasicSucceedsWithMsgContainingAllPossibleBytes) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kData_0_255;
  auto& bsn = this->kBsn0;
  THROW_ON_EPIDERR(EpidRegisterBasename(member, bsn.data(), bsn.size()));
  BasicSignature basic_sig;
  EXPECT_EQ(kEpidNoErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn.data(),
                          bsn.size(), &basic_sig, nullptr));
  VerifierCtxObj ctx(this->kGroupPublicKey);
  THROW_ON_EPIDERR(EpidVerifierSetBasename(ctx, bsn.data(), bsn.size()));
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}

TEST_F(EpidMemberTest,
       PROTECTED_SignBasicSucceedsMsgAllPossibleBytesForCredential_EPS0) {
  Prng my_prng;
  MemberCtxObj member(
      this->kEps0GroupPublicKey,
      *(MembershipCredential const*)&this->kEps0MemberPrivateKey,
      &Prng::Generate, &my_prng);
  auto& msg = this->kData_0_255;
  auto& bsn = this->kBsn0;
  BasicSignature basic_sig;
  THROW_ON_EPIDERR(EpidRegisterBasename(member, bsn.data(), bsn.size()));
  EXPECT_EQ(kEpidNoErr,
            EpidSignBasic(member, msg.data(), msg.size(), bsn.data(),
                          bsn.size(), &basic_sig, nullptr));
  VerifierCtxObj ctx(this->kEps0GroupPublicKey);
  THROW_ON_EPIDERR(EpidVerifierSetBasename(ctx, bsn.data(), bsn.size()));
  THROW_ON_EPIDERR(EpidVerifierSetHashAlg(ctx, kSha256));
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}
}  // namespace
