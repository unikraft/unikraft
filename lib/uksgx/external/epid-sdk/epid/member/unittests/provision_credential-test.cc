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
 * \brief Provision credential unit tests.
 */
#include <cstring>
#include <vector>

#include "gtest/gtest.h"

extern "C" {
#include "epid/member/api.h"
#include "epid/member/src/context.h"
#include "epid/member/src/storage.h"
}

#include "epid/common-testhelper/epid_gtest-testhelper.h"
#include "epid/common-testhelper/errors-testhelper.h"
#include "epid/common-testhelper/mem_params-testhelper.h"
#include "epid/common-testhelper/prng-testhelper.h"
#include "epid/member/unittests/member-testhelper.h"

namespace {

EpidStatus ProvisionCredentialAndStart(MemberCtx* ctx,
                                       GroupPubKey const* pub_key,
                                       MembershipCredential const* credential,
                                       MemberPrecomp const* precomp_str) {
  EpidStatus sts;
  sts = EpidProvisionCredential(ctx, pub_key, credential, precomp_str);
  if (sts != kEpidNoErr) {
    return sts;
  }
  sts = EpidMemberStartup(ctx);
  return sts;
}

TEST_F(EpidMemberTest, ProvisionCredentialFailsGivenNullParameters) {
  Prng prng;
  GroupPubKey pub_key = this->kGrpXKey;
  FpElemStr f = this->kGrpXMember9PrivKey.f;
  MembershipCredential credential;
  credential.A = this->kGrpXMember9PrivKey.A;
  credential.gid = this->kGrpXMember9PrivKey.gid;
  credential.x = this->kGrpXMember9PrivKey.x;
  // Note: this MemberPrecomp is for the wrong group, however it should not
  // be checked in Provision because doing so would negate the performance
  // boost of using the precomp.
  MemberPrecomp precomp = this->kMemberPrecomp;
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &prng, &f, &params);
  MemberCtxObj member(&params);
  EXPECT_EQ(kEpidBadArgErr,
            EpidProvisionCredential(nullptr, &pub_key, &credential, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            EpidProvisionCredential(member, nullptr, &credential, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            EpidProvisionCredential(member, &pub_key, nullptr, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            EpidProvisionCredential(nullptr, &pub_key, &credential, nullptr));
  EXPECT_EQ(kEpidBadArgErr,
            EpidProvisionCredential(member, nullptr, &credential, nullptr));
  EXPECT_EQ(kEpidBadArgErr,
            EpidProvisionCredential(member, &pub_key, nullptr, nullptr));
}

TEST_F(EpidMemberTest, ProvisionCredentialRejectsInvalidCredential) {
  Prng prng;
  GroupPubKey pub_key = this->kGrpXKey;
  FpElemStr f = this->kGrpXMember9PrivKey.f;
  MembershipCredential credential;
  MembershipCredential base_credential;
  base_credential.A = this->kGrpXMember9PrivKey.A;
  base_credential.gid = this->kGrpXMember9PrivKey.gid;
  base_credential.x = this->kGrpXMember9PrivKey.x;
  // Note: this MemberPrecomp is for the wrong group, however it should not
  // be checked in Provision because doing so would negate the performance
  // boost of using the precomp.
  MemberPrecomp precomp = this->kMemberPrecomp;
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &prng, &f, &params);
  MemberCtxObj member(&params);

  credential = base_credential;
  credential.A.x.data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, &precomp));
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, nullptr));

  credential = base_credential;
  credential.A.y.data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, &precomp));
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, nullptr));

  credential = base_credential;
  credential.x.data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, &precomp));
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, nullptr));
}

TEST_F(EpidMemberTest, ProvisionCredentialRejectsInvalidGroupKey) {
  Prng prng;
  GroupPubKey pub_key = this->kGrpXKey;
  FpElemStr f = this->kGrpXMember9PrivKey.f;
  MembershipCredential credential;
  credential.A = this->kGrpXMember9PrivKey.A;
  credential.gid = this->kGrpXMember9PrivKey.gid;
  credential.x = this->kGrpXMember9PrivKey.x;
  // Note: this MemberPrecomp is for the wrong group, however it should not
  // be checked in Provision because doing so would negate the performance
  // boost of using the precomp.
  MemberPrecomp precomp = this->kMemberPrecomp;
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &prng, &f, &params);
  MemberCtxObj member(&params);

  pub_key = this->kGroupPublicKey;
  pub_key.h1.x.data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, &precomp));
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, nullptr));

  pub_key = this->kGroupPublicKey;
  pub_key.h1.y.data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, &precomp));
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, nullptr));

  pub_key = this->kGroupPublicKey;
  pub_key.h2.x.data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, &precomp));
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, nullptr));

  pub_key = this->kGroupPublicKey;
  pub_key.h2.y.data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, &precomp));
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, nullptr));

  pub_key = this->kGroupPublicKey;
  pub_key.w.x[0].data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, &precomp));
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, nullptr));

  pub_key = this->kGroupPublicKey;
  pub_key.w.x[1].data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, &precomp));
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, nullptr));

  pub_key = this->kGroupPublicKey;
  pub_key.w.y[0].data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, &precomp));
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, nullptr));

  pub_key = this->kGroupPublicKey;
  pub_key.w.y[1].data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, &precomp));
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, nullptr));
}

