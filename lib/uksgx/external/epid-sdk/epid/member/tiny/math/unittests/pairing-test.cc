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
/// Unit tests of pairing implementation.
/*! \file */

#include <gtest/gtest.h>

#include "epid/member/tiny/math/unittests/cmp-testhelper.h"

extern "C" {
#include "epid/member/tiny/math/mathtypes.h"
#include "epid/member/tiny/math/pairing.h"
}

namespace {

////////////////////////////////////////////////////////////////////////
// PairingCompute

TEST(TinyPairingTest, PairingComputeWorks) {
  const EccPointFq a = {{0xf67c8483, 0xbaceaf8d, 0xfcaa5567, 0x8ac94411,
                         0x7d91c375, 0x0fcaa603, 0x210f0997, 0xd7e2f937},
                        {0x0f93424b, 0x88e246d6, 0x4859033f, 0x3ff08258,
                         0xf613c944, 0xaf6cd00a, 0x9f0d2673, 0x04b7a6ff}};
  const EccPointFq2 b = {{{0xa3c3fd4c, 0xad5323dc, 0x3770e217, 0x91a68d51,
                           0xdcfbda35, 0x6fb2b5c1, 0xbc72b09c, 0x3f4cb52d},
                          {0xb6f87404, 0xb8fb89d1, 0xb0c1d7f9, 0x49dff54f,
                           0x7fb08a69, 0x5e8ef4ce, 0xcb35f350, 0x90fa4fa2}},
                         {{0xc801fb5b, 0xfbbd005f, 0x1620b9cc, 0x444c5486,
                           0x083e43a1, 0x80f44b97, 0x62f3175a, 0xefc66005},
                          {0xeb618b01, 0x448994a3, 0x3aca949c, 0xdfba9b6f,
                           0x35140ec9, 0xa2aa865a, 0xe20470eb, 0xc16e2b46}}};
  const Fq12Elem expected = {
      {{{0x50ab7bc6, 0xd28d33ca, 0xa7de4ce1, 0x162996ed, 0xad5ee231, 0x4fc0a501,
         0x468be932, 0xba101ff6},
        {0x14d36207, 0x34c44c84, 0xdfa22b9e, 0x1f4d1fc0, 0xcb0454f2, 0xc4077c42,
         0xebde30b6, 0x15eb79f4}},
       {{0xf91d7519, 0xc456caad, 0xb908d0d3, 0x31b0be8c, 0x1c0def81, 0x80e14649,
         0x4657b6e7, 0xf18b84d1},
        {0xec73b557, 0xf8acbc05, 0x2e5f0a7e, 0xf485e0eb, 0x6fd516b6, 0xb7190100,
         0xc1fa4e50, 0x3fee7c43}},
       {{0xb3ebe0e5, 0xc572866a, 0xa10be392, 0x2d6f653d, 0x138bb1b6, 0x87cf70ba,
         0xbbef8650, 0xe3b31829},
        {0x4ba31303, 0x8d1afe6e, 0xe7138780, 0x36a08173, 0xcdc3182a, 0x1ecb0486,
         0xd5a961a5, 0xda0e5787}}},
      {{{0x0b13b454, 0xa62f5fbf, 0xa4bed641, 0xd3632805, 0xe8010941, 0x72cebb51,
         0x8aaa0095, 0x669e804d},
        {0xe49b7149, 0x8fc69d31, 0x956d88ab, 0x265c926b, 0x3bb2f1d4, 0xbfc206ea,
         0x6f29d6da, 0x5b5065dc}},
       {{0x0ff0848e, 0x1f3fdf5c, 0xb3533098, 0x1a434003, 0xc80d2a60, 0x4ac6aa4a,
         0xc99fbca8, 0xe0ce978f},
        {0x3357d172, 0xf5f8a018, 0x5908b3da, 0xe540395c, 0xb654a226, 0x89cef58e,
         0x47786d9f, 0x5a5d41d2}},
       {{0x788a3989, 0x49a5f719, 0x5a0042e3, 0x92f94303, 0x1bd0e6c8, 0x81c5ac15,
         0xe8a05ec8, 0xbbba6ced},
        {0x6d17dc8e, 0xaf351b75, 0xba6e2c36, 0x2a04ea26, 0x421e1737, 0x16fd0bfe,
         0x4bb33376, 0x32aebf4d}}}};

  PairingState ps;
  PairingInit(&ps);
  Fq12Elem res = {0};
  PairingCompute(&res, &a, &b, &ps);
  EXPECT_EQ(expected, res);
}

}  // namespace
