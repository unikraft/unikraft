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

/*!
 * \file
 * \brief Member unit tests.
 */
#include <cstring>
#include <vector>

#include "epid/common-testhelper/epid_gtest-testhelper.h"
#include "gtest/gtest.h"

#include "epid/common-testhelper/epid2params_wrapper-testhelper.h"
#include "epid/common-testhelper/errors-testhelper.h"
#include "epid/common-testhelper/mem_params-testhelper.h"
#include "epid/common-testhelper/prng-testhelper.h"
#include "epid/member/tpm2/unittests/tpm2-testhelper.h"
#include "epid/member/unittests/member-testhelper.h"

extern "C" {
#include "epid/member/api.h"
#include "epid/member/src/context.h"
#include "epid/member/src/storage.h"
#include "epid/member/src/write_precomp.h"
#include "epid/member/tpm2/nv.h"
}
bool operator==(MemberPrecomp const& lhs, MemberPrecomp const& rhs) {
  return 0 == std::memcmp(&lhs, &rhs, sizeof(lhs));
}
/// compares GroupPubKey values
bool operator==(GroupPubKey const& lhs, GroupPubKey const& rhs);

/// compares MembershipCredential values
bool operator==(MembershipCredential const& lhs,
                MembershipCredential const& rhs);
namespace {
//////////////////////////////////////////////////////////////////////////
// EpidMemberDeinit Tests
TEST_F(EpidMemberTest, DeinitWorksGivenNullMemberCtx) {
  EpidMemberDeinit(nullptr);
}

//////////////////////////////////////////////////////////////////////////
// EpidMemberGetSize Tests
TEST_F(EpidMemberTest, GetSizeFailsGivenNullParams) {
  size_t ctx_size = 0;
  MemberParams params = {0};
  EXPECT_EQ(kEpidBadArgErr, EpidMemberGetSize(&params, nullptr));
  EXPECT_EQ(kEpidBadArgErr, EpidMemberGetSize(nullptr, &ctx_size));
  EXPECT_EQ(kEpidBadArgErr, EpidMemberGetSize(nullptr, nullptr));
}

//////////////////////////////////////////////////////////////////////////
// EpidMemberGetSize Tests
TEST_F(EpidMemberTest, GetSizeWorksGivenValidParams) {
  size_t ctx_size = 0;
  Prng my_prng;
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &my_prng, nullptr, &params);
  EXPECT_EQ(kEpidNoErr, EpidMemberGetSize(&params, &ctx_size));
}

//////////////////////////////////////////////////////////////////////////
// EpidMemberInit Tests
TEST_F(EpidMemberTest, InitFailsGivenNullParameters) {
  size_t ctx_size = 0;
  MemberCtx* ctx = nullptr;
  Prng my_prng;
  MemberParams params = {0};
  std::vector<uint8_t> ctx_buf;
  SetMemberParams(&Prng::Generate, &my_prng, nullptr, &params);
  EXPECT_EQ(kEpidNoErr, EpidMemberGetSize(&params, &ctx_size));
  ctx_buf.resize(ctx_size);
  ctx = (MemberCtx*)&ctx_buf[0];

  EXPECT_EQ(kEpidBadArgErr, EpidMemberInit(nullptr, nullptr));
  EXPECT_EQ(kEpidBadArgErr, EpidMemberInit(&params, nullptr));
  EXPECT_EQ(kEpidBadArgErr, EpidMemberInit(nullptr, ctx));
}

TEST_F(EpidMemberTest, InitFailsGivenInvalidParameters) {
  FpElemStr f = {
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
  };
  size_t ctx_size = 0;
  MemberCtx* ctx = nullptr;
  Prng my_prng;
  MemberParams params = {0};
  std::vector<uint8_t> ctx_buf;
  SetMemberParams(&Prng::Generate, &my_prng, &f, &params);
  EXPECT_EQ(kEpidNoErr, EpidMemberGetSize(&params, &ctx_size));
  ctx_buf.resize(ctx_size);
  ctx = (MemberCtx*)&ctx_buf[0];

  EXPECT_EQ(kEpidBadArgErr, EpidMemberInit(&params, ctx));
}

