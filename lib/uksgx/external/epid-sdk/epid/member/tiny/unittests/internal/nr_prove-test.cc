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
/// Tiny NrProve unit tests.
/*! \file */

#ifndef SHARED
#include "epid/common-testhelper/epid_gtest-testhelper.h"
#include "epid/member/tiny/unittests/member-testhelper.h"
#include "gtest/gtest.h"

extern "C" {
#include "epid/member/tiny/src/native_types.h"
#include "epid/member/tiny/src/nrprove.h"
#include "epid/member/tiny/src/serialize.h"
#include "epid/member/tiny/src/signbasic.h"
}

#include "epid/common-testhelper/errors-testhelper.h"
#include "epid/common-testhelper/prng-testhelper.h"
#include "epid/common-testhelper/verifier_wrapper-testhelper.h"
namespace {

TEST_F(EpidMemberTest, NrProveFailsGivenInvalidSigRlEntry) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);

  auto msg = this->kTest1Msg;
  SigRl const* sig_rl = reinterpret_cast<const SigRl*>(this->kSigRlData.data());
  NrProof proof;
  // make sure that can generate NrProof using incorrupt sig_rl_enty
  THROW_ON_EPIDERR(EpidNrProve(member, msg.data(), msg.size(), &this->kBasicSig,
                               &sig_rl->bk[0], &proof));
  SigRlEntry sig_rl_enty_invalid_k = sig_rl->bk[0];
  sig_rl_enty_invalid_k.k.x.data.data[31]++;  // make it not in EC group
  EXPECT_EQ(kEpidBadArgErr,
            EpidNrProve(member, msg.data(), msg.size(), &this->kBasicSig,
                        &sig_rl_enty_invalid_k, &proof));

  SigRlEntry sig_rl_enty_invalid_b = sig_rl->bk[0];
  sig_rl_enty_invalid_b.b.x.data.data[31]++;  // make it not in EC group
  EXPECT_EQ(kEpidBadArgErr,
            EpidNrProve(member, msg.data(), msg.size(), &this->kBasicSig,
                        &sig_rl_enty_invalid_b, &proof));
}

TEST_F(EpidMemberTest, NrProveFailsGivenInvalidBasicSig) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);

  auto msg = this->kTest1Msg;
  SigRl const* sig_rl = reinterpret_cast<const SigRl*>(this->kSigRlData.data());

  NrProof proof;
  // make sure that can generate NrProof using incorrupt basic sig
  THROW_ON_EPIDERR(EpidNrProve(member, msg.data(), msg.size(), &this->kBasicSig,
                               &sig_rl->bk[0], &proof));
  // invalid basic sig is only when K value is invalid!!
  NativeBasicSignature basic_sig_invalid_K = this->kBasicSig;
  basic_sig_invalid_K.K.x.limbs.word[0]++;  // make it not in EC group
  EXPECT_EQ(kEpidBadArgErr,
            EpidNrProve(member, msg.data(), msg.size(), &basic_sig_invalid_K,
                        &sig_rl->bk[0], &proof));
}

TEST_F(EpidMemberTest, GeneratesNrProofForEmptyMessage) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);

  BasicSignature basic_sig;
  SigRl const* sig_rl = reinterpret_cast<const SigRl*>(this->kSigRlData.data());

  NrProof proof;

  EXPECT_EQ(kEpidNoErr, EpidNrProve(member, nullptr, 0, &this->kBasicSig,
                                    &sig_rl->bk[0], &proof));

  BasicSignatureSerialize(&basic_sig, &this->kBasicSig);
  // Check proof by doing an NrVerify
  VerifierCtxObj ctx(this->kGroupPublicKey);
  EXPECT_EQ(kEpidNoErr,
            EpidNrVerify(ctx, &basic_sig, nullptr, 0, &sig_rl->bk[0], &proof));
}

TEST_F(EpidMemberTest, GeneratesNrProofForMsgContainingAllPossibleBytes) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);

  BasicSignature basic_sig;
  auto msg = this->kData_0_255;
  SigRl const* sig_rl = reinterpret_cast<const SigRl*>(this->kSigRlData.data());

  NrProof proof;
  EXPECT_EQ(kEpidNoErr, EpidNrProve(member, msg.data(), msg.size(),
                                    &this->kBasicSig, &sig_rl->bk[0], &proof));

  // Check proof by doing an NrVerify
  BasicSignatureSerialize(&basic_sig, &this->kBasicSig);
  VerifierCtxObj ctx(this->kGroupPublicKey);
  EXPECT_EQ(kEpidNoErr, EpidNrVerify(ctx, &basic_sig, msg.data(), msg.size(),
                                     &sig_rl->bk[0], &proof));
}

TEST_F(EpidMemberTest, GeneratesNrProof) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);

  BasicSignature basic_sig;
  auto msg = this->kTest1Msg;
  SigRl const* sig_rl = reinterpret_cast<const SigRl*>(this->kSigRlData.data());

  NrProof proof;
  EXPECT_EQ(kEpidNoErr, EpidNrProve(member, msg.data(), msg.size(),
                                    &this->kBasicSig, &sig_rl->bk[0], &proof));

  // Check proof by doing an NrVerify
  BasicSignatureSerialize(&basic_sig, &this->kBasicSig);
  VerifierCtxObj ctx(this->kGroupPublicKey);
  EXPECT_EQ(kEpidNoErr, EpidNrVerify(ctx, &basic_sig, msg.data(), msg.size(),
                                     &sig_rl->bk[0], &proof));
}

TEST_F(EpidMemberTest, GeneratesNrProofUsingSha512) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);

  BasicSignature basic_sig;
  auto msg = this->kTest1Msg;
  SigRl const* sig_rl = reinterpret_cast<const SigRl*>(this->kSigRlData.data());
  THROW_ON_EPIDERR(EpidMemberSetHashAlg(member, kSha512));
  NrProof proof;
  EXPECT_EQ(kEpidNoErr, EpidNrProve(member, msg.data(), msg.size(),
                                    &this->kBasicSig, &sig_rl->bk[0], &proof));

  // Check proof by doing an NrVerify
  BasicSignatureSerialize(&basic_sig, &this->kBasicSig);
  VerifierCtxObj ctx(this->kGroupPublicKey);
  EXPECT_EQ(kEpidNoErr, EpidNrVerify(ctx, &basic_sig, msg.data(), msg.size(),
                                     &sig_rl->bk[0], &proof));
}

TEST_F(EpidMemberTest, GeneratesNrProofUsingSha256) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);

  BasicSignature basic_sig;
  auto msg = this->kTest1Msg;
  SigRl const* sig_rl = reinterpret_cast<const SigRl*>(this->kSigRlData.data());
  THROW_ON_EPIDERR(EpidMemberSetHashAlg(member, kSha256));
  NrProof proof;
  EXPECT_EQ(kEpidNoErr, EpidNrProve(member, msg.data(), msg.size(),
                                    &this->kBasicSig, &sig_rl->bk[0], &proof));

  // Check proof by doing an NrVerify
  BasicSignatureSerialize(&basic_sig, &this->kBasicSig);
  VerifierCtxObj ctx(this->kGroupPublicKey);
  THROW_ON_EPIDERR(EpidVerifierSetHashAlg(ctx, kSha256));
  EXPECT_EQ(kEpidNoErr, EpidNrVerify(ctx, &basic_sig, msg.data(), msg.size(),
                                     &sig_rl->bk[0], &proof));
}
}  // namespace
#endif  // SHARED
