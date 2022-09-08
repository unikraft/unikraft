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
 * \brief Provision compressed unit tests.
 */
#include <cstring>
#include <vector>

#include "epid/common-testhelper/epid_gtest-testhelper.h"
#include "gtest/gtest.h"

#include "epid/common-testhelper/errors-testhelper.h"
#include "epid/common-testhelper/mem_params-testhelper.h"
#include "epid/common-testhelper/prng-testhelper.h"
#include "epid/member/tiny/unittests/member-testhelper.h"

extern "C" {
#include "epid/member/api.h"
}

namespace {

EpidStatus ProvisionCompressedAndStart(
    MemberCtx* ctx, GroupPubKey const* pub_key,
    CompressedPrivKey const* compressed_privkey,
    MemberPrecomp const* precomp_str) {
  EpidStatus sts;
  sts = EpidProvisionCompressed(ctx, pub_key, compressed_privkey, precomp_str);
  if (sts != kEpidNoErr) {
    return sts;
  }
  sts = EpidMemberStartup(ctx);
  return sts;
}

TEST_F(EpidMemberTest, DISABLED_ProvisionCompressedFailsGivenNullParameters) {
  Prng prng;
  GroupPubKey pub_key = this->kGrpXKey;
  CompressedPrivKey priv_key = this->kGrpXMember9CompressedKey;
  // Note: this MemberPrecomp is for the wrong group, however it should not
  // be checked in Provision because doing so would negate the performance
  // boost of using the precomp.
  MemberPrecomp precomp = this->kMemberPrecomp;
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &prng, nullptr, &params);
  MemberCtxObj member(&params);
  EXPECT_EQ(kEpidBadArgErr,
            EpidProvisionCompressed(nullptr, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            EpidProvisionCompressed(member, nullptr, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            EpidProvisionCompressed(member, &pub_key, nullptr, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            EpidProvisionCompressed(nullptr, &pub_key, &priv_key, nullptr));
  EXPECT_EQ(kEpidBadArgErr,
            EpidProvisionCompressed(member, nullptr, &priv_key, nullptr));
  EXPECT_EQ(kEpidBadArgErr,
            EpidProvisionCompressed(member, &pub_key, nullptr, nullptr));
}

TEST_F(EpidMemberTest,
       DISABLED_ProvisionCompressedSucceedsGivenValidParameters) {
  Prng prng;
  GroupPubKey pub_key = this->kGrpXKey;
  CompressedPrivKey priv_key = this->kGrpXMember9CompressedKey;
  // Note: this MemberPrecomp is for the wrong group, however it should not
  // be checked in Provision because doing so would negate the performance
  // boost of using the precomp.
  MemberPrecomp precomp = this->kMemberPrecomp;
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &prng, nullptr, &params);
  MemberCtxObj member(&params);
  EXPECT_EQ(kEpidNoErr,
            EpidProvisionCompressed(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidNoErr,
            EpidProvisionCompressed(member, &pub_key, &priv_key, nullptr));
}

TEST_F(EpidMemberTest, DISABLED_ProvisionCompressedFailsForInvalidGroupPubKey) {
  Prng prng;
  GroupPubKey pub_key = this->kGrpXKey;
  CompressedPrivKey priv_key = this->kGrpXMember9CompressedKey;
  // Note: this MemberPrecomp is for the wrong group, however it should not
  // be checked in Provision because doing so would negate the performance
  // boost of using the precomp.
  MemberPrecomp precomp = this->kMemberPrecomp;
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &prng, nullptr, &params);
  MemberCtxObj member(&params);

  pub_key = this->kGrpXKey;
  pub_key.h1.x.data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionCompressedAndStart(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionCompressedAndStart(member, &pub_key, &priv_key, nullptr));

  pub_key = this->kGrpXKey;
  pub_key.h1.y.data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionCompressedAndStart(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionCompressedAndStart(member, &pub_key, &priv_key, nullptr));

  pub_key = this->kGrpXKey;
  pub_key.w.x[0].data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionCompressedAndStart(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionCompressedAndStart(member, &pub_key, &priv_key, nullptr));

  pub_key = this->kGrpXKey;
  pub_key.w.x[1].data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionCompressedAndStart(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionCompressedAndStart(member, &pub_key, &priv_key, nullptr));

  pub_key = this->kGrpXKey;
  pub_key.w.y[0].data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionCompressedAndStart(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionCompressedAndStart(member, &pub_key, &priv_key, nullptr));

  pub_key = this->kGrpXKey;
  pub_key.w.y[1].data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionCompressedAndStart(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionCompressedAndStart(member, &pub_key, &priv_key, nullptr));
}

TEST_F(EpidMemberTest, DISABLED_ProvisionCompressedFailsForInvalidPrivateKey) {
  Prng prng;
  GroupPubKey pub_key = this->kGrpXKey;
  CompressedPrivKey priv_key = this->kGrpXMember9CompressedKey;
  // Note: this MemberPrecomp is for the wrong group, however it should not
  // be checked in Provision because doing so would negate the performance
  // boost of using the precomp.
  MemberPrecomp precomp = this->kMemberPrecomp;
  MemberParams params = {0};
  SetMemberParams(&Prng::Generate, &prng, nullptr, &params);
  MemberCtxObj member(&params);

  priv_key = this->kGrpXMember9CompressedKey;
  priv_key.ax.data.data[0]++;
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionCompressedAndStart(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionCompressedAndStart(member, &pub_key, &priv_key, nullptr));

  priv_key = this->kGrpXMember9CompressedKey;
  priv_key.seed.data[0]++;
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionCompressedAndStart(member, &pub_key, &priv_key, &precomp));
  EXPECT_EQ(kEpidBadArgErr,
            ProvisionCompressedAndStart(member, &pub_key, &priv_key, nullptr));
}

}  // namespace
