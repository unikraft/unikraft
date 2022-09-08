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
/// ResizeOctStr unit tests.
/*! \file */
#include <vector>

#include "gtest/gtest.h"

extern "C" {
#include "epid/member/src/resize.h"
}

#include "epid/common-testhelper/errors-testhelper.h"
namespace {
TEST(ResizeOctStr, FailsGivenNullPointer) {
  uint8_t a[3] = {0, 1, 2};
  uint8_t r[3] = {0};
  EXPECT_EQ(kEpidBadArgErr, ResizeOctStr(nullptr, sizeof(a), r, sizeof(r)));
  EXPECT_EQ(kEpidBadArgErr, ResizeOctStr(a, sizeof(a), nullptr, sizeof(r)));
}

TEST(ResizeOctStr, FailsGivenInvalidSize) {
  uint8_t a[3] = {0, 1, 2};
  uint8_t r[3] = {0};
  EXPECT_EQ(kEpidBadArgErr, ResizeOctStr(a, 0, r, sizeof(r)));
  EXPECT_EQ(kEpidBadArgErr, ResizeOctStr(a, sizeof(a), r, 0));
}

TEST(ResizeOctStr, FailsGivenResultIsTooSmall) {
  uint8_t a[3] = {0, 1, 2};
  uint8_t r[1] = {0};
  EXPECT_EQ(kEpidBadArgErr, ResizeOctStr(a, sizeof(a), r, sizeof(r)));
}

TEST(ResizeOctStr, CanShrink) {
  uint8_t a[3] = {0, 1, 2};
  std::vector<uint8_t> r(2);
  std::vector<uint8_t> r_expected = {1, 2};
  EXPECT_EQ(kEpidNoErr, ResizeOctStr(a, sizeof(a), r.data(), r.size()));
  EXPECT_EQ(r_expected, r);
}

TEST(ResizeOctStr, CanExtend) {
  uint8_t a[3] = {0, 1, 2};
  std::vector<uint8_t> r(5);
  std::vector<uint8_t> r_expected = {0, 0, 0, 1, 2};
  EXPECT_EQ(kEpidNoErr, ResizeOctStr(a, sizeof(a), r.data(), r.size()));
  EXPECT_EQ(r_expected, r);
}
}  // namespace
