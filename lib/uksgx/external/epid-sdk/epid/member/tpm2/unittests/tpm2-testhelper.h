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
/// TPM fixture class.
/*! \file */
#ifndef EPID_MEMBER_TPM2_UNITTESTS_TPM2_TESTHELPER_H_
#define EPID_MEMBER_TPM2_UNITTESTS_TPM2_TESTHELPER_H_

#include <stdint.h>
#include <climits>
#include <vector>

#include "epid/common-testhelper/epid_gtest-testhelper.h"
#include "epid/member/tpm2/unittests/tpm2_wrapper-testhelper.h"
#include "gtest/gtest.h"

extern "C" {
#include "epid/common/bitsupplier.h"
#include "epid/common/src/hashsize.h"
#include "epid/member/tpm2/context.h"
}

typedef struct FqElemStr FqElemStr;
typedef struct FpElemStr FpElemStr;
typedef struct G1ElemStr G1ElemStr;

/// Test fixture class for Tpm
class EpidTpm2Test : public ::testing::Test {
 public:
  // tpm digest
  static const std::vector<uint8_t> kTpmFfElemStrData;
  static const G1ElemStr kEpidPointStr;
  static const uint8_t kDigestSha256[EPID_SHA256_DIGEST_BITSIZE / CHAR_BIT];

  static const FpElemStr kMemberFValue;
  static const G1ElemStr kP1Str;
  static const G1ElemStr kg1Str;
  static const std::vector<uint8_t> kS2Sha256;
  static const FqElemStr kY2Sha256Str;
  static const G1ElemStr kP2Sha256Str;
  static const G1ElemStr kP2Sha256ExpF;
  static const std::vector<uint8_t> kS2Sha384;
  static const FqElemStr kY2Sha384Str;
  static const G1ElemStr kP2Sha384Str;
  static const G1ElemStr kP2Sha384ExpF;
  static const std::vector<uint8_t> kS2Sha512;
  static const FqElemStr kY2Sha512Str;
  static const G1ElemStr kP2Sha512Str;
  static const G1ElemStr kP2Sha512ExpF;
  static const std::vector<uint8_t> kS2Sha512256;
  static const FqElemStr kY2Sha512256Str;
  static const G1ElemStr kP2Sha512256Str;
  static const G1ElemStr kP2Sha512256ExpF;
  /// setup called before each TEST_F starts
  virtual void SetUp() {}
  /// teardown called after each TEST_F finishes
  virtual void TearDown() {}
};

/// compares FpElemStr values
bool operator==(FpElemStr const& lhs, FpElemStr const& rhs);

/// compares G1ElemStr values
bool operator==(G1ElemStr const& lhs, G1ElemStr const& rhs);

#endif  // EPID_MEMBER_TPM2_UNITTESTS_TPM2_TESTHELPER_H_
