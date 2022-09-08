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
/// TPM2_CreatePrimary unit tests.
/*! \file */

#include "epid/common-testhelper/epid_gtest-testhelper.h"
#include "gtest/gtest.h"

#include "epid/common-testhelper/epid2params_wrapper-testhelper.h"
#include "epid/common-testhelper/errors-testhelper.h"
#include "epid/common-testhelper/prng-testhelper.h"
#include "epid/member/tpm2/unittests/tpm2-testhelper.h"

extern "C" {
#include "epid/member/tpm2/createprimary.h"
}
namespace {

TEST_F(EpidTpm2Test, CreatePrimaryFailsGivenNullParameters) {
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tss(nullptr, nullptr, nullptr, epid2params);
  G1ElemStr res = {0};
  EXPECT_EQ(kEpidBadArgErr, Tpm2CreatePrimary(tss, nullptr));
  EXPECT_EQ(kEpidBadArgErr, Tpm2CreatePrimary(nullptr, &res));
}
TEST_F(EpidTpm2Test, CreatePrimaryWorksGivenValidParameter) {
  Prng prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tss(Prng::Generate, &prng, nullptr, epid2params);
  G1ElemStr res = {0};
  THROW_ON_EPIDERR(Tpm2SetHashAlg(tss, kSha256));
  EXPECT_EQ(kEpidNoErr, Tpm2CreatePrimary(tss, &res));
}

}  // namespace
