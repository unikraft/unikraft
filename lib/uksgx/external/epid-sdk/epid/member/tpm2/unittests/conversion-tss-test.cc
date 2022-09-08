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
/// TPM mapper unit tests.
/*! \file */

#include <cstring>
#include "epid/common-testhelper/epid_gtest-testhelper.h"
#include "epid/common-testhelper/errors-testhelper.h"
#include "epid/member/tpm2/unittests/tpm2-testhelper.h"
#include "gtest/gtest.h"
extern "C" {
#include "epid/member/tpm2/ibm_tss/conversion.h"
}

bool operator==(OctStr256 const& lhs, OctStr256 const& rhs) {
  return 0 == std::memcmp(&lhs, &rhs, sizeof(OctStr256));
}

bool operator==(G1ElemStr const& epid_point, TPM2B_ECC_POINT const& tpm_point) {
  if (std::memcmp(&epid_point.x, tpm_point.point.x.t.buffer,
                  sizeof(G1ElemStr) / 2) != 0)
    return false;

  return 0 == std::memcmp(&epid_point.y, tpm_point.point.y.t.buffer,
                          sizeof(G1ElemStr) / 2);
}

////////////////////////////////////////////////
// EpidtoTpm2HashAlg
TEST_F(EpidTpm2Test, EpidtoTpm2HashAlgWorksCorrectly) {
  EXPECT_EQ(TPM_ALG_SHA256, EpidtoTpm2HashAlg(kSha256));
  EXPECT_EQ(TPM_ALG_SHA384, EpidtoTpm2HashAlg(kSha384));
  EXPECT_EQ(TPM_ALG_SHA512, EpidtoTpm2HashAlg(kSha512));
  EXPECT_EQ(TPM_ALG_NULL, EpidtoTpm2HashAlg(kSha512_256));
  EXPECT_EQ(TPM_ALG_NULL, EpidtoTpm2HashAlg(kInvalidHashAlg));
  EXPECT_EQ(TPM_ALG_NULL, EpidtoTpm2HashAlg((HashAlg)10));
}
////////////////////////////////////////////////
// Tpm2toEpidHashAlg
TEST_F(EpidTpm2Test, Tpm2toEpidHashAlgWorksCorrectly) {
  EXPECT_EQ(kSha256, Tpm2toEpidHashAlg(TPM_ALG_SHA256));
  EXPECT_EQ(kSha384, Tpm2toEpidHashAlg(TPM_ALG_SHA384));
  EXPECT_EQ(kSha512, Tpm2toEpidHashAlg(TPM_ALG_SHA512));
  EXPECT_EQ(kInvalidHashAlg, Tpm2toEpidHashAlg(TPM_ALG_NULL));
  EXPECT_EQ(kInvalidHashAlg, Tpm2toEpidHashAlg((TPMI_ALG_HASH)0x0020));
}
////////////////////////////////////////////////
// ReadTpm2FfElement
TEST_F(EpidTpm2Test, ReadTpm2FfElementFailsGivenNullPointer) {
  TPM2B_ECC_PARAMETER ecc_parameter = {0};
  OctStr256 stub = {0};
  EXPECT_EQ(kEpidBadArgErr, ReadTpm2FfElement(nullptr, &ecc_parameter));
  EXPECT_EQ(kEpidBadArgErr, ReadTpm2FfElement(&stub, nullptr));
}

TEST_F(EpidTpm2Test, ReadTpm2FfElementMapsGivenValidArguments) {
  TPM2B_ECC_PARAMETER ecc_parameter = {0};
  EXPECT_EQ(kEpidNoErr,
            ReadTpm2FfElement((OctStr256*)this->kTpmFfElemStrData.data(),
                              &ecc_parameter));
  EXPECT_EQ((uint16_t)this->kTpmFfElemStrData.size(), ecc_parameter.b.size);
  EXPECT_EQ(*(OctStr256*)this->kTpmFfElemStrData.data(),
            *(OctStr256*)ecc_parameter.b.buffer);
}

////////////////////////////////////////////////
// WriteTpm2FfElement

