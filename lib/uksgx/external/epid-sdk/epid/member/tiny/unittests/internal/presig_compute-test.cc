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
 * \brief tiny EpidMemberComputePreSig unit tests.
 */

#ifndef SHARED
#include "gtest/gtest.h"

extern "C" {
#include "epid/member/tiny/src/presig_compute.h"
}

#include "epid/common-testhelper/prng-testhelper.h"
#include "epid/member/tiny/unittests/member-testhelper.h"

namespace {

TEST_F(EpidMemberTest, SuccessfullyComputePresig) {
  Prng my_prng;
  MemberCtxObj member(this->kGroupPublicKey, this->kMemberPrivateKey,
                      this->kMemberPrecomp, &Prng::Generate, &my_prng);

  PreComputedSignatureData presig = {0};
  EXPECT_EQ(kEpidNoErr, EpidMemberComputePreSig(member, &presig));
}

}  // namespace
#endif  // SHARED
