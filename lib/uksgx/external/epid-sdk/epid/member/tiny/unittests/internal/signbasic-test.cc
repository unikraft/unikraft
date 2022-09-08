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

/*!
 * \file
 * \brief tiny SignBasic unit tests.
 */

#ifndef SHARED
#include "gtest/gtest.h"

extern "C" {
#include "epid/member/tiny/src/native_types.h"
#include "epid/member/tiny/src/serialize.h"
#include "epid/member/tiny/src/signbasic.h"
#include "epid/verifier/api.h"
}

#include "epid/common-testhelper/errors-testhelper.h"
#include "epid/common-testhelper/prng-testhelper.h"
#include "epid/common-testhelper/verifier_wrapper-testhelper.h"
#include "epid/member/tiny/unittests/member-testhelper.h"

namespace {

/////////////////////////////////////////////////////////////////////////
// SignBasic

TEST_F(EpidMemberTest, BasicSignaturesOfSameMessageAreDifferent) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  NativeBasicSignature basic_sig1 = {0};
  NativeBasicSignature basic_sig2 = {0};
  EXPECT_EQ(kEpidNoErr, EpidSignBasic(member, msg.data(), msg.size(), nullptr,
                                      0, &basic_sig1));
  EXPECT_EQ(kEpidNoErr, EpidSignBasic(member, msg.data(), msg.size(), nullptr,
                                      0, &basic_sig2));
  EXPECT_NE(0, memcmp(&basic_sig1, &basic_sig2, sizeof(NativeBasicSignature)));
}

TEST_F(EpidMemberTest, SignBasicSucceedsUsingRandomBase) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  NativeBasicSignature native_basic_sig = {0};
  EXPECT_EQ(kEpidNoErr, EpidSignBasic(member, msg.data(), msg.size(), nullptr,
                                      0, &native_basic_sig));
  // verify basic signature
  VerifierCtxObj ctx(this->kGroupPublicKey);
  BasicSignature basic_sig = {0};
  BasicSignatureSerialize(&basic_sig, &native_basic_sig);
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}

TEST_F(EpidMemberTest, SignBasicSucceedsUsingSha512) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  NativeBasicSignature native_basic_sig = {0};
  THROW_ON_EPIDERR(EpidMemberSetHashAlg(member, kSha512));
  EXPECT_EQ(kEpidNoErr, EpidSignBasic(member, msg.data(), msg.size(), nullptr,
                                      0, &native_basic_sig));
  // verify basic signature
  VerifierCtxObj ctx(this->kGroupPublicKey);
  BasicSignature basic_sig = {0};
  BasicSignatureSerialize(&basic_sig, &native_basic_sig);
  EpidVerifierSetHashAlg(ctx, kSha512);
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}

TEST_F(EpidMemberTest, SignBasicSucceedsUsingSha256) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  NativeBasicSignature native_basic_sig = {0};
  THROW_ON_EPIDERR(EpidMemberSetHashAlg(member, kSha256));
  EXPECT_EQ(kEpidNoErr, EpidSignBasic(member, msg.data(), msg.size(), nullptr,
                                      0, &native_basic_sig));
  // verify basic signature
  VerifierCtxObj ctx(this->kGroupPublicKey);
  BasicSignature basic_sig = {0};
  BasicSignatureSerialize(&basic_sig, &native_basic_sig);
  EpidVerifierSetHashAlg(ctx, kSha256);
  EXPECT_EQ(kEpidSigValid,
            EpidVerifyBasicSig(ctx, &basic_sig, msg.data(), msg.size()));
}

}  // namespace
#endif  // SHARED
