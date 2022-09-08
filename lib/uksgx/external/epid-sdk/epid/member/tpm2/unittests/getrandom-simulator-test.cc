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

// Test if GetRandom can get deterministic data for known seed
TEST_F(EpidTpm2Test, GetRandomGetsExpectedDataForGivenSeed) {
  std::vector<uint8_t> expected_digest = {
      0x76, 0x90, 0x2c, 0x3b, 0xa5, 0x25, 0xd7, 0x1e, 0x66, 0x67, 0xaa, 0xb9,
      0x80, 0x50, 0x9c, 0x17, 0x65, 0x19, 0x06, 0x2a, 0x53, 0x49, 0x7d, 0x1b,
      0xe5, 0xf4, 0xec, 0xf3, 0xf0, 0x69, 0x81, 0xdc, 0x0f, 0x5a, 0x6f, 0x1c,
      0xb3, 0x78, 0xa8, 0xea, 0x6b, 0xab, 0x1d, 0xc7, 0xd6, 0x1a, 0x10, 0x1a};
  std::vector<uint8_t> output(48);
  Prng my_prng;
  my_prng.set_seed(0x1234);
  Epid2ParamsObj epid2params;
  int num_bits = (int)expected_digest.size() * 8;
  Tpm2CtxObj tpm2(&Prng::Generate, &my_prng, NULL, epid2params);
  my_prng.set_seed(0x1234);
  EXPECT_EQ(kEpidNoErr, Tpm2GetRandom(tpm2, num_bits, output.data()));
  EXPECT_EQ(expected_digest, output);
}

}  // namespace