TEST_F(EpidMemberTest, ProvisionCredentialRejectsCredentialNotInGroup) {
  Prng prng;
  GroupPubKey pub_key = this->kGrpXKey;
  FpElemStr f = this->kGrpXMember9PrivKey.f;
  MembershipCredential credential;
  MembershipCredential base_credential;
  base_credential.A = this->kGrpXMember9PrivKey.A;
  base_credential.gid = this->kGrpXMember9PrivKey.gid;
  base_credential.x = this->kGrpXMember9PrivKey.x;
  // Note: this MemberPrecomp is for the wrong group, however it should not
  // be checked in Provision because doing so would negate the performance
  // boost of using the precomp.
  MemberPrecomp precomp = this->kMemberPrecomp;
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &prng, &f, &params);
  MemberCtxObj member(&params);

  credential = base_credential;
  credential.gid.data[0] = ~credential.gid.data[0];
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, &precomp));
  EXPECT_EQ(kEpidBadArgErr, ProvisionCredentialAndStart(member, &pub_key,
                                                        &credential, nullptr));
}

TEST_F(EpidMemberTest, CanProvisionUsingMembershipCredentialPrecomp) {
  Prng prng;
  GroupPubKey pub_key = this->kGrpXKey;
  FpElemStr f = this->kGrpXMember9PrivKey.f;
  MembershipCredential credential;
  credential.A = this->kGrpXMember9PrivKey.A;
  credential.gid = this->kGrpXMember9PrivKey.gid;
  credential.x = this->kGrpXMember9PrivKey.x;
  // Note: this MemberPrecomp is for the wrong group, however it should not
  // be checked in Provision because doing so would negate the performance
  // boost of using the precomp.
  MemberPrecomp precomp = this->kMemberPrecomp;
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &prng, &f, &params);
  MemberCtxObj member(&params);
  EXPECT_EQ(kEpidNoErr, ProvisionCredentialAndStart(member, &pub_key,
                                                    &credential, &precomp));
}

TEST_F(EpidMemberTest, CanProvisionUsingMembershipCredentialNoPrecomp) {
  Prng prng;
  GroupPubKey pub_key = this->kGrpXKey;
  FpElemStr f = this->kGrpXMember9PrivKey.f;
  MembershipCredential credential;
  credential.A = this->kGrpXMember9PrivKey.A;
  credential.gid = this->kGrpXMember9PrivKey.gid;
  credential.x = this->kGrpXMember9PrivKey.x;
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &prng, &f, &params);
  MemberCtxObj member(&params);
  EXPECT_EQ(kEpidNoErr, ProvisionCredentialAndStart(member, &pub_key,
                                                    &credential, nullptr));
}

// test that create succeeds with valid IKGF given parameters
TEST_F(EpidMemberTest, CanProvisionUsingIKGFMembershipCredentialPrecomp) {
  Prng prng;
  const GroupPubKey* pub_key = reinterpret_cast<const GroupPubKey*>(
      this->kGroupPublicKeyDataIkgf.data());
  const PrivKey* priv_key =
      reinterpret_cast<const PrivKey*>(this->kMemberPrivateKeyDataIkgf.data());
  FpElemStr f = priv_key->f;
  MembershipCredential credential;
  credential.A = priv_key->A;
  credential.gid = priv_key->gid;
  credential.x = priv_key->x;
  // Note: this MemberPrecomp is for the wrong group, however it should not
  // be checked in Provision because doing so would negate the performance
  // boost of using the precomp.
  MemberPrecomp precomp = this->kMemberPrecomp;
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &prng, &f, &params);
  MemberCtxObj member(&params);
  EXPECT_EQ(kEpidNoErr, ProvisionCredentialAndStart(member, pub_key,
                                                    &credential, &precomp));
}

TEST_F(EpidMemberTest, CanProvisionUsingIKGFMembershipCredentialNoPrecomp) {
  Prng prng;
  const GroupPubKey* pub_key = reinterpret_cast<const GroupPubKey*>(
      this->kGroupPublicKeyDataIkgf.data());
  const PrivKey* priv_key =
      reinterpret_cast<const PrivKey*>(this->kMemberPrivateKeyDataIkgf.data());
  FpElemStr f = priv_key->f;
  MembershipCredential credential;
  credential.A = priv_key->A;
  credential.gid = priv_key->gid;
  credential.x = priv_key->x;
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &prng, &f, &params);
  MemberCtxObj member(&params);
  EXPECT_EQ(kEpidNoErr,
            ProvisionCredentialAndStart(member, pub_key, &credential, nullptr));
}

TEST_F(EpidMemberTest,
       ProvisionCredentialCanStoreMembershipCredentialNoPrecomp) {
  Prng prng;
  uint32_t nv_index = 0x01c10100;

  MembershipCredential const orig_credential =
      *(MembershipCredential*)&this->kGrpXMember9PrivKey;
  MembershipCredential credential;

  GroupPubKey pub_key = this->kGrpXKey;
  FpElemStr f = this->kGrpXMember9PrivKey.f;

  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &prng, &f, &params);
  MemberCtxObj member(&params);
  EXPECT_EQ(kEpidNoErr, ProvisionCredentialAndStart(member, &pub_key,
                                                    &orig_credential, nullptr));

  EXPECT_EQ(kEpidNoErr,
            EpidNvReadMembershipCredential(((MemberCtx*)member)->tpm2_ctx,
                                           nv_index, &pub_key, &credential));
  EXPECT_EQ(orig_credential, credential);
}

}  // namespace