TEST_F(EpidMemberTest, InitSucceedsGivenValidParameters) {
  FpElemStr f = {
      0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
  };
  size_t ctx_size = 0;
  MemberCtx* ctx = nullptr;
  Prng my_prng;
  MemberParams params = {0};
  std::vector<uint8_t> ctx_buf;
  SetMemberParams(&Prng::Generate, &my_prng, &f, &params);
  EXPECT_EQ(kEpidNoErr, EpidMemberGetSize(&params, &ctx_size));
  ctx_buf.resize(ctx_size);
  ctx = (MemberCtx*)&ctx_buf[0];

  EXPECT_EQ(kEpidNoErr, EpidMemberInit(&params, ctx));
  EpidMemberDeinit(ctx);
}

TEST_F(EpidMemberTest, InitSucceedsGivenValidParametersWithNoF) {
  size_t ctx_size = 0;
  MemberCtx* ctx = nullptr;
  Prng my_prng;
  MemberParams params = {0};
  std::vector<uint8_t> ctx_buf;
  SetMemberParams(&Prng::Generate, &my_prng, nullptr, &params);
  EXPECT_EQ(kEpidNoErr, EpidMemberGetSize(&params, &ctx_size));
  ctx_buf.resize(ctx_size);
  ctx = (MemberCtx*)&ctx_buf[0];

  EXPECT_EQ(kEpidNoErr, EpidMemberInit(&params, ctx));
  EpidMemberDeinit(ctx);
}

//////////////////////////////////////////////////////////////////////////
// EpidMemberDelete Tests
TEST_F(EpidMemberTest, DeleteWorksGivenNullMemberCtx) {
  EpidMemberDelete(nullptr);
  MemberCtx* member_ctx = nullptr;
  EpidMemberDelete(&member_ctx);
}
TEST_F(EpidMemberTest, DeleteNullsMemberCtx) {
  MemberCtx* ctx = nullptr;
  Prng my_prng;
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &my_prng, nullptr, &params);
  THROW_ON_EPIDERR(EpidMemberCreate(&params, &ctx));
  EpidMemberDelete(&ctx);
  EXPECT_EQ(nullptr, ctx);
}

//////////////////////////////////////////////////////////////////////////
// EpidMemberCreate Tests
// test that create fails if any mandatory parameters are NULL
TEST_F(EpidMemberTest, CreateFailsGivenNullParameters) {
  MemberCtx* ctx = nullptr;
  Prng my_prng;
  MemberParams params = {0};
  EXPECT_EQ(kEpidBadArgErr, EpidMemberCreate(nullptr, &ctx));

  SetMemberParams(&Prng::Generate, &my_prng, nullptr, &params);
  EXPECT_EQ(kEpidBadArgErr, EpidMemberCreate(&params, nullptr));

  SetMemberParams(nullptr, &my_prng, nullptr, &params);
  EXPECT_EQ(kEpidBadArgErr, EpidMemberCreate(&params, nullptr));
}

TEST_F(EpidMemberTest, CreateFailsGivenInvalidParameters) {
  MemberCtx* ctx = nullptr;
  Prng my_prng;
  MemberParams params = {0};
  FpElemStr f = {
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
  };
  SetMemberParams(&Prng::Generate, &my_prng, &f, &params);
  EXPECT_EQ(kEpidBadArgErr, EpidMemberCreate(&params, &ctx));
  EpidMemberDelete(&ctx);
}

TEST_F(EpidMemberTest, CreateSucceedsGivenValidParameters) {
  MemberCtx* ctx = nullptr;
  Prng my_prng;
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &my_prng, nullptr, &params);
  EXPECT_EQ(kEpidNoErr, EpidMemberCreate(&params, &ctx));
  EpidMemberDelete(&ctx);
}

