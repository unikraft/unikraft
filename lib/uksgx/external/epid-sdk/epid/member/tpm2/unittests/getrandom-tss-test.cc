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
/// TPM Context unit tests.
/*! \file */

#include <limits.h>
#include <stdint.h>
#include "gtest/gtest.h"

#include "epid/common-testhelper/epid2params_wrapper-testhelper.h"
#include "epid/member/tpm2/unittests/tpm2-testhelper.h"

extern "C" {
#include "epid/member/tpm2/context.h"
#include "epid/member/tpm2/getrandom.h"
}

namespace {

TEST_F(EpidTpm2Test, GetRandomOnTssReturnsDifferentBufs) {
  Epid2ParamsObj epid2params;
  Tpm2CtxObj tss(nullptr, nullptr, nullptr, epid2params);
  size_t length = 10;
  std::vector<uint8_t> buf1(length, (uint8_t)0);
  std::vector<uint8_t> buf2(length, (uint8_t)0);
  EXPECT_EQ(kEpidNoErr, Tpm2GetRandom(tss, length * CHAR_BIT, buf1.data()));
  EXPECT_EQ(kEpidNoErr, Tpm2GetRandom(tss, length * CHAR_BIT, buf2.data()));
  EXPECT_NE(buf1, buf2);
}

}  // namespace
