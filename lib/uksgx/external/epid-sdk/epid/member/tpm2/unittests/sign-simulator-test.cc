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
/// TPM Sign unit tests.
/*! \file */
#include "gtest/gtest.h"

#include "epid/common-testhelper/epid2params_wrapper-testhelper.h"
#include "epid/common-testhelper/epid_params-testhelper.h"
#include "epid/common-testhelper/errors-testhelper.h"
#include "epid/common-testhelper/prng-testhelper.h"
#include "epid/member/tpm2/unittests/tpm2-testhelper.h"

extern "C" {
#include "epid/common/src/memory.h"
#include "epid/member/tpm2/commit.h"
#include "epid/member/tpm2/load_external.h"
#include "epid/member/tpm2/sign.h"
}

namespace {
//////////////////////////////////////////////////////////////////////////
// Tpm2Sign Tests

TEST_F(EpidTpm2Test, SignProducesKnownSignature) {
  Epid20Params params;
  EcPointObj k(&params.G1), l(&params.G1), e(&params.G1);
  FfElementObj sig_k(&params.fp), sig_s(&params.fp);
  uint16_t counter = 0;

  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, &this->kMemberFValue, epid2params);
  THROW_ON_EPIDERR(Tpm2SetHashAlg(tpm, kSha256));
  THROW_ON_EPIDERR(Tpm2LoadExternal(tpm, &this->kMemberFValue));
  THROW_ON_EPIDERR(
      Tpm2Commit(tpm, nullptr, nullptr, 0, nullptr, k, l, e, &counter));

  EXPECT_EQ(kEpidNoErr,
            Tpm2Sign(tpm, this->kDigestSha256, sizeof(this->kDigestSha256),
                     counter, sig_k, sig_s));

  Prng the_same_prng;
  FfElementObj f(&params.fp, this->kMemberFValue);
  FfElementObj t(&params.fp);
  FfElementObj r1(&params.fp), s_expected(&params.fp);
  BigNumStr zero = {0};
  THROW_ON_EPIDERR(
      FfGetRandom(params.fp, &zero, &Prng::Generate, &the_same_prng, r1));
  THROW_ON_EPIDERR(ReadFfElement(params.fp, this->kDigestSha256,
                                 sizeof(this->kDigestSha256), t));
  THROW_ON_EPIDERR(FfMul(params.fp, f, t, s_expected));
  THROW_ON_EPIDERR(FfAdd(params.fp, r1, s_expected, s_expected));
  FpElemStr s_expected_str = {0};
  THROW_ON_EPIDERR(WriteFfElement(params.fp, s_expected, &s_expected_str,
                                  sizeof(s_expected_str)));

  FpElemStr s_str = {0};
  THROW_ON_EPIDERR(WriteFfElement(params.fp, sig_s, &s_str, sizeof(s_str)));
  EXPECT_EQ(s_expected_str, s_str);
}

}  // namespace