//////////////////////////////////////////////////////////////////////////
// EpidMemberStartup
TEST_F(EpidMemberTest, StartupFailsGivenNullParameters) {
  EXPECT_EQ(kEpidBadArgErr, EpidMemberStartup(nullptr));
}

TEST_F(EpidMemberTest, StartupSucceedsGivenValidParameters) {
  Prng prng;
  GroupPubKey pub_key = this->kGroupPublicKey;
  PrivKey priv_key = this->kMemberPrivateKey;
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &prng, nullptr, &params);
  MemberCtxObj member(&params);
  EXPECT_EQ(kEpidNoErr, EpidProvisionKey(member, &pub_key, &priv_key, nullptr));
  MembershipCredential credential_expected = ((MemberCtx*)member)->credential;
  // reset member credential to test if startup reads them from NV memory
  // correctly
  ((MemberCtx*)member)->pub_key = {0};
  ((MemberCtx*)member)->credential = {0};
  EXPECT_EQ(kEpidNoErr, EpidMemberStartup(member));
  EXPECT_EQ(pub_key, ((MemberCtx*)member)->pub_key);
  EXPECT_EQ(credential_expected, ((MemberCtx*)member)->credential);
}

//////////////////////////////////////////////////////////////////////////
// EpidMemberSetHashAlg
TEST_F(EpidMemberTest, SetHashAlgFailsGivenNullPtr) {
  EXPECT_EQ(kEpidBadArgErr, EpidMemberSetHashAlg(nullptr, kSha256));
}
TEST_F(EpidMemberTest, CanSetHashAlgoToSHA256) {
  Prng my_prng;
  MemberCtxObj member_ctx(&Prng::Generate, &my_prng);
  EXPECT_EQ(kEpidNoErr, EpidMemberSetHashAlg(member_ctx, kSha256));
}
TEST_F(EpidMemberTest, CanSetHashAlgoToSHA384) {
  Prng my_prng;
  MemberCtxObj member_ctx(&Prng::Generate, &my_prng);
  EXPECT_EQ(kEpidNoErr, EpidMemberSetHashAlg(member_ctx, kSha384));
}
TEST_F(EpidMemberTest, CanSetHashAlgoToSHA512) {
  Prng my_prng;
  MemberCtxObj member_ctx(&Prng::Generate, &my_prng);
  EXPECT_EQ(kEpidNoErr, EpidMemberSetHashAlg(member_ctx, kSha512));
}
TEST_F(EpidMemberTest, CanSetHashAlgoToSHA512256) {
  Prng my_prng;
  MemberCtxObj member_ctx(&Prng::Generate, &my_prng);
  EXPECT_EQ(kEpidNoErr, EpidMemberSetHashAlg(member_ctx, kSha512_256));
}
TEST_F(EpidMemberTest, SetHashAlgFailsForNonSupportedAlgorithm) {
  Prng my_prng;
  MemberCtxObj member_ctx(&Prng::Generate, &my_prng);
  EXPECT_EQ(kEpidBadArgErr, EpidMemberSetHashAlg(member_ctx, kSha3_256));
  EXPECT_EQ(kEpidBadArgErr, EpidMemberSetHashAlg(member_ctx, kSha3_384));
  EXPECT_EQ(kEpidBadArgErr, EpidMemberSetHashAlg(member_ctx, kSha3_512));
  EXPECT_EQ(kEpidBadArgErr, EpidMemberSetHashAlg(member_ctx, (HashAlg)-1));
}
//////////////////////////////////////////////////////////////////////////
// EpidMemberSetSigRl
TEST_F(EpidMemberTest, SetSigRlFailsGivenNullPointer) {
  Prng my_prng;
  MemberCtxObj member_ctx(this->kGroupPublicKey, this->kMemberPrivateKey,
                          &Prng::Generate, &my_prng);
  SigRl srl = {{{0}}, {{0}}, {{0}}, {{{{0}, {0}}, {{0}, {0}}}}};
  srl.gid = this->kGroupPublicKey.gid;
  EXPECT_EQ(kEpidBadArgErr, EpidMemberSetSigRl(nullptr, &srl, sizeof(SigRl)));
  EXPECT_EQ(kEpidBadArgErr,
            EpidMemberSetSigRl(member_ctx, nullptr, sizeof(SigRl)));
}
TEST_F(EpidMemberTest, SetSigRlFailsGivenZeroSize) {
  Prng my_prng;
  MemberCtxObj member_ctx(this->kGroupPublicKey, this->kMemberPrivateKey,
                          &Prng::Generate, &my_prng);
  SigRl srl = {{{0}}, {{0}}, {{0}}, {{{{0}, {0}}, {{0}, {0}}}}};
  srl.gid = this->kGroupPublicKey.gid;
  EXPECT_EQ(kEpidBadArgErr, EpidMemberSetSigRl(member_ctx, &srl, 0));
}
// Size parameter must be at least big enough for n2 == 0 case
TEST_F(EpidMemberTest, SetSigRlFailsGivenTooSmallSize) {
  Prng my_prng;
  MemberCtxObj member_ctx(this->kGroupPublicKey, this->kMemberPrivateKey,
                          &Prng::Generate, &my_prng);
  SigRl srl = {{{0}}, {{0}}, {{0}}, {{{{0}, {0}}, {{0}, {0}}}}};
  srl.gid = this->kGroupPublicKey.gid;
  EXPECT_EQ(
      kEpidBadArgErr,
      EpidMemberSetSigRl(member_ctx, &srl, (sizeof(srl) - sizeof(srl.bk)) - 1));
  srl.n2 = this->kOctStr32_1;
  EXPECT_EQ(
      kEpidBadArgErr,
      EpidMemberSetSigRl(member_ctx, &srl, (sizeof(srl) - sizeof(srl.bk)) - 1));
}
TEST_F(EpidMemberTest, SetSigRlFailsGivenN2TooBigForSize) {
  Prng my_prng;
  MemberCtxObj member_ctx(this->kGroupPublicKey, this->kMemberPrivateKey,
                          &Prng::Generate, &my_prng);
  SigRl srl = {{{0}}, {{0}}, {{0}}, {{{{0}, {0}}, {{0}, {0}}}}};
  srl.gid = this->kGroupPublicKey.gid;
  srl.n2 = this->kOctStr32_1;
  EXPECT_EQ(kEpidBadArgErr,
            EpidMemberSetSigRl(member_ctx, &srl, sizeof(srl) - sizeof(srl.bk)));
}
TEST_F(EpidMemberTest, SetSigRlFailsGivenN2TooSmallForSize) {
  Prng my_prng;
  MemberCtxObj member_ctx(this->kGroupPublicKey, this->kMemberPrivateKey,
                          &Prng::Generate, &my_prng);
  SigRl srl = {{{0}}, {{0}}, {{0}}, {{{{0}, {0}}, {{0}, {0}}}}};
  srl.gid = this->kGroupPublicKey.gid;
  EXPECT_EQ(kEpidBadArgErr, EpidMemberSetSigRl(member_ctx, &srl, sizeof(srl)));
}
TEST_F(EpidMemberTest, SetSigRlFailsGivenBadGroupId) {
  Prng my_prng;
  MemberCtxObj member_ctx(this->kGroupPublicKey, this->kMemberPrivateKey,
                          &Prng::Generate, &my_prng);
  SigRl srl = {{{0}}, {{0}}, {{0}}, {{{{0}, {0}}, {{0}, {0}}}}};
  srl.gid = this->kGroupPublicKey.gid;
  srl.gid.data[0] = ~srl.gid.data[0];
  EXPECT_EQ(kEpidBadArgErr,
            EpidMemberSetSigRl(member_ctx, &srl, sizeof(srl) - sizeof(srl.bk)));
}
TEST_F(EpidMemberTest, SetSigRlFailsGivenEmptySigRlFromDifferentGroup) {
  Prng my_prng;
  MemberCtxObj member_ctx(this->kGroupPublicKey, this->kMemberPrivateKey,
                          &Prng::Generate, &my_prng);
  SigRl const* sig_rl = reinterpret_cast<SigRl const*>(this->kGrpXSigRl.data());
  size_t sig_rl_size = this->kGrpXSigRl.size();
  EXPECT_EQ(kEpidBadArgErr,
            EpidMemberSetSigRl(member_ctx, sig_rl, sig_rl_size));
}
TEST_F(EpidMemberTest, SetSigRlFailsGivenOldVersion) {
  Prng my_prng;
  MemberCtxObj member_ctx(this->kGroupPublicKey, this->kMemberPrivateKey,
                          &Prng::Generate, &my_prng);
  SigRl srl = {{{0}}, {{0}}, {{0}}, {{{{0}, {0}}, {{0}, {0}}}}};
  srl.gid = this->kGroupPublicKey.gid;
  srl.version = this->kOctStr32_1;
  EXPECT_EQ(kEpidNoErr,
            EpidMemberSetSigRl(member_ctx, &srl, sizeof(srl) - sizeof(srl.bk)));
  OctStr32 octstr32_0 = {0x00, 0x00, 0x00, 0x00};
  srl.version = octstr32_0;
  EXPECT_EQ(kEpidBadArgErr,
            EpidMemberSetSigRl(member_ctx, &srl, sizeof(srl) - sizeof(srl.bk)));
}
TEST_F(EpidMemberTest, SetSigRlPreservesOldRlOnFailure) {
  Prng my_prng;
  MemberCtxObj member_ctx(this->kGrpXKey, this->kGrpXSigrevokedMember0PrivKey,
                          &Prng::Generate, &my_prng);
  SigRl const* sig_rl = reinterpret_cast<SigRl const*>(this->kGrpXSigRl.data());
  size_t sig_rl_size = this->kGrpXSigRl.size();
  EXPECT_EQ(kEpidNoErr, EpidMemberSetSigRl(member_ctx, sig_rl, sig_rl_size));
  // wrong sigrl contains revoked member0 and has lower version
  SigRl const* wrong_sig_rl =
      reinterpret_cast<SigRl const*>(this->kGrpXSigRlSingleEntry.data());
  size_t wrong_sig_rl_size = this->kGrpXSigRlSingleEntry.size();
  EXPECT_EQ(kEpidBadArgErr,
            EpidMemberSetSigRl(member_ctx, wrong_sig_rl, wrong_sig_rl_size));
  auto& msg = this->kMsg0;
  std::vector<uint8_t> sig_data(EpidGetSigSize(sig_rl));
  EpidSignature* sig = reinterpret_cast<EpidSignature*>(sig_data.data());
  size_t sig_len = sig_data.size() * sizeof(uint8_t);
  // Check that sigrevoked member is still in SigRl
  EXPECT_EQ(kEpidSigRevokedInSigRl, EpidSign(member_ctx, msg.data(), msg.size(),
                                             nullptr, 0, sig, sig_len));
}
TEST_F(EpidMemberTest, SetSigRlWorksGivenValidSigRl) {
  Prng my_prng;
  MemberCtxObj member_ctx(this->kGrpXKey, this->kGrpXMember0PrivKey,
                          &Prng::Generate, &my_prng);
  SigRl const* sig_rl = reinterpret_cast<SigRl const*>(this->kGrpXSigRl.data());
  size_t sig_rl_size = this->kGrpXSigRl.size();
  EXPECT_EQ(kEpidNoErr, EpidMemberSetSigRl(member_ctx, sig_rl, sig_rl_size));
}
TEST_F(EpidMemberTest, SetSigRlWorksGivenEmptySigRl) {
  Prng my_prng;
  MemberCtxObj member_ctx(this->kGroupPublicKey, this->kMemberPrivateKey,
                          &Prng::Generate, &my_prng);
  uint8_t sig_rl_data_n2_zero[] = {
      // gid
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x01,
      // version
      0x00, 0x00, 0x00, 0x00,
      // n2
      0x0, 0x00, 0x00, 0x00,
      // not bk's
  };
  SigRl* sig_rl = reinterpret_cast<SigRl*>(sig_rl_data_n2_zero);
  size_t sig_rl_size = sizeof(sig_rl_data_n2_zero);
  EXPECT_EQ(kEpidNoErr, EpidMemberSetSigRl(member_ctx, sig_rl, sig_rl_size));
}
TEST_F(EpidMemberTest, SetSigRlWorksGivenSigRlWithOneEntry) {
  Prng my_prng;
  MemberCtxObj member_ctx(this->kGrpXKey, this->kGrpXMember0PrivKey,
                          &Prng::Generate, &my_prng);
  SigRl const* sig_rl =
      reinterpret_cast<SigRl const*>(this->kGrpXSigRlSingleEntry.data());
  size_t sig_rl_size = this->kGrpXSigRlSingleEntry.size();
  EXPECT_EQ(kEpidNoErr, EpidMemberSetSigRl(member_ctx, sig_rl, sig_rl_size));
}
TEST_F(EpidMemberTest, SetSigRlFailsIfNotProvisioned) {
  Prng my_prng;
  MemberCtxObj member_ctx(&Prng::Generate, &my_prng);
  uint8_t sig_rl_data[] = {
      // gid
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x01,
      // version
      0x00, 0x00, 0x00, 0x00,
      // n2
      0x0, 0x00, 0x00, 0x00,
      // not bk's
  };
  SigRl* sig_rl = reinterpret_cast<SigRl*>(sig_rl_data);
  size_t sig_rl_size = sizeof(sig_rl_data);
  EXPECT_EQ(kEpidOutOfSequenceError,
            EpidMemberSetSigRl(member_ctx, sig_rl, sig_rl_size));
}
//////////////////////////////////////////////////////////////////////////
// EpidRegisterBasename
TEST_F(EpidMemberTest, RegisterBaseNameFailsGivenNullPtr) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  std::vector<uint8_t> basename = {'_', 'b', 'a', 's', 'e', 'n', 'a', 'm', 'e'};
  EXPECT_EQ(kEpidBadArgErr,
            EpidRegisterBasename(member, nullptr, basename.size()));
  EXPECT_EQ(kEpidBadArgErr,
            EpidRegisterBasename(nullptr, basename.data(), basename.size()));
}
TEST_F(EpidMemberTest, RegisterBaseNameFailsGivenDuplicateBaseName) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  std::vector<uint8_t> basename = {'d', 'b', 'a', 's', 'e', 'n', 'a', 'm', 'e'};
  EXPECT_EQ(kEpidNoErr,
            EpidRegisterBasename(member, basename.data(), basename.size()));
  EXPECT_EQ(kEpidDuplicateErr,
            EpidRegisterBasename(member, basename.data(), basename.size()));
}
TEST_F(EpidMemberTest, RegisterBaseNameFailsGivenInvalidBaseName) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  std::vector<uint8_t> basename = {};
  std::vector<uint8_t> basename2 = {'b', 's', 'n'};
  EXPECT_EQ(kEpidBadArgErr,
            EpidRegisterBasename(member, basename.data(), basename.size()));
  EXPECT_EQ(kEpidBadArgErr, EpidRegisterBasename(member, basename2.data(), 0));
}
TEST_F(EpidMemberTest, RegisterBaseNameSucceedsGivenUniqueBaseName) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  std::vector<uint8_t> basename = {'b', 's', 'n', '0', '1'};
  EXPECT_EQ(kEpidNoErr,
            EpidRegisterBasename(member, basename.data(), basename.size()));
}
TEST_F(EpidMemberTest, RegisterBaseNameSucceedsGivenMultipleUniqueBaseNames) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  std::vector<uint8_t> basename1 = {'b', 's', 'n', '0', '1'};
  std::vector<uint8_t> basename2 = {'b', 's', 'n', '0', '2'};
  std::vector<uint8_t> basename3 = {'b', 's', 'n', '0', '3'};
  EXPECT_EQ(kEpidNoErr,
            EpidRegisterBasename(member, basename1.data(), basename1.size()));
  EXPECT_EQ(kEpidNoErr,
            EpidRegisterBasename(member, basename2.data(), basename2.size()));
  EXPECT_EQ(kEpidNoErr,
            EpidRegisterBasename(member, basename3.data(), basename3.size()));
  // Verify that basenames registered succesfully
  EXPECT_EQ(kEpidDuplicateErr,
            EpidRegisterBasename(member, basename1.data(), basename1.size()));
  EXPECT_EQ(kEpidDuplicateErr,
            EpidRegisterBasename(member, basename2.data(), basename2.size()));
  EXPECT_EQ(kEpidDuplicateErr,
            EpidRegisterBasename(member, basename3.data(), basename3.size()));
}
TEST_F(EpidMemberTest,
       RegisterBaseNameSucceedsGivenBsnContainingAllPossibleBytes) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  EXPECT_EQ(kEpidNoErr, EpidRegisterBasename(member, this->kData_0_255.data(),
                                             this->kData_0_255.size()));
}
//////////////////////////////////////////////////////////////////////////
// EpidClearRegisteredBasenames
TEST_F(EpidMemberTest, EpidClearRegisteredBasenamesFailsGivenNullPtr) {
  EXPECT_EQ(kEpidBadArgErr, EpidClearRegisteredBasenames(nullptr));
}
TEST_F(EpidMemberTest, EpidClearRegisteredBasenamesClearsBasenames) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  THROW_ON_EPIDERR(
      EpidRegisterBasename(member, this->kBsn0.data(), this->kBsn0.size()));
  EXPECT_EQ(kEpidNoErr, EpidClearRegisteredBasenames(member));
  // check, that after clearing EpidRegisterBasename works correctly
  THROW_ON_EPIDERR(
      EpidRegisterBasename(member, this->kBsn0.data(), this->kBsn0.size()));
}
TEST_F(EpidMemberTest, EpidClearRegisteredBasenamesClearsAllBasenames) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  for (int i = 0; i < 3; ++i) {
    THROW_ON_EPIDERR(EpidRegisterBasename(member, &i, sizeof(i)));
  }
  EXPECT_EQ(kEpidNoErr, EpidClearRegisteredBasenames(member));
  for (int i = 0; i < 3; ++i) {
    THROW_ON_EPIDERR(EpidRegisterBasename(member, &i, sizeof(i)));
  }
}
TEST_F(EpidMemberTest,
       EpidClearRegisteredBasenamesCausesSignWithBasenameAfterItToFail) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  auto& msg = this->kMsg0;
  auto& bsn = this->kBsn0;
  THROW_ON_EPIDERR(EpidRegisterBasename(member, bsn.data(), bsn.size()));
  std::vector<uint8_t> sig_data(EpidGetSigSize(nullptr));
  EpidSignature* sig = reinterpret_cast<EpidSignature*>(sig_data.data());
  size_t sig_len = sig_data.size() * sizeof(uint8_t);
  THROW_ON_EPIDERR(EpidSign(member, msg.data(), msg.size(), bsn.data(),
                            bsn.size(), sig, sig_len));
  THROW_ON_EPIDERR(EpidClearRegisteredBasenames(member));
  ASSERT_EQ(kEpidBadArgErr, EpidSign(member, msg.data(), msg.size(), bsn.data(),
                                     bsn.size(), sig, sig_len));
}
//////////////////////////////////////////////////////////////////////////
// EpidMemberWritePrecomp
TEST_F(EpidMemberTest, MemberWritePrecompFailsGivenNullPointer) {
  MemberPrecomp precomp;
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  MemberCtx* ctx = member;
  EXPECT_EQ(kEpidBadArgErr, EpidMemberWritePrecomp(nullptr, &precomp));
  EXPECT_EQ(kEpidBadArgErr, EpidMemberWritePrecomp(ctx, nullptr));
}
TEST_F(EpidMemberTest,
       MemberWritePrecompSucceedGivenValidArgumentWithLoadedPrecomp) {
  MemberPrecomp precomp;
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  MemberCtx* ctx = member;
  EXPECT_EQ(kEpidNoErr, EpidMemberWritePrecomp(ctx, &precomp));
  MemberPrecomp expected_precomp = this->kMemberPrecomp;
  EXPECT_EQ(expected_precomp, precomp);
}
TEST_F(EpidMemberTest,
       MemberWritePrecompSucceedGivenValidArgumentWithCalcPrecomp) {
  MemberPrecomp precomp;
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      &Prng::Generate, &my_prng);
  MemberCtx* ctx = member;
  EXPECT_EQ(kEpidNoErr, EpidMemberWritePrecomp(ctx, &precomp));
  MemberPrecomp expected_precomp = this->kMemberPrecomp;
  EXPECT_EQ(expected_precomp, precomp);
}
TEST_F(EpidMemberTest, DefaultHashAlgIsSha512) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);
  MemberCtx* ctx = member;
  EXPECT_EQ(kSha512, ctx->hash_alg);
}
#ifdef TPM_TSS
//////////////////////////////////////////////////////////////////////////
// MemberCanLoadMembershipCredentialFromTpm
TEST_F(EpidMemberTest,
       MemberCanLoadPreviouslyProvisionedMembershipCredentialFromTpm) {
  // Not clear that this test is valid or in the right place.
  Prng prng;
  Epid2ParamsObj epid2params;

  uint32_t const nv_index = 0x01c10100;
  GroupPubKey pub_key_expected = this->kGrpXKey;
  GroupPubKey pub_key;
  FpElemStr f = this->kGrpXMember9PrivKey.f;
  MembershipCredential credential_expected = {this->kGrpXMember9PrivKey.gid,
                                              this->kGrpXMember9PrivKey.A,
                                              this->kGrpXMember9PrivKey.x};
  MembershipCredential credential;
  // Tpm2CtxObj calls Tpm2CreateContext() and sets
  // is_context_already_created=true. To call this function in
  // EpidMemberInit() successfully Tpm2DeleteContext() must be called.
  // Putting creation of Tpm2CtxObj object in a block solves it
  {
    // write credentials
    Tpm2CtxObj tpm(&Prng::Generate, &prng, nullptr, epid2params);
    THROW_ON_EPIDERR(EpidNvWriteMembershipCredential(
        tpm, &pub_key_expected, &credential_expected, nv_index));

    // read credentials to confirm that credential has been really inserted
    EXPECT_EQ(kEpidNoErr, EpidNvReadMembershipCredential(
                              tpm, nv_index, &pub_key, &credential));
    EXPECT_EQ(pub_key_expected, pub_key);
    EXPECT_EQ(credential_expected, credential);
  }

  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &prng, &f, &params);
  MemberCtxObj member(&params);
  EXPECT_EQ(kEpidNoErr, EpidMemberStartup(member));
  EXPECT_EQ(pub_key_expected, ((MemberCtx*)member)->pub_key);
  EXPECT_EQ(credential_expected, ((MemberCtx*)member)->credential);

  Tpm2NvUndefineSpace(((MemberCtx*)member)->tpm2_ctx, nv_index);
}
#endif
}  // namespace
