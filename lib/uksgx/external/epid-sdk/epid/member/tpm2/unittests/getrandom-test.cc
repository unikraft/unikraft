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
/// TPM GetRandom unit tests.
/*! \file */
#include <vector>
#include "epid/common-testhelper/epid_gtest-testhelper.h"
#include "gtest/gtest.h"

#include "epid/common-testhelper/epid2params_wrapper-testhelper.h"
#include "epid/common-testhelper/prng-testhelper.h"
#include "epid/member/tpm2/unittests/tpm2-testhelper.h"

extern "C" {
#include "epid/member/tpm2/context.h"
#include "epid/member/tpm2/getrandom.h"
}

namespace {
//////////////////////////////////////////////////////////////////////////
// Tpm2GetRandom Tests
TEST_F(EpidTpm2Test, GetRandomFailsGivenNullParameters) {
  uint8_t output[48] = {0};
  Prng my_prng;
  my_prng.set_seed(0x1234);
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);
  EXPECT_EQ(kEpidBadArgErr, Tpm2GetRandom(nullptr, 48 * 8, output));
  EXPECT_EQ(kEpidBadArgErr, Tpm2GetRandom(tpm, 48 * 8, nullptr));
}

TEST_F(EpidTpm2Test, GetRandomReturnsDifferentBufs) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);
  int length = 10;
  std::vector<uint8_t> buf1(length, (uint8_t)0);
  std::vector<uint8_t> buf2(length, (uint8_t)0);
  EXPECT_EQ(kEpidNoErr, Tpm2GetRandom(tpm, length * CHAR_BIT, buf1.data()));
  EXPECT_EQ(kEpidNoErr, Tpm2GetRandom(tpm, length * CHAR_BIT, buf2.data()));
  EXPECT_NE(buf1, buf2);
}

TEST_F(EpidTpm2Test, GetRandomCanGenerateLongStream) {
  Prng my_prng;
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tpm(&Prng::Generate, &my_prng, nullptr, epid2params);
  int length = 1000;
  std::vector<uint8_t> zeros(length, (uint8_t)0);
  std::vector<uint8_t> buf = zeros;
  EXPECT_EQ(kEpidNoErr, Tpm2GetRandom(tpm, length * CHAR_BIT, buf.data()));
  EXPECT_NE(buf, zeros);
}

}  // namespace