TEST_F(EpidTpm2Test, WriteTpm2FfElementFailsGivenNullPointer) {
  TPM2B_ECC_PARAMETER ecc_parameter = {0};
  OctStr256 result = {0};
  EXPECT_EQ(kEpidBadArgErr, WriteTpm2FfElement(nullptr, &result));
  EXPECT_EQ(kEpidBadArgErr, WriteTpm2FfElement(&ecc_parameter, nullptr));
}

TEST_F(EpidTpm2Test, WriteTpm2FfElementfailsGivenSmallBufSize) {
  TPM2B_ECC_PARAMETER ecc_parameter = {0};
  OctStr256 result = {0};
  THROW_ON_EPIDERR(ReadTpm2FfElement((OctStr256*)this->kTpmFfElemStrData.data(),
                                     &ecc_parameter));

  ecc_parameter.b.size++;
  EXPECT_EQ(kEpidBadArgErr, WriteTpm2FfElement(&ecc_parameter, &result));
}

TEST_F(EpidTpm2Test, WriteTpm2FfElementWorksGivenLargerBufSize) {
  TPM2B_ECC_PARAMETER ecc_parameter = {0};
  OctStr256 result = {0};
  THROW_ON_EPIDERR(ReadTpm2FfElement((OctStr256*)this->kTpmFfElemStrData.data(),
                                     &ecc_parameter));

  std::vector<uint8_t> expected(ecc_parameter.b.size);
  for (size_t i = 1; i < expected.size(); ++i) {
    expected[i] = this->kTpmFfElemStrData[i - 1];
  }
  ecc_parameter.b.size--;
  expected[0] = 0x00;
  EXPECT_EQ(kEpidNoErr, WriteTpm2FfElement(&ecc_parameter, &result));

  EXPECT_EQ(*(OctStr256*)expected.data(), result);
}

TEST_F(EpidTpm2Test, WriteTpm2FfElementWorksGivenValidArguments) {
  TPM2B_ECC_PARAMETER ecc_parameter = {0};
  OctStr256 result = {0};
  THROW_ON_EPIDERR(ReadTpm2FfElement((OctStr256*)this->kTpmFfElemStrData.data(),
                                     &ecc_parameter));

  EXPECT_EQ(kEpidNoErr, WriteTpm2FfElement(&ecc_parameter, &result));
  EXPECT_EQ(*(OctStr256*)this->kTpmFfElemStrData.data(), result);
}

////////////////////////////////////////////////
//  ReadTpmFromEcPoint
TEST_F(EpidTpm2Test, ReadTpm2EcPointFailsGivenNullPointer) {
  TPM2B_ECC_POINT tpm_point;

  EXPECT_EQ(kEpidBadArgErr, ReadTpm2EcPoint(nullptr, &tpm_point));
  EXPECT_EQ(kEpidBadArgErr, ReadTpm2EcPoint(&this->kEpidPointStr, nullptr));
}

TEST_F(EpidTpm2Test, ReadTpm2EcPointWorksGivenValidArguments) {
  TPM2B_ECC_POINT tpm_point;

  EXPECT_EQ(kEpidNoErr, ReadTpm2EcPoint(&this->kEpidPointStr, &tpm_point));

  EXPECT_EQ(this->kEpidPointStr, tpm_point);
}

////////////////////////////////////////////////
//  WriteTpm2EcPoint
TEST_F(EpidTpm2Test, WriteTpm2EcPointFailsGivenNullPointer) {
  TPM2B_ECC_POINT tpm_point;
  G1ElemStr str = {0};
  EXPECT_EQ(kEpidBadArgErr, WriteTpm2EcPoint(nullptr, &str));
  EXPECT_EQ(kEpidBadArgErr, WriteTpm2EcPoint(&tpm_point, nullptr));
}

TEST_F(EpidTpm2Test, WriteTpm2EcPointWorksGivenValidArguments) {
  TPM2B_ECC_POINT tpm_point;
  G1ElemStr str = {0};
  EXPECT_EQ(kEpidNoErr, ReadTpm2EcPoint(&this->kEpidPointStr, &tpm_point));

  EXPECT_EQ(kEpidNoErr, WriteTpm2EcPoint(&tpm_point, &str));

  EXPECT_EQ(str, tpm_point);
}
