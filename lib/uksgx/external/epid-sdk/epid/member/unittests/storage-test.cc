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
 * \brief Member credentials storage helper API unit tests.
 */
#include <cstring>

#include "epid/common-testhelper/epid_gtest-testhelper.h"
#include "gtest/gtest.h"

#include "epid/common-testhelper/epid2params_wrapper-testhelper.h"
#include "epid/common-testhelper/errors-testhelper.h"
#include "epid/common-testhelper/prng-testhelper.h"
#include "epid/member/tpm2/unittests/tpm2-testhelper.h"
#include "epid/member/unittests/member-testhelper.h"

extern "C" {
#include "epid/member/src/storage.h"
#include "epid/member/tpm2/nv.h"
}

namespace {

TEST_F(EpidMemberTest, NvWriteMembershipCredentialFailsGivenNullPointer) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);

  uint32_t nv_index = 0x01c10100;
  GroupPubKey const pub_key = this->kGroupPublicKey;
  // PrivKey can be trimed to MembershipCredential
  MembershipCredential const credential =
      *(MembershipCredential*)&this->kMemberPrivateKey;

  EXPECT_EQ(kEpidBadArgErr, EpidNvWriteMembershipCredential(
                                nullptr, &pub_key, &credential, nv_index));
  EXPECT_EQ(kEpidBadArgErr, EpidNvWriteMembershipCredential(
                                tpm, nullptr, &credential, nv_index));
  EXPECT_EQ(kEpidBadArgErr,
            EpidNvWriteMembershipCredential(tpm, &pub_key, nullptr, nv_index));
  // cleanup nv_index for next test
  Tpm2NvUndefineSpace(tpm, nv_index);
}

TEST_F(EpidMemberTest, NvReadMembershipCredentialFailsGivenNoCredentials) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);

  uint32_t nv_index = 0x01c10101;
  GroupPubKey pub_key = this->kGroupPublicKey;
  // PrivKey can be trimed to MembershipCredential
  MembershipCredential credential =
      *(MembershipCredential*)&this->kMemberPrivateKey;

  EXPECT_EQ(kEpidBadArgErr, EpidNvReadMembershipCredential(
                                tpm, nv_index, &pub_key, &credential));
}

TEST_F(EpidMemberTest, NvReadMembershipCredentialFailsGivenNullPointer) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);

  uint32_t nv_index = 0x01c10100;
  GroupPubKey pub_key = this->kGroupPublicKey;
  // PrivKey can be trimed to MembershipCredential
  MembershipCredential credential =
      *(MembershipCredential*)&this->kMemberPrivateKey;

  // write credentials
  EXPECT_EQ(kEpidNoErr, EpidNvWriteMembershipCredential(tpm, &pub_key,
                                                        &credential, nv_index));

  EXPECT_EQ(kEpidBadArgErr, EpidNvReadMembershipCredential(
                                nullptr, nv_index, &pub_key, &credential));
  EXPECT_EQ(kEpidBadArgErr, EpidNvReadMembershipCredential(
                                tpm, nv_index, nullptr, &credential));
  EXPECT_EQ(kEpidBadArgErr,
            EpidNvReadMembershipCredential(tpm, nv_index, &pub_key, nullptr));
  // cleanup nv_index for next test
  Tpm2NvUndefineSpace(tpm, nv_index);
}

TEST_F(EpidMemberTest, WrittenMembershipCredentialCanBeRead) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);

  uint32_t nv_index = 0x01c10100;
  GroupPubKey pub_key = this->kGroupPublicKey;
  // PrivKey can be trimed to MembershipCredential
  MembershipCredential credential_expected =
      *(MembershipCredential*)&this->kMemberPrivateKey;
  MembershipCredential credential;

  // write credentials
  EXPECT_EQ(kEpidNoErr, EpidNvWriteMembershipCredential(
                            tpm, &pub_key, &credential_expected, nv_index));

  // read credentials
  EXPECT_EQ(kEpidNoErr, EpidNvReadMembershipCredential(tpm, nv_index, &pub_key,
                                                       &credential));

  EXPECT_EQ(this->kGroupPublicKey, pub_key);
  EXPECT_EQ(credential_expected, credential);
  // cleanup nv_index for next test
  Tpm2NvUndefineSpace(tpm, nv_index);
}

}  // namespace
