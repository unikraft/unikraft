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
 * \brief ComputePreSig unit tests.
 */
#include <algorithm>
#include <cstring>
#include <limits>
#include <vector>

#include "epid/common-testhelper/epid_gtest-testhelper.h"
#include "gtest/gtest.h"

extern "C" {
#include "epid/member/api.h"
}

#include "epid/common-testhelper/errors-testhelper.h"
#include "epid/common-testhelper/prng-testhelper.h"
#include "epid/member/tiny/unittests/member-testhelper.h"

/// Count of elements in array
#define COUNT_OF(A) (sizeof(A) / sizeof((A)[0]))

namespace {

///////////////////////////////////////////////////////////////////////
// EpidAddPreSigs
TEST_F(EpidMemberTest, DISABLED_AddPreSigsFailsGivenNullPointer) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);

  EXPECT_EQ(kEpidBadArgErr, EpidAddPreSigs(nullptr, 1));
}

TEST_F(EpidMemberTest, DISABLED_AddPreSigsFailsGivenHugeNumberOfPreSigs) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);

  // number_presigs = 0x80..01 of size equal to sizeof(size_t)
  EXPECT_NE(kEpidNoErr, EpidAddPreSigs(member, (SIZE_MAX >> 1) + 2));
}

TEST_F(EpidMemberTest,
       DISABLED_AddPreSigsComputesSpecifiedNumberOfPresigsIfInputPresigsNull) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);

  ASSERT_EQ(kEpidNoErr, EpidAddPreSigs(member, 2));
  ASSERT_EQ(kEpidNoErr, EpidAddPreSigs(member, 1));
  // request to generate 0 pre-computed signatures do nothing
  ASSERT_EQ(kEpidNoErr, EpidAddPreSigs(member, 0));
  EXPECT_EQ((size_t)3, EpidGetNumPreSigs(member));
}

TEST_F(EpidMemberTest,
       DISABLED_AddPreSigsAddsCorrectNumberOfPresigsGivenValidInput) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);

  const size_t presigs1_added = 2;
  const size_t presigs2_added = 3;

  // add
  ASSERT_EQ(kEpidNoErr, EpidAddPreSigs(member, presigs1_added));
  // extend
  ASSERT_EQ(kEpidNoErr, EpidAddPreSigs(member, presigs2_added));
  // add empty pre-computed signatures array does not affect internal pool
  ASSERT_EQ(kEpidNoErr, EpidAddPreSigs(member, 0));
  EXPECT_EQ(presigs1_added + presigs2_added, EpidGetNumPreSigs(member));
}

///////////////////////////////////////////////////////////////////////
// EpidGetNumPreSigs
TEST_F(EpidMemberTest, DISABLED_GetNumPreSigsReturnsZeroGivenNullptr) {
  EXPECT_EQ((size_t)0, EpidGetNumPreSigs(nullptr));
}

TEST_F(EpidMemberTest, DISABLED_NumPreSigsForNewleyCreatedContextIsZero) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);

  EXPECT_EQ((size_t)0, EpidGetNumPreSigs(member));
}

TEST_F(EpidMemberTest, DISABLED_GetNumPreSigsReturnsNumberOfAddedPresigs) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);

  const size_t presigs_added = 5;

  THROW_ON_EPIDERR(EpidAddPreSigs(member, presigs_added));
  EXPECT_EQ(presigs_added, EpidGetNumPreSigs(member));
}

}  // namespace
