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
/// IsKeyValid unit tests.
/*! \file  */
#include "epid/common-testhelper/epid_gtest-testhelper.h"
#include "gtest/gtest.h"

extern "C" {
#include "epid/member/src/validatekey.h"
}

#include "epid/common-testhelper/epid2params_wrapper-testhelper.h"
#include "epid/common-testhelper/epid_params-testhelper.h"
#include "epid/common-testhelper/errors-testhelper.h"
#include "epid/common-testhelper/prng-testhelper.h"
#include "epid/member/unittests/member-testhelper.h"

namespace {

////////////////////////////////////////////////
//  EpidMemberIsKeyValid
TEST_F(EpidMemberTest, EpidMemberIsKeyValidFailsGivenNullPointer) {
  // create
  Prng my_prng;
  Epid2ParamsObj epid2params;
  MemberCtxObj member(&Prng::Generate, &my_prng);

  // provision
  HashAlg hash_alg = kSha256;
  const GroupPubKey pub_key = this->kGroupPublicKey;
  const PrivKey priv_key = this->kMemberPrivateKey;

  THROW_ON_EPIDERR(EpidMemberSetHashAlg(member, hash_alg));
  EXPECT_EQ(kEpidNoErr,
            EpidProvisionKey(member, &this->kGroupPublicKey,
                             &this->kMemberPrivateKey, &this->kMemberPrecomp));

  EXPECT_FALSE(EpidMemberIsKeyValid(nullptr, &priv_key.A, &priv_key.x,
                                    &pub_key.h1, &pub_key.w));
  EXPECT_FALSE(EpidMemberIsKeyValid(member, nullptr, &priv_key.x, &pub_key.h1,
                                    &pub_key.w));
  EXPECT_FALSE(EpidMemberIsKeyValid(member, &priv_key.A, nullptr, &pub_key.h1,
                                    &pub_key.w));
  EXPECT_FALSE(EpidMemberIsKeyValid(member, &priv_key.A, &priv_key.x, nullptr,
                                    &pub_key.w));
  EXPECT_FALSE(EpidMemberIsKeyValid(member, &priv_key.A, &priv_key.x,
                                    &pub_key.h1, nullptr));
}

TEST_F(EpidMemberTest, EpidMemberIsKeyValidSucceedsForSha256) {
  // create
  Prng my_prng;
  Epid2ParamsObj epid2params;
  MemberCtxObj member(&Prng::Generate, &my_prng);

  // provision
  HashAlg hash_alg = kSha256;
  const GroupPubKey pub_key = this->kGroupPublicKey;
  const PrivKey priv_key = this->kMemberPrivateKey;

  THROW_ON_EPIDERR(EpidMemberSetHashAlg(member, hash_alg));
  EXPECT_EQ(kEpidNoErr,
            EpidProvisionKey(member, &this->kGroupPublicKey,
                             &this->kMemberPrivateKey, &this->kMemberPrecomp));
  EXPECT_TRUE(EpidMemberIsKeyValid(member, &priv_key.A, &priv_key.x,
                                   &pub_key.h1, &pub_key.w));
}

TEST_F(EpidMemberTest, EpidMemberIsKeyValidFailsGivenIncorrectKeys) {
  // create
  Prng my_prng;
  Epid2ParamsObj epid2params;
  MemberCtxObj member(&Prng::Generate, &my_prng);

  // provision
  HashAlg hash_alg = kSha256;
  GroupPubKey pub_key = this->kGroupPublicKey;
  PrivKey priv_key = this->kMemberPrivateKey;

  THROW_ON_EPIDERR(EpidMemberSetHashAlg(member, hash_alg));
  EXPECT_EQ(kEpidNoErr,
            EpidProvisionKey(member, &this->kGroupPublicKey,
                             &this->kMemberPrivateKey, &this->kMemberPrecomp));

  // check the key is valid
  EXPECT_TRUE(EpidMemberIsKeyValid(member, &priv_key.A, &priv_key.x,
                                   &pub_key.h1, &pub_key.w));

  // check key is invalid with incorrect data
  PrivKey tmp_priv_key = priv_key;
  tmp_priv_key.A.x.data.data[31] -= 1;
  EXPECT_FALSE(EpidMemberIsKeyValid(member, &tmp_priv_key.A, &priv_key.x,
                                    &pub_key.h1, &pub_key.w));

  tmp_priv_key = priv_key;
  tmp_priv_key.A.y.data.data[31] -= 1;
  EXPECT_FALSE(EpidMemberIsKeyValid(member, &tmp_priv_key.A, &tmp_priv_key.x,
                                    &pub_key.h1, &pub_key.w));

  tmp_priv_key = priv_key;
  tmp_priv_key.x.data.data[31] -= 1;
  EXPECT_FALSE(EpidMemberIsKeyValid(member, &tmp_priv_key.A, &tmp_priv_key.x,
                                    &pub_key.h1, &pub_key.w));

  GroupPubKey tmp_pub_key = pub_key;
  tmp_pub_key.h1.x.data.data[31] -= 1;
  EXPECT_FALSE(EpidMemberIsKeyValid(member, &tmp_priv_key.A, &priv_key.x,
                                    &tmp_pub_key.h1, &tmp_pub_key.w));

  tmp_pub_key = pub_key;
  tmp_pub_key.h1.y.data.data[31] -= 1;
  EXPECT_FALSE(EpidMemberIsKeyValid(member, &tmp_priv_key.A, &priv_key.x,
                                    &tmp_pub_key.h1, &tmp_pub_key.w));

  tmp_pub_key = pub_key;
  tmp_pub_key.w.x->data.data[31] -= 1;
  EXPECT_FALSE(EpidMemberIsKeyValid(member, &tmp_priv_key.A, &priv_key.x,
                                    &tmp_pub_key.h1, &tmp_pub_key.w));

  tmp_pub_key = pub_key;
  tmp_pub_key.w.y->data.data[31] -= 1;
  EXPECT_FALSE(EpidMemberIsKeyValid(member, &tmp_priv_key.A, &priv_key.x,
                                    &tmp_pub_key.h1, &tmp_pub_key.w));
}

TEST_F(EpidMemberTest,
       PROTECTED_EpidMemberIsKeyValidSucceedsByCredentialForSha256_EPS0) {
  // create
  Prng my_prng;
  Epid2ParamsObj epid2params;
  MemberCtxObj member(&Prng::Generate, &my_prng);

  // provision
  HashAlg hash_alg = kSha256;
  const GroupPubKey eps0_pub_key = {
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x01},  // group id
      {{{{0xb3, 0x6f, 0xff, 0x81, 0xe2, 0x1b, 0x17, 0xeb, 0x3d, 0x75, 0x3d,
          0x61, 0x7e, 0x27, 0xb0, 0xcb, 0xd0, 0x6d, 0x8f, 0x9d, 0x64, 0xce,
          0xe3, 0xce, 0x43, 0x4c, 0x62, 0xfd, 0xb5, 0x80, 0xe0, 0x99}}},
       {{{0x3a, 0x07, 0x56, 0x80, 0xe0, 0x88, 0x59, 0xa4, 0xfd, 0xb5, 0xb7,
          0x9d, 0xe9, 0x4d, 0xae, 0x9c, 0xee, 0x3d, 0x66, 0x42, 0x82, 0x45,
          0x7e, 0x7f, 0xd8, 0x69, 0x3e, 0xa1, 0x74, 0xf4, 0x59, 0xee}}}},  // h1
      {{{{0xd2, 0x74, 0x2e, 0x9f, 0x63, 0xc2, 0x51, 0x8e, 0xd5, 0xdb, 0xca,
          0x1c, 0x54, 0x74, 0x10, 0x7b, 0xdc, 0x99, 0xed, 0x42, 0xd5, 0x5b,
          0xa7, 0x04, 0x29, 0x66, 0x61, 0x63, 0xbc, 0xdd, 0x7f, 0xe1}}},
       {{{0x76, 0x5d, 0xc0, 0x6e, 0xe3, 0x14, 0xac, 0x72, 0x48, 0x12, 0x0a,
          0xa6, 0xe8, 0x5b, 0x08, 0x7b, 0xda, 0x3f, 0x51, 0x7d, 0xde, 0x4c,
          0xea, 0xcb, 0x93, 0xa5, 0x6e, 0xcc, 0xe7, 0x8e, 0x10, 0x84}}}},  // h2
      {{{{{0xbd, 0x19, 0x5a, 0x95, 0xe2, 0x0f, 0xca, 0x1c, 0x50, 0x71, 0x94,
           0x51, 0x40, 0x1b, 0xa5, 0xb6, 0x78, 0x87, 0x53, 0xf6, 0x6a, 0x95,
           0xca, 0xc6, 0x8d, 0xcd, 0x36, 0x88, 0x07, 0x28, 0xe8, 0x96}}},
        {{{0xca, 0x78, 0x11, 0x5b, 0xb8, 0x6a, 0xe7, 0xe5, 0xa6, 0x65, 0x7a,
           0x68, 0x15, 0xd7, 0x75, 0xf8, 0x24, 0x14, 0xcf, 0xd1, 0x0f, 0x6c,
           0x56, 0xf5, 0x22, 0xd9, 0xfd, 0xe0, 0xe2, 0xf4, 0xb3, 0xa1}}}},
       {{{{0x90, 0x21, 0xa7, 0xe0, 0xe8, 0xb3, 0xc7, 0x25, 0xbc, 0x07, 0x72,
           0x30, 0x5d, 0xee, 0xf5, 0x6a, 0x89, 0x88, 0x46, 0xdd, 0x89, 0xc2,
           0x39, 0x9c, 0x0a, 0x3b, 0x58, 0x96, 0x57, 0xe4, 0xf3, 0x3c}}},
        {{{0x79, 0x51, 0x69, 0x36, 0x1b, 0xb6, 0xf7, 0x05, 0x5d, 0x0a, 0x88,
           0xdb, 0x1f, 0x3d, 0xea, 0xa2, 0xba, 0x6b, 0xf0, 0xda, 0x8e, 0x25,
           0xc6, 0xad, 0x83, 0x7d, 0x3e, 0x31, 0xee, 0x11, 0x40, 0xa9}}}}}  // w
  };

