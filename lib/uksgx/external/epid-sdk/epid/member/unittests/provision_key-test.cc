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
 * \brief Provision key unit tests.
 */
#include <cstring>
#include <vector>

#include "epid/common-testhelper/epid_gtest-testhelper.h"
#include "gtest/gtest.h"

#include "epid/common-testhelper/errors-testhelper.h"
#include "epid/common-testhelper/mem_params-testhelper.h"
#include "epid/common-testhelper/prng-testhelper.h"
#include "epid/member/unittests/member-testhelper.h"

extern "C" {
#include "epid/member/api.h"
#include "epid/member/src/context.h"
#include "epid/member/src/storage.h"
}

namespace {

EpidStatus ProvisionBulkAndStart(MemberCtx* ctx, GroupPubKey const* pub_key,
                                 PrivKey const* priv_key,
                                 MemberPrecomp const* precomp_str) {
  EpidStatus sts;
  sts = EpidProvisionKey(ctx, pub_key, priv_key, precomp_str);
  if (sts != kEpidNoErr) {
    return sts;
  }
  sts = EpidMemberStartup(ctx);
  return sts;
}

TEST_F(EpidMemberTest, ProvisionBulkFailsGivenNullParameters) {
  Prng prng;
  GroupPubKey pub_key = this->kGroupPublicKey;
  PrivKey priv_key = this->kMemberPrivateKey;
  MemberPrecomp precomp = this->kMemberPrecomp;
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &prng, nullptr, &params);
  MemberCtxObj member(&params);
  EXPECT_EQ(kEpidBadArgErr,
            EpidProvisionKey(nullptr, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            EpidProvisionKey(member, nullptr, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            EpidProvisionKey(member, &pub_key, nullptr, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            EpidProvisionKey(nullptr, &pub_key, &priv_key, nullptr));
  EXPECT_EQ(kEpidBadArgErr,
            EpidProvisionKey(member, nullptr, &priv_key, nullptr));
  EXPECT_EQ(kEpidBadArgErr,
            EpidProvisionKey(member, &pub_key, nullptr, nullptr));
}

TEST_F(EpidMemberTest, ProvisionBulkSucceedsGivenValidParameters) {
  Prng prng;
  GroupPubKey pub_key = this->kGroupPublicKey;
  PrivKey priv_key = this->kMemberPrivateKey;
  MemberPrecomp precomp = this->kMemberPrecomp;
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &prng, nullptr, &params);
  MemberCtxObj member(&params);
  EXPECT_EQ(kEpidNoErr,
            EpidProvisionKey(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidNoErr, EpidProvisionKey(member, &pub_key, &priv_key, nullptr));
}

// test that create succeeds with valid IKGF given parameters
TEST_F(EpidMemberTest, ProvisionBulkSucceedsGivenValidParametersUsingIKGFData) {
  Prng prng;
  const GroupPubKey pub_key = {
#include "epid/common-testhelper/testdata/ikgf/groupa/pubkey.inc"
  };
  const PrivKey priv_key = {
#include "epid/common-testhelper/testdata/ikgf/groupa/member0/mprivkey.inc"
  };

  const MemberPrecomp precomp = {
#include "epid/common-testhelper/testdata/ikgf/groupa/member0/mprecomp.inc"
  };
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &prng, nullptr, &params);
  MemberCtxObj member(&params);
  EXPECT_EQ(kEpidNoErr,
            EpidProvisionKey(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidNoErr, EpidProvisionKey(member, &pub_key, &priv_key, nullptr));
}

TEST_F(EpidMemberTest, ProvisionBulkFailsForInvalidGroupPubKey) {
  Prng prng;

  GroupPubKey pub_key = this->kGroupPublicKey;
  PrivKey priv_key = this->kMemberPrivateKey;
  MemberPrecomp precomp = this->kMemberPrecomp;
  MemberParams params = {0};

  SetMemberParams(&Prng::Generate, &prng, nullptr, &params);
  MemberCtxObj member(&params);

  pub_key = this->kGroupPublicKey;
  pub_key.h1.x.data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, nullptr));

  pub_key = this->kGroupPublicKey;
  pub_key.h1.y.data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, nullptr));

  pub_key = this->kGroupPublicKey;
  pub_key.h2.x.data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, nullptr));

  pub_key = this->kGroupPublicKey;
  pub_key.h2.y.data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, nullptr));

  pub_key = this->kGroupPublicKey;
  pub_key.w.x[0].data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, nullptr));

  pub_key = this->kGroupPublicKey;
  pub_key.w.x[1].data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, nullptr));

  pub_key = this->kGroupPublicKey;
  pub_key.w.y[0].data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, nullptr));

  pub_key = this->kGroupPublicKey;
  pub_key.w.y[1].data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, nullptr));
}

TEST_F(EpidMemberTest, ProvisionBulkFailsForInvalidF) {
  Prng prng;
  FpElemStr f = {
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
  };
  GroupPubKey pub_key = this->kGroupPublicKey;
  PrivKey priv_key = this->kMemberPrivateKey;
  MemberPrecomp precomp = this->kMemberPrecomp;
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &prng, nullptr, &params);
  MemberCtxObj member(&params);

  priv_key = this->kMemberPrivateKey;
  priv_key.f = f;
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, nullptr));
}

TEST_F(EpidMemberTest, ProvisionBulkFailsForInvalidPrivateKey) {
  Prng prng;

  GroupPubKey pub_key = this->kGroupPublicKey;
  PrivKey priv_key = this->kMemberPrivateKey;
  MemberPrecomp precomp = this->kMemberPrecomp;
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &prng, nullptr, &params);
  MemberCtxObj member(&params);

  priv_key = this->kMemberPrivateKey;
  priv_key.A.x.data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, nullptr));

  priv_key = this->kMemberPrivateKey;
  priv_key.A.y.data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, nullptr));
}

TEST_F(EpidMemberTest, ProvisionBulkCanStoreMembershipCredential) {
  Prng prng;
  uint32_t nv_index = 0x01c10100;

  GroupPubKey pub_key = this->kGroupPublicKey;
  PrivKey priv_key = this->kMemberPrivateKey;
  MembershipCredential const orig_credential{priv_key.gid, priv_key.A,
                                             priv_key.x};
  MembershipCredential credential;
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &prng, &priv_key.f, &params);
  MemberCtxObj member(&params);
  EXPECT_EQ(kEpidNoErr,
            ProvisionBulkAndStart(member, &pub_key, &priv_key, nullptr));

  EXPECT_EQ(kEpidNoErr,
            EpidNvReadMembershipCredential(((MemberCtx*)member)->tpm2_ctx,
                                           nv_index, &pub_key, &credential));
  EXPECT_EQ(orig_credential, credential);
}

}  // namespace
