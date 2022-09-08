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
/// TPM non volatile memory API unit tests.
/*! \file */
#include <array>
#include <cstring>
#include "gtest/gtest.h"

#include "epid/common-testhelper/epid2params_wrapper-testhelper.h"
#include "epid/common-testhelper/errors-testhelper.h"
#include "epid/common-testhelper/prng-testhelper.h"
#include "epid/member/tpm2/unittests/tpm2-testhelper.h"

extern "C" {
#include "epid/member/tpm2/nv.h"
}

bool operator==(MembershipCredential const& lhs,
                MembershipCredential const& rhs) {
  return 0 == std::memcmp(&lhs, &rhs, sizeof(lhs));
}

namespace {

TEST(NvTest, CanStoreMembershipCredential) {
  // Demonstrate NV API usage
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);
  uint32_t nv_index = 0x01000000;

  MembershipCredential const credential = {
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x01},  // group id
      {{{{0x46, 0xc9, 0x69, 0xee, 0xf4, 0x68, 0xe1, 0x5f, 0xac, 0xbf, 0xdd,
          0x77, 0xeb, 0x4c, 0xaf, 0x8a, 0x87, 0x68, 0x3f, 0x4e, 0xda, 0xf2,
          0x96, 0xec, 0x57, 0x08, 0x90, 0xe8, 0x19, 0x62, 0x54, 0xdb}}},
       {{{0x1e, 0x52, 0x23, 0x16, 0x91, 0xe4, 0xa8, 0x1d, 0x9a, 0x1b, 0x8a,
          0xad, 0x0a, 0xcf, 0x36, 0x4f, 0xae, 0x43, 0xde, 0x62, 0xff, 0xa6,
          0x4b, 0xa8, 0x16, 0x24, 0x98, 0x80, 0x82, 0x80, 0x37, 0x77}}}},  // A
      {{{0x0a, 0x30, 0xae, 0x43, 0xa1, 0xe0, 0xd7, 0xdf, 0x10, 0x5e, 0xaf,
         0xd8, 0x5a, 0x61, 0x10, 0x86, 0xd0, 0x9d, 0xb9, 0xe4, 0x46, 0xdd,
         0xb7, 0x1b, 0x00, 0x14, 0x7c, 0x6b, 0x13, 0x72, 0xc3, 0x77}}}  // x
  };
  MembershipCredential data = {0};

  // probe is nv_index is defined
  if (kEpidNoErr != Tpm2NvRead(tpm, nv_index, sizeof(data), 0, &data)) {
    EXPECT_EQ(kEpidNoErr,
              Tpm2NvDefineSpace(tpm, nv_index, sizeof(MembershipCredential)));
  }
  // write
  EXPECT_EQ(kEpidNoErr,
            Tpm2NvWrite(tpm, nv_index, sizeof(credential), 0, &credential));

  // read
  EXPECT_EQ(kEpidNoErr, Tpm2NvRead(tpm, nv_index, sizeof(data), 0, &data));
  EXPECT_EQ(credential, data);
  THROW_ON_EPIDERR(Tpm2NvUndefineSpace(tpm, nv_index));
}

TEST(NvTest, CanUseOffset) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);
  uint32_t nv_index = 0x01000000;

  std::array<uint8_t, 3> const data1_src = {1, 2, 3};
  std::array<uint8_t, 5> const data2_src = {4, 5, 6, 7, 8};
  std::array<uint8_t, 3> data1_dst = {0};
  std::array<uint8_t, 5> data2_dst = {0};

  THROW_ON_EPIDERR(
      Tpm2NvDefineSpace(tpm, nv_index, data1_src.size() + data2_src.size()));

  EXPECT_EQ(kEpidNoErr,
            Tpm2NvWrite(tpm, nv_index, data1_src.size(), 0, data1_src.data()));
  EXPECT_EQ(kEpidNoErr,
            Tpm2NvWrite(tpm, nv_index, data2_src.size(),
                        (uint16_t)data1_src.size(), data2_src.data()));

  EXPECT_EQ(kEpidNoErr,
            Tpm2NvRead(tpm, nv_index, data1_dst.size(), 0, data1_dst.data()));
  EXPECT_EQ(data1_src, data1_dst);

  EXPECT_EQ(kEpidNoErr,
            Tpm2NvRead(tpm, nv_index, data2_dst.size(),
                       (uint16_t)data1_dst.size(), data2_dst.data()));
  EXPECT_EQ(data2_src, data2_dst);
  THROW_ON_EPIDERR(Tpm2NvUndefineSpace(tpm, nv_index));
}

//////////////////////////////////////////////////////////////////////////
// Tpm2NvDefineSpace Tests
TEST(NvTest, NvDefineSpaceFailsGivenNullParameters) {
  uint32_t nv_index = 0x01000000;
  EXPECT_EQ(kEpidBadArgErr,
            Tpm2NvDefineSpace(nullptr, nv_index, sizeof(MembershipCredential)));
}