  const PrivKey eps0_priv_key = {
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x01},  // group id
      {{{{0xae, 0x5a, 0x18, 0x8d, 0xc8, 0xa9, 0xe2, 0x5c, 0xdf, 0xef, 0x62,
          0x6b, 0x34, 0xdf, 0x0d, 0xf2, 0xf6, 0xcf, 0x6a, 0x4d, 0x68, 0x88,
          0xc8, 0x12, 0x08, 0xae, 0xb6, 0x30, 0x54, 0xdf, 0xa7, 0xdc}}},
       {{{0xec, 0x39, 0x86, 0x7b, 0x5c, 0x5e, 0x28, 0x17, 0x5b, 0xfe, 0x93,
          0xa1, 0x31, 0x8a, 0x82, 0x99, 0xb0, 0x84, 0x8c, 0x90, 0xc5, 0x54,
          0x2c, 0x6d, 0xff, 0x75, 0xcf, 0x05, 0x6e, 0x2b, 0x6c, 0xf3}}}},  // A
      {0x0a, 0x30, 0xae, 0x43, 0xa1, 0xe0, 0xd7, 0xdf, 0x10, 0x5e, 0xaf,
       0xd8, 0x5a, 0x61, 0x10, 0x86, 0xd0, 0x9d, 0xb9, 0xe4, 0x46, 0xdd,
       0xb7, 0x1b, 0x00, 0x14, 0x7c, 0x6b, 0x13, 0x72, 0xc3, 0x77},  // x
      {0x7a, 0x57, 0x41, 0x5b, 0x85, 0x44, 0x0e, 0x2b, 0xb3, 0xcc, 0xa7,
       0x99, 0x6d, 0x19, 0x79, 0x45, 0x04, 0xb8, 0x94, 0x07, 0x47, 0x14,
       0xed, 0x8d, 0xf4, 0x1e, 0x7d, 0xa0, 0x17, 0xc5, 0xc4, 0x10}  // f
  };

  THROW_ON_EPIDERR(EpidMemberSetHashAlg(member, hash_alg));
  EXPECT_TRUE(EpidMemberIsKeyValid(member, &eps0_priv_key.A, &eps0_priv_key.x,
                                   &eps0_pub_key.h1, &eps0_pub_key.w));
}
}  // namespace
