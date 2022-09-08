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
/// Tpm2LoadExternal unit tests.
/*! \file */

#include <stdint.h>

#include "epid/common-testhelper/epid2params_wrapper-testhelper.h"
#include "epid/common-testhelper/errors-testhelper.h"
#include "epid/common-testhelper/prng-testhelper.h"
#include "epid/member/tpm2/unittests/tpm2-testhelper.h"
#include "gtest/gtest.h"

extern "C" {
#include "epid/member/tpm2/load_external.h"
}

namespace {
//////////////////////////////////////////////////////////////////////////
// Tpm2LoadExternal Tests
TEST_F(EpidTpm2Test, LoadExternalCanLoadFValueSha384) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  FpElemStr f_str = this->kMemberFValue;
  Tpm2CtxObj ctx(&Prng::Generate, &my_prng, &f_str, epid2params);
  THROW_ON_EPIDERR(Tpm2SetHashAlg(ctx, kSha384));
  EXPECT_EQ(kEpidNoErr, Tpm2LoadExternal(ctx, &f_str));
}
TEST_F(EpidTpm2Test, LoadExternalCanLoadFValueSha512) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  FpElemStr f_str = this->kMemberFValue;
  Tpm2CtxObj ctx(&Prng::Generate, &my_prng, &f_str, epid2params);
  THROW_ON_EPIDERR(Tpm2SetHashAlg(ctx, kSha512));
  EXPECT_EQ(kEpidNoErr, Tpm2LoadExternal(ctx, &f_str));
}
TEST_F(EpidTpm2Test, LoadExternalCanLoadFValueSha512_256) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  FpElemStr f_str = this->kMemberFValue;
  Tpm2CtxObj ctx(&Prng::Generate, &my_prng, &f_str, epid2params);
  THROW_ON_EPIDERR(Tpm2SetHashAlg(ctx, kSha512_256));
  EXPECT_EQ(kEpidNoErr, Tpm2LoadExternal(ctx, &f_str));
}
}  // namespace