TEST(NvTest, NvDefineSpaceCanAllocateSpace) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);
  uint32_t nv_index = 0x01000000;
  EXPECT_EQ(kEpidNoErr,
            Tpm2NvDefineSpace(tpm, nv_index, sizeof(MembershipCredential)));
  THROW_ON_EPIDERR(Tpm2NvUndefineSpace(tpm, nv_index));
}

TEST(NvTest, NvDefineSpaceCatchReDefinition) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);
  uint32_t nv_index = 0x01000002;
  EXPECT_EQ(kEpidNoErr,
            Tpm2NvDefineSpace(tpm, nv_index, sizeof(MembershipCredential)));
  EXPECT_EQ(kEpidDuplicateErr,
            Tpm2NvDefineSpace(tpm, nv_index, sizeof(MembershipCredential)));
  THROW_ON_EPIDERR(Tpm2NvUndefineSpace(tpm, nv_index));
}

//////////////////////////////////////////////////////////////////////////
// Tpm2NvUndefineSpace Tests
TEST(NvTest, NvUndefineSpaceFailsGivenNullParameters) {
  uint32_t nv_index = 0x01000000;
  EXPECT_EQ(kEpidBadArgErr, Tpm2NvUndefineSpace(nullptr, nv_index));
}

TEST(NvTest, NvUndefineSpaceCanDeallocateSpace) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);
  uint32_t nv_index = 0x01000000;
  MembershipCredential data = {0};
  THROW_ON_EPIDERR(Tpm2NvDefineSpace(tpm, nv_index, sizeof(data)));
  EXPECT_EQ(kEpidNoErr, Tpm2NvUndefineSpace(tpm, nv_index));
  EXPECT_EQ(kEpidBadArgErr, Tpm2NvRead(tpm, nv_index, sizeof(data), 0, &data));
}

TEST(NvTest, NvUndefineSpaceCatchReDefinition) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);
  uint32_t nv_index = 0x01000000;
  MembershipCredential const data = {0};
  EXPECT_EQ(kEpidNoErr, Tpm2NvDefineSpace(tpm, nv_index, sizeof(data)));
  EXPECT_EQ(kEpidNoErr, Tpm2NvUndefineSpace(tpm, nv_index));
  EXPECT_EQ(kEpidBadArgErr, Tpm2NvUndefineSpace(tpm, nv_index));
}

//////////////////////////////////////////////////////////////////////////
// Tpm2NvWrite Tests
TEST(NvTest, NvWriteFailsGivenNullParameters) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);
  uint32_t nv_index = 0x01000000;
  THROW_ON_EPIDERR(
      Tpm2NvDefineSpace(tpm, nv_index, sizeof(MembershipCredential)));

  MembershipCredential const data = {0};
  EXPECT_EQ(kEpidBadArgErr,
            Tpm2NvWrite(nullptr, nv_index, sizeof(data), 0, &data));
  EXPECT_EQ(kEpidBadArgErr,
            Tpm2NvWrite(tpm, nv_index, sizeof(data), 0, nullptr));
  THROW_ON_EPIDERR(Tpm2NvUndefineSpace(tpm, nv_index));
}

TEST(NvTest, NvWriteCanWrite) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);
  uint32_t nv_index = 0x01000000;
  THROW_ON_EPIDERR(
      Tpm2NvDefineSpace(tpm, nv_index, sizeof(MembershipCredential)));

  MembershipCredential const data = {0};
  EXPECT_EQ(kEpidNoErr, Tpm2NvWrite(tpm, nv_index, sizeof(data), 0, &data));
  EXPECT_EQ(kEpidNoErr, Tpm2NvWrite(tpm, nv_index, sizeof(data) - 1, 1, &data));
  THROW_ON_EPIDERR(Tpm2NvUndefineSpace(tpm, nv_index));
}

TEST(NvTest, NvWriteFailsGivenOverflow) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);
  uint32_t nv_index = 0x01000000;
  THROW_ON_EPIDERR(
      Tpm2NvDefineSpace(tpm, nv_index, sizeof(MembershipCredential)));

  MembershipCredential const data = {0};
  EXPECT_EQ(kEpidBadArgErr, Tpm2NvWrite(tpm, nv_index, sizeof(data), 1, &data));
  EXPECT_EQ(kEpidBadArgErr,
            Tpm2NvWrite(tpm, nv_index, sizeof(data) + 1, 1, &data));
  EXPECT_EQ(kEpidBadArgErr,
            Tpm2NvWrite(tpm, nv_index, 1, sizeof(MembershipCredential), &data));
  THROW_ON_EPIDERR(Tpm2NvUndefineSpace(tpm, nv_index));
}

