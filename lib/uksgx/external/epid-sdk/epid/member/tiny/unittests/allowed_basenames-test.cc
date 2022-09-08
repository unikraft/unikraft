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
 * \brief Basename management unit tests.
 */
#ifndef SHARED
#include "epid/common-testhelper/epid_gtest-testhelper.h"
#include "gtest/gtest.h"

extern "C" {
#include "epid/member/tiny/src/allowed_basenames.h"
}

namespace {

TEST(AllowedBasenames, GetBasenamesSizeSucceeds) {
  size_t bsn_count = 10;
  size_t expected_bsn_size =
      sizeof(size_t)                     // current_bsn_number
      + sizeof(size_t)                   // max_bsn_number
      + sizeof(sha_digest) * bsn_count;  // digest * max_bsn_number
  size_t bsn_size = BasenamesGetSize(bsn_count);
  EXPECT_EQ(expected_bsn_size, bsn_size);
}

TEST(AllowedBasenames, ReportsRegisteredBasename) {
  const std::vector<uint8_t> bsn0 = {'b', 's', 'n', '0'};
  const std::vector<uint8_t> bsn1 = {'b', 's', 'n', '1'};

  AllowedBasenames* base_names = nullptr;
  std::vector<uint8_t> base_names_buf;
  size_t bsn_size = BasenamesGetSize(2);
  base_names_buf.resize(bsn_size);
  memset(base_names_buf.data(), 1, bsn_size);
  base_names = (AllowedBasenames*)&base_names_buf[0];
  InitBasenames(base_names, 2);

  EXPECT_TRUE(AllowBasename(base_names, bsn0.data(), bsn0.size()));
  EXPECT_TRUE(AllowBasename(base_names, bsn1.data(), bsn1.size()));
  EXPECT_TRUE(IsBasenameAllowed(base_names, bsn0.data(), bsn0.size()));
}

TEST(AllowedBasenames, ReportsUnregisteredBasename) {
  const std::vector<uint8_t> bsn0 = {'b', 's', 'n', '0'};
  const std::vector<uint8_t> bsn1 = {'b', 's', 'n', '1'};

  AllowedBasenames* base_names = nullptr;
  std::vector<uint8_t> base_names_buf;
  size_t bsn_size = BasenamesGetSize(1);
  base_names_buf.resize(bsn_size);
  base_names = (AllowedBasenames*)&base_names_buf[0];
  InitBasenames(base_names, 1);

  EXPECT_TRUE(AllowBasename(base_names, bsn0.data(), bsn0.size()));
  EXPECT_FALSE(IsBasenameAllowed(base_names, bsn1.data(), bsn1.size()));
}

TEST(AllowedBasenames, IsBasenameAllowedReturnsFalseWhenNoBasenamesRegistered) {
  const std::vector<uint8_t> bsn0 = {'b', 's', 'n', '0'};

  AllowedBasenames* base_names = nullptr;
  std::vector<uint8_t> base_names_buf;
  size_t bsn_size = BasenamesGetSize(1);
  base_names_buf.resize(bsn_size);
  base_names = (AllowedBasenames*)&base_names_buf[0];
  InitBasenames(base_names, 1);

  EXPECT_FALSE(IsBasenameAllowed(base_names, bsn0.data(), bsn0.size()));
}

TEST(AllowedBasenames, AllowBasenameReturnsFalseForMoreThanMaxBasenames) {
  const std::vector<uint8_t> bsn0 = {'b', 's', 'n', '0'};

  AllowedBasenames* base_names = nullptr;
  std::vector<uint8_t> base_names_buf;
  size_t bsn_size = BasenamesGetSize(5);
  base_names_buf.resize(bsn_size);
  base_names = (AllowedBasenames*)&base_names_buf[0];
  InitBasenames(base_names, 5);

  for (int n = 0; n < 5; n++) {
    EXPECT_TRUE(AllowBasename(base_names, &n, sizeof(n)));
  }
  // One more basename should not fit into storage
  EXPECT_FALSE(AllowBasename(base_names, bsn0.data(), bsn0.size()));
}

TEST(AllowedBasenames, AllowBasenameCanReturnTrue) {
  const std::vector<uint8_t> bsn0 = {'b', 's', 'n', '0'};

  AllowedBasenames* base_names = nullptr;
  std::vector<uint8_t> base_names_buf;
  size_t bsn_size = BasenamesGetSize(1);
  base_names_buf.resize(bsn_size);
  base_names = (AllowedBasenames*)&base_names_buf[0];
  InitBasenames(base_names, 1);

  EXPECT_TRUE(AllowBasename(base_names, bsn0.data(), bsn0.size()));
}

}  // namespace
#endif  // SHARED
