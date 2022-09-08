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
 * \brief Member C++ wrapper interface.
 */
#ifndef EPID_MEMBER_UNITTESTS_MEMBER_TESTHELPER_H_
#define EPID_MEMBER_UNITTESTS_MEMBER_TESTHELPER_H_

#include <stdint.h>
#include <vector>

#include "epid/common-testhelper/epid_gtest-testhelper.h"
#include "epid/common-testhelper/member_wrapper-testhelper.h"
#include "gtest/gtest.h"

extern "C" {
#include "epid/member/api.h"
}

typedef struct G1ElemStr G1ElemStr;
/// compares G1ElemStr values
bool operator==(G1ElemStr const& lhs, G1ElemStr const& rhs);

/// compares MembershipCredential values
bool operator==(MembershipCredential const& lhs,
                MembershipCredential const& rhs);

/// compares GroupPubKey values
bool operator==(GroupPubKey const& lhs, GroupPubKey const& rhs);

/// Test fixture class for EpidMember
class EpidMemberTest : public ::testing::Test {
 public:
  /// test data
  static const GroupPubKey kGroupPublicKey;
  /// test data
  static const PrivKey kMemberPrivateKey;
  /// test data
  static const std::vector<uint8_t> kGroupPublicKeyDataIkgf;
  /// test data
  static const std::vector<uint8_t> kMemberPrivateKeyDataIkgf;
  /// test data
  static const MemberPrecomp kMemberPrecomp;
  /// test data
  static const std::vector<uint8_t> kGrp01Member0SigTest1Sha256;
  /// test data
  static const std::vector<uint8_t> kGrp01Member0SigTest1Sha384;
  /// test data
  static const std::vector<uint8_t> kGrp01Member0SigTest1Sha512;
  /// test data
  static const std::vector<uint8_t> kTest1Msg;
  /// signature based revocation list with 50 entries
  static std::vector<uint8_t> kSigRlData;
  /// signature based revocation list with 5 entries
  static std::vector<uint8_t> kSigRl5EntryData;
  /// a message
  static const std::vector<uint8_t> kMsg0;
  /// a message
  static const std::vector<uint8_t> kMsg1;
  /// a basename
  static const std::vector<uint8_t> kBsn0;
  /// a basename
  static const std::vector<uint8_t> kBsn1;
  /// a data with bytes [0,255]
  static const std::vector<uint8_t> kData_0_255;
  /// a group key in group X
  static const GroupPubKey kGrpXKey;
  /// a member 0 private key in group X
  static const PrivKey kGrpXMember0PrivKey;
  /// a member private key in group X revoked in SigRl
  static const PrivKey kGrpXSigrevokedMember0PrivKey;
  /// a SigRl of group X
  static const std::vector<uint8_t> kGrpXSigRl;
  /// a SigRl with single entry of group X
  static const std::vector<uint8_t> kGrpXSigRlSingleEntry;
  /// a compressed private key in group X
  static const CompressedPrivKey kGrpXMember9CompressedKey;
  /// a private key in group X
  static const PrivKey kGrpXMember9PrivKey;

  /// a group key in group Y
  static const GroupPubKey kGrpYKey;
  /// a compressed private key in group Y
  static const CompressedPrivKey kGrpYMember9CompressedKey;

  /// value "1" represented as an octstr constant
  static const OctStr32 kOctStr32_1;

  /// EPS specific group public key
  static const GroupPubKey kEps0GroupPublicKey;
  /// EPS specific member private key
  static const PrivKey kEps0MemberPrivateKey;

  /// setup called before each TEST_F starts
  virtual void SetUp() {}
  /// teardown called after each TEST_F finishes
  virtual void TearDown() {}
};

#endif  // EPID_MEMBER_UNITTESTS_MEMBER_TESTHELPER_H_
