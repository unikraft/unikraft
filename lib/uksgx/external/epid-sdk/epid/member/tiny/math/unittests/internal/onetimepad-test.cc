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
/// Unit tests for OneTimePad.
/*! \file */

#include <gtest/gtest.h>

#include "epid/member/tiny/math/unittests/onetimepad.h"

namespace {

TEST(OneTimePadTest, GenerateFailsWhenDefaultConstructedWithoutInit) {
  OneTimePad otp;
  EXPECT_EQ(0u, otp.BitsConsumed());
  std::vector<unsigned int> actual({0, 0});
  EXPECT_NE(0, OneTimePad::Generate(actual.data(), 8, &otp));
}

TEST(OneTimePadTest, GeneratesCorrectDataWhenConstructedWithUint8s) {
  std::vector<unsigned int> actual1({0, 0});
  std::vector<unsigned int> actual2({0, 0});
  const std::vector<unsigned int> expected1({0x07050301, 0});
  const std::vector<unsigned int> expected2({0x0e0c0a09, 0});
  OneTimePad otp({0x01, 0x03, 0x05, 0x07, 0x09, 0x0a, 0x0c, 0x0e});
  EXPECT_EQ(0u, otp.BitsConsumed());
  EXPECT_EQ(0, OneTimePad::Generate(actual1.data(), 32, &otp));
  EXPECT_EQ(expected1, actual1);
  EXPECT_EQ(32u, otp.BitsConsumed());
  EXPECT_EQ(0, OneTimePad::Generate(actual2.data(), 32, &otp));
  EXPECT_EQ(expected2, actual2);
  EXPECT_EQ(64u, otp.BitsConsumed());
}

TEST(OneTimePadTest, GeneratesCorrectDataWhenInitilizedWithUint8s) {
  std::vector<unsigned int> actual1({0, 0});
  std::vector<unsigned int> actual2({0, 0});
  const std::vector<unsigned int> expected1({0x07050301, 0});
  const std::vector<unsigned int> expected2({0x0e0c0a09, 0});
  OneTimePad otp;
  otp.InitUint8({0x01, 0x03, 0x05, 0x07, 0x09, 0x0a, 0x0c, 0x0e});
  EXPECT_EQ(0u, otp.BitsConsumed());
  EXPECT_EQ(0, OneTimePad::Generate(actual1.data(), 32, &otp));
  EXPECT_EQ(expected1, actual1);
  EXPECT_EQ(32u, otp.BitsConsumed());
  EXPECT_EQ(0, OneTimePad::Generate(actual2.data(), 32, &otp));
  EXPECT_EQ(expected2, actual2);
  EXPECT_EQ(64u, otp.BitsConsumed());
}
TEST(OneTimePadTest, GeneratesCorrectDataWhenInitilizedWithUint32s) {
  std::vector<uint32_t> actual({0x00});
  const std::vector<uint32_t> expected({0x01});
  std::vector<uint32_t> actual2({0x00});
  const std::vector<uint32_t> expected2({0x01020304});
  OneTimePad otp({0x01, 0x03, 0x05, 0x07, 0x09, 0x0a, 0x0c, 0x0e});
  otp.InitUint32({0x01, 0x01020304});
  EXPECT_EQ(0u, otp.BitsConsumed());
  EXPECT_EQ(0, OneTimePad::Generate(actual.data(), 32, &otp));
  EXPECT_EQ(32u, otp.BitsConsumed());
  EXPECT_EQ(expected, actual);
  EXPECT_EQ(0, OneTimePad::Generate(actual2.data(), 32, &otp));
  EXPECT_EQ(expected2, actual2);
  EXPECT_EQ(64u, otp.BitsConsumed());
}
TEST(OneTimePadTest, GeneratesSingleBytesCorrectly) {
  OneTimePad otp({0x01, 0x03, 0x05, 0x07, 0x09, 0x0a, 0x0c, 0x0e});
  std::vector<uint8_t> expected1({0x01, 0x00, 0x00, 0x00});
  std::vector<uint8_t> expected2({0x03, 0x00, 0x00, 0x00});
  std::vector<uint8_t> expected3({0x05, 0x00, 0x00, 0x00});
  std::vector<uint8_t> actual({0, 0, 0, 0});
  EXPECT_EQ(0, OneTimePad::Generate((uint32_t*)actual.data(), 8, &otp));
  EXPECT_EQ(8u, otp.BitsConsumed());
  EXPECT_EQ(expected1, actual);
  EXPECT_EQ(0, OneTimePad::Generate((uint32_t*)actual.data(), 8, &otp));
  EXPECT_EQ(16u, otp.BitsConsumed());
  EXPECT_EQ(expected2, actual);
  EXPECT_EQ(0, OneTimePad::Generate((uint32_t*)actual.data(), 8, &otp));
  EXPECT_EQ(24u, otp.BitsConsumed());
  EXPECT_EQ(expected3, actual);
}

TEST(OneTimePadTest, GenerateRejectsNullPtr) {
  OneTimePad otp(8);
  EXPECT_NE(0, OneTimePad::Generate(nullptr, 32, &otp));
}
TEST(OneTimePadTest, GenerateRejectsNegativeBits) {
  OneTimePad otp(8);
  std::vector<unsigned int> actual({0, 0});
  EXPECT_NE(0, OneTimePad::Generate(actual.data(), -32, &otp));
}
TEST(OneTimePadTest, GenerateRejectsZeroBits) {
  OneTimePad otp(8);
  std::vector<unsigned int> actual({0, 0});
  EXPECT_NE(0, OneTimePad::Generate(actual.data(), 0, &otp));
}
TEST(OneTimePadTest, GenerateRejectsTooLargeRequest) {
  OneTimePad otp(8);
  std::vector<unsigned int> actual({0, 0});
  EXPECT_EQ(0, OneTimePad::Generate(actual.data(), 32, &otp));
  EXPECT_EQ(0, OneTimePad::Generate(actual.data(), 32, &otp));
  EXPECT_NE(0, OneTimePad::Generate(actual.data(), 32, &otp));
}

TEST(OneTimePadTest, GenerateRejectsUnsupportedBitRequest) {
  OneTimePad otp(8);
  std::vector<unsigned int> actual({0, 0});
  EXPECT_NE(0, OneTimePad::Generate(actual.data(), 31, &otp));
}

TEST(OneTimePadTest, GenerateCoddlesDevelopersWhoDoNotCheckReturnValues) {
  OneTimePad otp(4);
  uint32_t word;
  EXPECT_EQ(0, OneTimePad::Generate(&word, 32, &otp));
  EXPECT_NE(0, OneTimePad::Generate(&word, 32, &otp));
  EXPECT_THROW(OneTimePad::Generate(&word, 32, &otp), std::runtime_error);
}

}  // namespace