TEST(NvTest, NvWriteFailsGivenInvalidLength) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);
  uint32_t nv_index = 0x01000000;
  THROW_ON_EPIDERR(
      Tpm2NvDefineSpace(tpm, nv_index, sizeof(MembershipCredential)));

  MembershipCredential const data = {0};
  EXPECT_EQ(kEpidBadArgErr, Tpm2NvWrite(tpm, nv_index, 0, 0, &data));
  THROW_ON_EPIDERR(Tpm2NvUndefineSpace(tpm, nv_index));
}

TEST(NvTest, NvWriteFailsGivenIndexUndefined) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);
  uint32_t nv_index = 0x01000003;

  MembershipCredential const data = {0};
  EXPECT_EQ(kEpidBadArgErr, Tpm2NvWrite(tpm, nv_index, 1, 0, &data));
}

//////////////////////////////////////////////////////////////////////////
// Tpm2NvRead Tests
TEST(NvTest, NvReadFailsGivenNullParameters) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);
  uint32_t nv_index = 0x01000000;
  THROW_ON_EPIDERR(
      Tpm2NvDefineSpace(tpm, nv_index, sizeof(MembershipCredential)));

  MembershipCredential data = {0};
  EXPECT_EQ(kEpidBadArgErr,
            Tpm2NvRead(nullptr, nv_index, sizeof(data), 0, &data));
  EXPECT_EQ(kEpidBadArgErr,
            Tpm2NvRead(tpm, nv_index, sizeof(data), 0, nullptr));
  THROW_ON_EPIDERR(Tpm2NvUndefineSpace(tpm, nv_index));
}

TEST(NvTest, NvReadCanRead) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);
  uint32_t nv_index = 0x01000000;
  std::vector<uint8_t> const data_src = {1, 2, 3, 4, 5, 6, 7, 8};
  std::vector<uint8_t> data_dst(data_src.size());
  THROW_ON_EPIDERR(Tpm2NvDefineSpace(tpm, nv_index, data_src.size()));
  THROW_ON_EPIDERR(
      Tpm2NvWrite(tpm, nv_index, data_src.size(), 0, data_src.data()));

  EXPECT_EQ(kEpidNoErr, Tpm2NvRead(tpm, nv_index, 3, 0, data_dst.data()));
  EXPECT_EQ(kEpidNoErr, Tpm2NvRead(tpm, nv_index, data_src.size() - 3, 3,
                                   data_dst.data() + 3));
  THROW_ON_EPIDERR(Tpm2NvUndefineSpace(tpm, nv_index));
  EXPECT_EQ(data_src, data_dst);
}

TEST(NvTest, NvReadFailIfWriteWasNotCalled) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);
  uint32_t nv_index = 0x01000000;
  THROW_ON_EPIDERR(
      Tpm2NvDefineSpace(tpm, nv_index, sizeof(MembershipCredential)));

  MembershipCredential data = {0};
  EXPECT_EQ(kEpidBadArgErr, Tpm2NvRead(tpm, nv_index, sizeof(data), 0, &data));
  THROW_ON_EPIDERR(Tpm2NvUndefineSpace(tpm, nv_index));
}

TEST(NvTest, NvReadFailsGivenOverflow) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);
  uint32_t nv_index = 0x01000000;
  THROW_ON_EPIDERR(
      Tpm2NvDefineSpace(tpm, nv_index, sizeof(MembershipCredential)));

  MembershipCredential data = {0};
  EXPECT_EQ(kEpidBadArgErr, Tpm2NvRead(tpm, nv_index, sizeof(data), 1, &data));
  EXPECT_EQ(kEpidBadArgErr,
            Tpm2NvRead(tpm, nv_index, sizeof(data) + 1, 0, &data));
  EXPECT_EQ(kEpidBadArgErr,
            Tpm2NvRead(tpm, nv_index, 1, sizeof(MembershipCredential), &data));
  THROW_ON_EPIDERR(Tpm2NvUndefineSpace(tpm, nv_index));
}

TEST(NvTest, NvReadFailsGivenInvalidLength) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);
  uint32_t nv_index = 0x01000000;
  THROW_ON_EPIDERR(
      Tpm2NvDefineSpace(tpm, nv_index, sizeof(MembershipCredential)));

  MembershipCredential data = {0};
  EXPECT_EQ(kEpidBadArgErr, Tpm2NvRead(tpm, nv_index, 0, 0, &data));
  THROW_ON_EPIDERR(Tpm2NvUndefineSpace(tpm, nv_index));
}

TEST(NvTest, NvReadFailsGivenIndexUndefined) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);
  uint32_t nv_index = 0x01000003;

  MembershipCredential data = {0};
  EXPECT_EQ(kEpidBadArgErr, Tpm2NvRead(tpm, nv_index, 1, 0, &data));
}

}  // namespace
