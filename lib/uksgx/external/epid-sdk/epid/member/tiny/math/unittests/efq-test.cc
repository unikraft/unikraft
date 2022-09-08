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
/// Unit tests of EFq implementation.
/*! \file */

#include <gtest/gtest.h>
#include <cstring>
#include <random>

#include "epid/member/tiny/math/unittests/cmp-testhelper.h"
#include "epid/member/tiny/math/unittests/onetimepad.h"

extern "C" {
#include "epid/member/tiny/math/efq.h"
#include "epid/member/tiny/math/fq.h"
#include "epid/member/tiny/math/mathtypes.h"
#include "epid/member/tiny/math/vli.h"
}

namespace {

////////////////////////////////////////////////////////////////////////
// EFqMulSSCM
TEST(TinyEFqTest, EFqMulSSCMWorks) {
  const EccPointJacobiFq expected = {
      {0xacd848b1, 0xe3d60553, 0x69271cd1, 0x7cf6090e, 0x16c63bcd, 0xdb0c6cf0,
       0x2ab60283, 0x38fc72a8},
      {0xf7e0f7d1, 0x6b0cf194, 0x5cf18c77, 0x82a4b960, 0xa6c40a30, 0xf0b3b20f,
       0x5f1a477b, 0x7e2a6668},
      {0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
       0x00000000, 0x00000000}};
  const EccPointJacobiFq left = {
      {0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357, 0x4780716c,
       0xffd94b0f, 0x5e643124},
      {0x5e9cb480, 0x6d4aaf9c, 0x99f1f606, 0x222d89b0, 0x30b79eab, 0x88844bd6,
       0xc65e7c30, 0x4830c4ec},
      {0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
       0x00000000, 0x00000000}};
  const FpElem power = {0x0adf9a12, 0x5cbc9ef4, 0x91762984, 0xa08a22fb,
                        0x52a6fddf, 0xf51e743e, 0x7b47b24b, 0x389f865f};

  EccPointJacobiFq actual = {0};

  EFqMulSSCM(&actual, &left, &power);
  EXPECT_EQ(expected, actual);
}

TEST(TinyEFqTest, EFqMulSSCMWorksInPlace) {
  const EccPointJacobiFq expected = {
      {0xacd848b1, 0xe3d60553, 0x69271cd1, 0x7cf6090e, 0x16c63bcd, 0xdb0c6cf0,
       0x2ab60283, 0x38fc72a8},
      {0xf7e0f7d1, 0x6b0cf194, 0x5cf18c77, 0x82a4b960, 0xa6c40a30, 0xf0b3b20f,
       0x5f1a477b, 0x7e2a6668},
      {0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
       0x00000000, 0x00000000}};
  EccPointJacobiFq left = {{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c,
                            0xafa65357, 0x4780716c, 0xffd94b0f, 0x5e643124},
                           {0x5e9cb480, 0x6d4aaf9c, 0x99f1f606, 0x222d89b0,
                            0x30b79eab, 0x88844bd6, 0xc65e7c30, 0x4830c4ec},
                           {0x00000001, 0x00000000, 0x00000000, 0x00000000,
                            0x00000000, 0x00000000, 0x00000000, 0x00000000}};
  const FpElem power = {0x0adf9a12, 0x5cbc9ef4, 0x91762984, 0xa08a22fb,
                        0x52a6fddf, 0xf51e743e, 0x7b47b24b, 0x389f865f};

  EFqMulSSCM(&left, &left, &power);
  EXPECT_EQ(expected, left);
}
////////////////////////////////////////////////////////////////////////
// EFqMultiExp

TEST(TinyEFqTest, EFqAffineExpWorks) {
  EccPointFq efq_left = {0};
  EccPointFq efq_right = {{{0x76abb18a, 0x92c0f7b9, 0x2c1a37e0, 0x7fdf6ca1,
                            0xe3401760, 0x66eb7d52, 0x918d50a7, 0x12a65bd6}},
                          {{0x1f3a8b42, 0xc8d3127f, 0x6d96f7cd, 0xea6fe2f6,
                            0x0ac0b46b, 0x557a5f30, 0xaf075250, 0x786528cb}}};

  EccPointFq efq_expect = {{{0x79171d4b, 0x0f5ac562, 0x3b911d03, 0x0992a2dc,
                             0x3130dd84, 0x16344c80, 0x436ca13c, 0xaf1d9819}},
                           {{0x742268cd, 0x070b4ac7, 0x7f2b13b7, 0xe167da7f,
                             0xd84d16af, 0x9e824ebe, 0x6b5dc0f0, 0x90bd1aa3}}};

  FpElem fp_exp = {3};

  EFqAffineExp(&efq_left, &efq_right, &fp_exp);

  EXPECT_EQ(efq_expect, efq_left);
}
TEST(TinyEFqTest, EFqAffineExpWorksInPlace) {
  EccPointFq efq_right = {{{0x76abb18a, 0x92c0f7b9, 0x2c1a37e0, 0x7fdf6ca1,
                            0xe3401760, 0x66eb7d52, 0x918d50a7, 0x12a65bd6}},
                          {{0x1f3a8b42, 0xc8d3127f, 0x6d96f7cd, 0xea6fe2f6,
                            0x0ac0b46b, 0x557a5f30, 0xaf075250, 0x786528cb}}};

  EccPointFq efq_expect = {{{0x79171d4b, 0x0f5ac562, 0x3b911d03, 0x0992a2dc,
                             0x3130dd84, 0x16344c80, 0x436ca13c, 0xaf1d9819}},
                           {{0x742268cd, 0x070b4ac7, 0x7f2b13b7, 0xe167da7f,
                             0xd84d16af, 0x9e824ebe, 0x6b5dc0f0, 0x90bd1aa3}}};

  FpElem fp_exp = {3};

  EFqAffineExp(&efq_right, &efq_right, &fp_exp);

  EXPECT_EQ(efq_expect, efq_right);
}
////////////////////////////////////////////////////////////////////////
// EFqAffineMultiExp
TEST(TinyEFqTest, EFqAffineMultiExpWorks) {
  // eFq2^3*eFq2^3
  EccPointFq efq_left = {0};
  EccPointFq efq_right = {{{0x76abb18a, 0x92c0f7b9, 0x2c1a37e0, 0x7fdf6ca1,
                            0xe3401760, 0x66eb7d52, 0x918d50a7, 0x12a65bd6}},
                          {{0x1f3a8b42, 0xc8d3127f, 0x6d96f7cd, 0xea6fe2f6,
                            0x0ac0b46b, 0x557a5f30, 0xaf075250, 0x786528cb}}};

  EccPointFq efq_expect = {{{0x1a178986, 0xeeb06c08, 0x857d79f0, 0x246d0ed6,
                             0xed31a3b2, 0x7bf832a0, 0x81a8a27b, 0x3d0cab80}},
                           {{0xfe2a1509, 0x41cda394, 0x42b33efb, 0x2811fa22,
                             0x0ad56486, 0xe52b1a56, 0xf6dc881c, 0x6fee593f}}};

  FpElem fp_exp = {3};

  EFqAffineMultiExp(&efq_left, &efq_right, &fp_exp, &efq_right, &fp_exp);

  EXPECT_EQ(efq_expect, efq_left);
}
////////////////////////////////////////////////////////////////////////
// EFqMultiExp
TEST(TinyEFqTest, EFqMultiExpWorks) {
  EccPointFq efq_left = {0};
  EccPointFq efq_right = {{{0x76abb18a, 0x92c0f7b9, 0x2c1a37e0, 0x7fdf6ca1,
                            0xe3401760, 0x66eb7d52, 0x918d50a7, 0x12a65bd6}},
                          {{0x1f3a8b42, 0xc8d3127f, 0x6d96f7cd, 0xea6fe2f6,
                            0x0ac0b46b, 0x557a5f30, 0xaf075250, 0x786528cb}}};

  EccPointFq efq_expect = {{{0x1a178986, 0xeeb06c08, 0x857d79f0, 0x246d0ed6,
                             0xed31a3b2, 0x7bf832a0, 0x81a8a27b, 0x3d0cab80}},
                           {{0xfe2a1509, 0x41cda394, 0x42b33efb, 0x2811fa22,
                             0x0ad56486, 0xe52b1a56, 0xf6dc881c, 0x6fee593f}}};

  EccPointJacobiFq efqj_left = {0};
  EccPointJacobiFq efqj_right;

  FpElem fp_exp = {3};

  EFqFromAffine(&efqj_right, &efq_right);
  EFqMultiExp(&efqj_left, &efqj_right, &fp_exp, &efqj_right, &fp_exp);
  EFqToAffine(&efq_left, &efqj_left);

  EXPECT_EQ(efq_expect, efq_left);
}
////////////////////////////////////////////////////////////////////////
// EFqAffineAdd
TEST(TinyEFqTest, EFqAffineAddWorks) {
  EccPointFq efq_left = {{{0x76abb18a, 0x92c0f7b9, 0x2c1a37e0, 0x7fdf6ca1,
                           0xe3401760, 0x66eb7d52, 0x918d50a7, 0x12a65bd6}},
                         {{0x1f3a8b42, 0xc8d3127f, 0x6d96f7cd, 0xea6fe2f6,
                           0x0ac0b46b, 0x557a5f30, 0xaf075250, 0x786528cb}}};

  EccPointFq const efq_expect = {
      {{0x7305f782, 0x84101559, 0xe065da04, 0xfd373fcf, 0x31d2f725, 0x9b420bdc,
        0x622ac9c3, 0xba0ef378}},
      {{0x97b2adf0, 0x2935c14b, 0xfc6415b0, 0x48ba036a, 0x61ca7383, 0xcff7f03d,
        0x23a2c3e6, 0x0df0d4a5}}};

  EFqAffineAdd(&efq_left, &efq_left, &efq_left);
  EXPECT_EQ(efq_expect, efq_left);
}
////////////////////////////////////////////////////////////////////////
// EFqAffineDbl
TEST(TinyEFqTest, EFqAffineDblWorks) {
  EccPointFq efq_left = {{{0x76abb18a, 0x92c0f7b9, 0x2c1a37e0, 0x7fdf6ca1,
                           0xe3401760, 0x66eb7d52, 0x918d50a7, 0x12a65bd6}},
                         {{0x1f3a8b42, 0xc8d3127f, 0x6d96f7cd, 0xea6fe2f6,
                           0x0ac0b46b, 0x557a5f30, 0xaf075250, 0x786528cb}}};

  EccPointFq const efq_expect = {
      {{0x7305f782, 0x84101559, 0xe065da04, 0xfd373fcf, 0x31d2f725, 0x9b420bdc,
        0x622ac9c3, 0xba0ef378}},
      {{0x97b2adf0, 0x2935c14b, 0xfc6415b0, 0x48ba036a, 0x61ca7383, 0xcff7f03d,
        0x23a2c3e6, 0x0df0d4a5}}};

  EFqAffineDbl(&efq_left, &efq_left);
  EXPECT_EQ(efq_expect, efq_left);
}
////////////////////////////////////////////////////////////////////////
// EFqDbl
TEST(TinyEFqTest, EFqDblWorks) {
  EccPointJacobiFq efqj_left = {
      {{0x76abb18a, 0x92c0f7b9, 0x2c1a37e0, 0x7fdf6ca1, 0xe3401760, 0x66eb7d52,
        0x918d50a7, 0x12a65bd6}},
      {{0x1f3a8b42, 0xc8d3127f, 0x6d96f7cd, 0xea6fe2f6, 0x0ac0b46b, 0x557a5f30,
        0xaf075250, 0x786528cb}},
      {{0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000}}};

  EccPointJacobiFq const efqj_expect = {
      {{0x7305f782, 0x84101559, 0xe065da04, 0xfd373fcf, 0x31d2f725, 0x9b420bdc,
        0x622ac9c3, 0xba0ef378}},
      {{0x97b2adf0, 0x2935c14b, 0xfc6415b0, 0x48ba036a, 0x61ca7383, 0xcff7f03d,
        0x23a2c3e6, 0x0df0d4a5}},
      {{0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000}}};
  EFqDbl(&efqj_left, &efqj_left);
  EXPECT_EQ(efqj_expect, efqj_left);
}
////////////////////////////////////////////////////////////////////////
// EFqAdd
TEST(TinyEFqTest, EFqAddWorks) {
  EccPointJacobiFq efqj_left = {
      {{0x76abb18a, 0x92c0f7b9, 0x2c1a37e0, 0x7fdf6ca1, 0xe3401760, 0x66eb7d52,
        0x918d50a7, 0x12a65bd6}},
      {{0x1f3a8b42, 0xc8d3127f, 0x6d96f7cd, 0xea6fe2f6, 0x0ac0b46b, 0x557a5f30,
        0xaf075250, 0x786528cb}},
      {{0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000}}};

  EccPointJacobiFq const efqj_expect = {
      {{0x7305f782, 0x84101559, 0xe065da04, 0xfd373fcf, 0x31d2f725, 0x9b420bdc,
        0x622ac9c3, 0xba0ef378}},
      {{0x97b2adf0, 0x2935c14b, 0xfc6415b0, 0x48ba036a, 0x61ca7383, 0xcff7f03d,
        0x23a2c3e6, 0x0df0d4a5}},
      {{0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000}}};

  EFqAdd(&efqj_left, &efqj_left, &efqj_left);
  EXPECT_EQ(efqj_expect, efqj_left);
}
////////////////////////////////////////////////////////////////////////
// EFqRand

// Checks if EFqRand can generate points with Y >= q+1
TEST(TinyEFqTest, EFqRandCanGenerateBigY) {
  OneTimePad otp({
      0x25, 0xeb, 0x8c, 0x48, 0xff, 0x89, 0xcb, 0x85, 0x4f, 0xc0, 0x90, 0x81,
      0xcc, 0x47, 0xed, 0xfc, 0x86, 0x19, 0xb2, 0x14, 0xfe, 0x65, 0x92, 0xd4,
      0x8b, 0xfc, 0xea, 0x9c, 0x9d, 0x8e, 0x32, 0x44, 0xd7, 0xd7, 0xe9, 0xf1,
      0xf7, 0xde, 0x60, 0x56, 0x8d, 0xe9, 0x89, 0x07, 0x3f, 0x3d, 0x16, 0x39,
  });

  // expected.y >= q+1
  EccPointFq expected = {{0xd78542d3, 0xf824c127, 0x81eea621, 0x6990833d,
                          0x9d843df6, 0x3df36126, 0x435e8eda, 0x2d267586},
                         {0x1B8DC14B, 0x5967C520, 0x61CECE5D, 0x0290B5BC,
                          0x49278859, 0x2C523D9A, 0x6D0AE6DE, 0xAE5590C9}};

  EccPointFq actual = {0};
  EXPECT_TRUE(EFqRand(&actual, OneTimePad::Generate, &otp));
  EXPECT_EQ(expected, actual);
  EXPECT_EQ(384u, otp.BitsConsumed());
}

// Checks if EFqRand can generate points with Y <= q-1
TEST(TinyEFqTest, EFqRandCanGenerateSmallY) {
  OneTimePad otp({
      0x47, 0xe7, 0xb0, 0x36, 0x0f, 0x85, 0x91, 0xaa, 0x14, 0x76, 0xb0, 0x16,
      0xe5, 0x8d, 0xf1, 0x72, 0x61, 0xb5, 0x54, 0x0a, 0x60, 0xb7, 0x3d, 0x38,
      0xd9, 0x95, 0xe7, 0x60, 0xf9, 0xd3, 0x19, 0xf1, 0x8e, 0x8d, 0xd4, 0x74,
      0x2b, 0x86, 0xcd, 0xb8, 0xbb, 0x8f, 0x18, 0xfb, 0x89, 0xc2, 0xc7, 0x35,
  });

  // expected.y <= q-1
  EccPointFq expected = {{0x5f65199e, 0x59366f56, 0xb1d35d89, 0xf42fdc1f,
                          0x07fd66d6, 0x95f32cb3, 0xc4ef7101, 0xeb426988},
                         {0x834171f5, 0xca1c1f05, 0xdcb4d142, 0xbe060756,
                          0x9185652b, 0xb3b9ede1, 0x671f682e, 0x22d0c94c}};

  EccPointFq actual = {0};
  EXPECT_TRUE(EFqRand(&actual, OneTimePad::Generate, &otp));
  EXPECT_EQ(expected, actual);
  EXPECT_EQ(384u, otp.BitsConsumed());
}

TEST(TinyEFqTest, EFqRandWorksForFailedSquareRoot) {
  // Only last 48 bytes of the given otp will result in x such that
  // Fq.sqrt of (x^3 + a*x + b) exists.
  OneTimePad otp({
      0x01, 0x80, 0x3c, 0xd1, 0x08, 0xd8, 0x8d, 0x73, 0xaf, 0xea, 0x79, 0xc8,
      0x1e, 0x47, 0x83, 0xc6, 0x95, 0x31, 0x39, 0x03, 0xc4, 0x18, 0xf1, 0x2b,
      0x4c, 0x1a, 0x34, 0x50, 0x6d, 0x73, 0x29, 0xd2, 0x0f, 0x40, 0xc4, 0x19,
      0x6f, 0xe2, 0xd7, 0x87, 0x1a, 0x99, 0x68, 0x16, 0x09, 0xc3, 0xe7, 0x7e,
      0x17, 0x7d, 0x64, 0x9b, 0xa5, 0x39, 0x53, 0xa6, 0x88, 0x20, 0xa2, 0x0a,
      0x17, 0x8f, 0xef, 0x57, 0x19, 0xc7, 0xf3, 0x5c, 0x4a, 0xbe, 0x2e, 0xa0,
      0xd8, 0x97, 0xb7, 0x41, 0x71, 0x4d, 0x03, 0x80, 0xf8, 0xfd, 0xcd, 0x06,
      0x34, 0xd5, 0xc6, 0x02, 0x4c, 0xdb, 0x95, 0xcb, 0x07, 0x4d, 0xc8, 0x4b,
      0x4c, 0x2b, 0x14, 0x1e, 0x24, 0x67, 0x07, 0x2d, 0xc4, 0x39, 0xf0, 0xfc,
      0xd2, 0x60, 0x0d, 0x0a, 0x17, 0x7c, 0x51, 0x87, 0x79, 0x98, 0xca, 0xdc,
      0x94, 0xa0, 0x8c, 0xc1, 0x5e, 0x3c, 0xe9, 0x98, 0x52, 0x73, 0x61, 0x82,
      0xec, 0xdc, 0x67, 0x62, 0x0a, 0xb6, 0x60, 0xe9, 0x52, 0xd6, 0xc6, 0xc2,
      0x47, 0xe7, 0xb0, 0x36, 0x0f, 0x85, 0x91, 0xaa, 0x14, 0x76, 0xb0, 0x16,
      0xe5, 0x8d, 0xf1, 0x72, 0x61, 0xb5, 0x54, 0x0a, 0x60, 0xb7, 0x3d, 0x38,
      0xd9, 0x95, 0xe7, 0x60, 0xf9, 0xd3, 0x19, 0xf1, 0x8e, 0x8d, 0xd4, 0x74,
      0x2b, 0x86, 0xcd, 0xb8, 0xbb, 0x8f, 0x18, 0xfb, 0x89, 0xc2, 0xc7, 0x35,
  });

  EccPointFq expected = {{0x5f65199e, 0x59366f56, 0xb1d35d89, 0xf42fdc1f,
                          0x07fd66d6, 0x95f32cb3, 0xc4ef7101, 0xeb426988},
                         {0x834171f5, 0xca1c1f05, 0xdcb4d142, 0xbe060756,
                          0x9185652b, 0xb3b9ede1, 0x671f682e, 0x22d0c94c}};

  EccPointFq actual = {0};

  EXPECT_TRUE(EFqRand(&actual, OneTimePad::Generate, &otp));
  EXPECT_EQ(expected, actual);
  EXPECT_EQ(1536u, otp.BitsConsumed());
}

////////////////////////////////////////////////////////////////////////
// EFqSet

TEST(TinyEFqTest, EFqSetWorks) {
  EccPointJacobiFq efqj_left = {0};
  EccPointJacobiFq const efqj_expect = {{{1}}, {{2}}, {{1}}};

  FqElem x = {1};
  FqElem y = {2};

  EFqSet(&efqj_left, &x, &y);
  EXPECT_EQ(efqj_expect, efqj_left);
}

////////////////////////////////////////////////////////////////////////
// EFqIsInf
TEST(TinyEFqTest, EFqIsInfPasses) {
  EccPointJacobiFq const efqj_inf = {{{0}}, {{1}}, {{0}}};

  EXPECT_TRUE(EFqIsInf(&efqj_inf));
}

TEST(TinyEFqTest, EFqIsInfFails) {
  EccPointJacobiFq const efqj_noninf = {{{1}}, {{2}}, {{1}}};

  EXPECT_FALSE(EFqIsInf(&efqj_noninf));
}

////////////////////////////////////////////////////////////////////////
// EFqFromAffine
TEST(TinyEFqTest, FqFromAffineWorks) {
  EccPointJacobiFq efqj_left = {0};
  EccPointJacobiFq const efqj_expect = {{{1}}, {{2}}, {{1}}};
  EccPointFq efq_left = {{{1}}, {{2}}};

  EFqFromAffine(&efqj_left, &efq_left);
  EXPECT_EQ(efqj_expect, efqj_left);
}
////////////////////////////////////////////////////////////////////////
// EFqToAffine

TEST(TinyEFqTest, EFqToAffineWorks) {
  EccPointFq efq_left = {0};
  EccPointFq const efq_expect = {{{1}}, {{2}}};
  EccPointJacobiFq efqj_left = {{{1}}, {{2}}, {{1}}};

  EFqToAffine(&efq_left, &efqj_left);
  EXPECT_EQ(efq_expect, efq_left);
}

////////////////////////////////////////////////////////////////////////
// EFqNeg

TEST(TinyEFqTest, EFqNegWorks) {
  EccPointJacobiFq efqj_left = {0};
  EccPointJacobiFq efqj_right = {
      {{0x76abb18a, 0x92c0f7b9, 0x2c1a37e0, 0x7fdf6ca1, 0xe3401760, 0x66eb7d52,
        0x918d50a7, 0x12a65bd6}},
      {{0x1f3a8b42, 0xc8d3127f, 0x6d96f7cd, 0xea6fe2f6, 0x0ac0b46b, 0x557a5f30,
        0xaf075250, 0x786528cb}},
      {{0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000}}};

  EccPointJacobiFq const efqj_expect = {
      {{0x76abb18a, 0x92c0f7b9, 0x2c1a37e0, 0x7fdf6ca1, 0xe3401760, 0x66eb7d52,
        0x918d50a7, 0x12a65bd6}},
      {{0x8f98a4d1, 0x0a561b5c, 0xa50112b5, 0x226c8304, 0xe3b0f033, 0xf16b932e,
        0x50f59e7c, 0x879ad734}},
      {{0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000}}};
  EFqNeg(&efqj_left, &efqj_right);

  EXPECT_EQ(efqj_expect, efqj_left);
}

////////////////////////////////////////////////////////////////////////
// EFqEq

TEST(TinyEFqTest, EFqEqPasses) {
  EccPointJacobiFq const efqj_left = {{{1}}, {{2}}, {{1}}};

  EccPointJacobiFq const efqj_right = {{{1}}, {{2}}, {{1}}};

  EXPECT_TRUE(EFqEq(&efqj_left, &efqj_right));
}

TEST(TinyEFqTest, EFqEqFails) {
  EccPointJacobiFq const efqj_left = {{{1}}, {{2}}, {{1}}};

  EccPointJacobiFq const efqj_right = {
      {{0x76abb18a, 0x92c0f7b9, 0x2c1a37e0, 0x7fdf6ca1, 0xe3401760, 0x66eb7d52,
        0x918d50a7, 0x12a65bd6}},
      {{0x1f3a8b42, 0xc8d3127f, 0x6d96f7cd, 0xea6fe2f6, 0x0ac0b46b, 0x557a5f30,
        0xaf075250, 0x786528cb}},
      {{0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000}}};

  EXPECT_FALSE(EFqEq(&efqj_left, &efqj_right));
}

////////////////////////////////////////////////////////////////////////
// EFqHash

TEST(TinyEFqTest, EFqHashWithSha512Works) {
  EccPointFq efq_result = {0};
  EccPointFq efq_expect = {{{0x992a96e4, 0x42f7b394, 0xf3bada9c, 0xb4033d83,
                             0x21979b9b, 0xbc82a6a2, 0x55555586, 0x8c62a02d}},
                           {{0x406c8f03, 0xe691d8f3, 0xadf2f27b, 0x69540cc6,
                             0xe02b87f7, 0x217d5424, 0x17b9fbe5, 0x4c0ea762}}};
  unsigned char const msg_buf[] = {'a', 'b', 'c'};
  size_t len = sizeof(msg_buf);
  HashAlg hashalg = kSha512;
  EXPECT_TRUE(EFqHash(&efq_result, msg_buf, len, hashalg));
  EXPECT_EQ(efq_expect, efq_result);
}

TEST(TinyEFqTest, EFqHashWithSha256Works) {
  EccPointFq efq_result = {0};
  EccPointFq efq_expect = {{{0x8008953d, 0xd54a103c, 0x70e6186b, 0x54f5a62a,
                             0xadbe836e, 0xf3716581, 0x88ff2562, 0x2ebb504d}},
                           {{0xdc48f9f7, 0x48aa0d6c, 0x623e2c1f, 0x7ecdf02e,
                             0x07f07a32, 0xbd6738b1, 0xb13f3cb4, 0x8a43a104}}};
  unsigned char const msg_buf[] = {'a', 'b', 'c'};
  size_t len = sizeof(msg_buf);
  HashAlg hashalg = kSha256;
  EXPECT_TRUE(EFqHash(&efq_result, msg_buf, len, hashalg));
  EXPECT_EQ(efq_expect, efq_result);
}

TEST(TinyEFqTest, HashWorksForResultYSmallerThanHalfOfQAndEven) {
  std::vector<uint8_t> msg = {'a', 'a', 'd'};
  EccPointFq result;
  EccPointFq expected = {{0xb315d67e, 0x1924ae56, 0xcf527861, 0xebb789b6,
                          0x3f429d2a, 0xb193bf9a, 0x6bd8502f, 0x5e73be39},
                         {0x0bd51968, 0x0f13472d, 0xc96b5096, 0xa9cd4491,
                          0x4ab668cf, 0x2123d56c, 0xf30af180, 0x0db43c33}};
  EXPECT_TRUE(EFqHash(&result, msg.data(), msg.size(), kSha512));
  EXPECT_EQ(expected, result);
}
TEST(TinyEFqTest, HashWorksForResultYSmallerThanHalfOfQAndOdd) {
  std::vector<uint8_t> msg = {'a', 'a', 'c'};
  EccPointFq result;
  EccPointFq expected = {{0x81D359E2, 0xF438B2AC, 0xAA4342EB, 0x80042B18,
                          0x850E1C62, 0x90860717, 0xC79A1AB8, 0xF8F4F2F6},
                         {0xC1A2BA30, 0x369C0D70, 0x03EAF9DD, 0x4F93FC67,
                          0xBB6E5D10, 0x441B22F9, 0xEC70C946, 0xE8D39BD4}};
  EXPECT_TRUE(EFqHash(&result, msg.data(), msg.size(), kSha512));
  EXPECT_EQ(expected, result);
}

TEST(TinyEFqTest, HashWorksForResultYLargerThanHalfOfQAndOdd) {
  std::vector<uint8_t> msg = {'a', 'a', 'b'};
  EccPointFq result;
  EccPointFq expected = {{0x31F874DA, 0x62DB014E, 0x4A4FC69A, 0xC1DCC122,
                          0xC423DAF8, 0x27AB3AAC, 0xF1DE0993, 0x07906282},
                         {0x8DA507A4, 0x568E1D1E, 0x6D373E90, 0x99A18DA4,
                          0xC5717AA2, 0x98C222F5, 0x0A2ADDF2, 0xA1212A44}};
  EFqHash(&result, msg.data(), msg.size(), kSha512);
  EXPECT_EQ(expected, result);
}

TEST(TinyEFqTest, HashWorksForResultYLargerThanHalfOfQAndEven) {
  std::vector<uint8_t> msg = {'a', 'a', 'e'};
  EccPointFq result;
  EccPointFq expected = {{0xA798F97C, 0xF24EC264, 0xD4C051F4, 0xBA4A5B45,
                          0xD2CF5996, 0x121A3F66, 0x222279F0, 0x208E4FD4},
                         {0x3C816617, 0x6CF621EB, 0x8B8DAFC4, 0x63EB39C7,
                          0xE6C8AF5C, 0x32C732D0, 0xC5C46152, 0x114E8AE0}};
  EFqHash(&result, msg.data(), msg.size(), kSha512);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// EFqCp

TEST(TinyEFqTest, EFqCpWorks) {
  const EccPointFq ecfq_point = {
      {{0x76ABB18A, 0x92C0F7B9, 0x2C1A37E0, 0x7FDF6CA1, 0xE3401760, 0x66EB7D52,
        0x918D50A7, 0x12A65BD6}},
      {{0x1F3A8B42, 0xC8D3127F, 0x6D96F7CD, 0xEA6FE2F6, 0x0AC0B46B, 0x557A5F30,
        0xAF075250, 0x786528CB}}};
  EccPointFq result_ecfq_point = {0};
  EFqCp(&result_ecfq_point, &ecfq_point);
  EXPECT_EQ(ecfq_point, result_ecfq_point);
}

////////////////////////////////////////////////////////////////////////
// EFqEqAffine

TEST(TinyEFqTest, EFqEqAffinePasses) {
  EccPointFq efq_left = {{{1}}, {{2}}};
  EccPointFq efq_right = {{{1}}, {{2}}};

  EXPECT_TRUE(EFqEqAffine(&efq_left, &efq_right));
}

TEST(TinyEFqTest, EFqEqAffineFails) {
  EccPointFq efq_left = {{{1}}, {{2}}};
  EccPointFq efq_right = {{{0x76abb18a, 0x92c0f7b9, 0x2c1a37e0, 0x7fdf6ca1,
                            0xe3401760, 0x66eb7d52, 0x918d50a7, 0x12a65bd6}},
                          {{0x8f98a4d1, 0x0a561b5c, 0xa50112b5, 0x226c8304,
                            0xe3b0f033, 0xf16b932e, 0x50f59e7c, 0x879ad734}}};

  EXPECT_FALSE(EFqEqAffine(&efq_left, &efq_right));
}
////////////////////////////////////////////////////////////////////////
// EFqCondSet

TEST(TinyEFqTest, EFqCondSetWorksForTrue) {
#define TRUE 1
  EccPointJacobiFq efqj_left = {0};
  EccPointJacobiFq efqj_right = {
      {{0x76abb18a, 0x92c0f7b9, 0x2c1a37e0, 0x7fdf6ca1, 0xe3401760, 0x66eb7d52,
        0x918d50a7, 0x12a65bd6}},
      {{0x1f3a8b42, 0xc8d3127f, 0x6d96f7cd, 0xea6fe2f6, 0x0ac0b46b, 0x557a5f30,
        0xaf075250, 0x786528cb}},
      {{0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000}}};
  EccPointJacobiFq efqj_expect = {{{1}}, {{2}}, {{1}}};

  EFqCondSet(&efqj_left, &efqj_expect, &efqj_right, TRUE);
  EXPECT_EQ(efqj_left, efqj_expect);
}

TEST(TinyEFqTest, EFqCondSetWorksForFalse) {
#define FALSE 0
  EccPointJacobiFq efqj_left = {0};
  EccPointJacobiFq efqj_right = {{{1}}, {{2}}, {{1}}};
  EccPointJacobiFq efqj_expect = {
      {{0x76abb18a, 0x92c0f7b9, 0x2c1a37e0, 0x7fdf6ca1, 0xe3401760, 0x66eb7d52,
        0x918d50a7, 0x12a65bd6}},
      {{0x1f3a8b42, 0xc8d3127f, 0x6d96f7cd, 0xea6fe2f6, 0x0ac0b46b, 0x557a5f30,
        0xaf075250, 0x786528cb}},
      {{0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000}}};

  EFqCondSet(&efqj_left, &efqj_right, &efqj_expect, FALSE);
  EXPECT_EQ(efqj_left, efqj_expect);
}

////////////////////////////////////////////////////////////////////////
// EFqJCp

TEST(TinyEFqTest, EFqJCpWorks) {
  const EccPointJacobiFq efqj = {{{1}}, {{2}}, {{1}}};
  EccPointJacobiFq efqj_result = {0};
  EFqJCp(&efqj_result, &efqj);
  EXPECT_EQ(efqj, efqj_result);
}

////////////////////////////////////////////////////////////////////////
// EFqInf

TEST(TinyEFqTest, EFqInfWorks) {
  EccPointJacobiFq efqj_left;
  EccPointJacobiFq efqj_expect = {{{0}}, {{1}}, {{0}}};
  EFqInf(&efqj_left);

  EXPECT_EQ(efqj_expect, efqj_left);
}

////////////////////////////////////////////////////////////////////////
// EFqOnCurve

TEST(TinyEFqTest, EFqOnCurvePasses) {
  const EccPointFq ecfq_point = {
      {{0x76ABB18A, 0x92C0F7B9, 0x2C1A37E0, 0x7FDF6CA1, 0xE3401760, 0x66EB7D52,
        0x918D50A7, 0x12A65BD6}},
      {{0x1F3A8B42, 0xC8D3127F, 0x6D96F7CD, 0xEA6FE2F6, 0x0AC0B46B, 0x557A5F30,
        0xAF075250, 0x786528CB}}};
  EXPECT_EQ(1, EFqOnCurve(&ecfq_point));
}

TEST(TinyEFqTest, EFqOnCurveFails) {
  EccPointFq bad_ecfq_point = {
      {{0x76ABB18A, 0x92C0F7B9, 0x2C1A37E0, 0x7FDF6CA1, 0xE3401760, 0x66EB7D52,
        0x918D50A7, 0x12A65BD6}},
      {{0x1F3A8B42, 0xC8D3127F, 0x6D96F7CD, 0xEA6FE2F6, 0x0AC0B46B, 0x557A5F30,
        0xAF075250, 0x786528CB}}};
  uint8_t* ecpq_vec = (uint8_t*)bad_ecfq_point.x.limbs.word;
  ecpq_vec[31]++;
  EXPECT_EQ(0, EFqOnCurve(&bad_ecfq_point));
}

////////////////////////////////////////////////////////////////////////
// EFqJOnCurve

TEST(TinyEFqTest, EFqJOnCurvePasses) {
  EccPointJacobiFq efqj = {{{1}}, {{2}}, {{1}}};

  EXPECT_TRUE(EFqJOnCurve(&efqj));
}

TEST(TinyEFqTest, EFqJOnCurveFails) {
  EccPointJacobiFq efqj = {{{1}}, {{4}}, {{1}}};

  EXPECT_FALSE(EFqJOnCurve(&efqj));
}

TEST(TinyEFqTest, EFqJOnCurveAcceptsPointAtInfinity) {
  EccPointJacobiFq infinity = {{{0}}, {{1}}, {{0}}};

  EXPECT_TRUE(EFqJOnCurve(&infinity));
}

////////////////////////////////////////////////////////////////////////
// EFqJRand

// Checks if EFqJRand can generate points with Y >= q+1
TEST(TinyEFqTest, EFqJRandCanGenerateBigY) {
  OneTimePad otp({
      0x25, 0xeb, 0x8c, 0x48, 0xff, 0x89, 0xcb, 0x85, 0x4f, 0xc0, 0x90, 0x81,
      0xcc, 0x47, 0xed, 0xfc, 0x86, 0x19, 0xb2, 0x14, 0xfe, 0x65, 0x92, 0xd4,
      0x8b, 0xfc, 0xea, 0x9c, 0x9d, 0x8e, 0x32, 0x44, 0xd7, 0xd7, 0xe9, 0xf1,
      0xf7, 0xde, 0x60, 0x56, 0x8d, 0xe9, 0x89, 0x07, 0x3f, 0x3d, 0x16, 0x39,
  });

  // expected.y >= q+1
  EccPointJacobiFq expected = {
      {0xd78542d3, 0xf824c127, 0x81eea621, 0x6990833d, 0x9d843df6, 0x3df36126,
       0x435e8eda, 0x2d267586},
      {0x1B8DC14B, 0x5967C520, 0x61CECE5D, 0x0290B5BC, 0x49278859, 0x2C523D9A,
       0x6D0AE6DE, 0xAE5590C9},
      {0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
       0x00000000, 0x00000000}};

  EccPointJacobiFq actual = {0};
  EXPECT_TRUE(EFqJRand(&actual, OneTimePad::Generate, &otp));
  EXPECT_EQ(expected, actual);
  EXPECT_EQ(384u, otp.BitsConsumed());
}

// Checks if EFqJRand can generate points with Y <= q-1
TEST(TinyEFqTest, EFqJRandCanGenerateSmallY) {
  OneTimePad otp({
      0x47, 0xe7, 0xb0, 0x36, 0x0f, 0x85, 0x91, 0xaa, 0x14, 0x76, 0xb0, 0x16,
      0xe5, 0x8d, 0xf1, 0x72, 0x61, 0xb5, 0x54, 0x0a, 0x60, 0xb7, 0x3d, 0x38,
      0xd9, 0x95, 0xe7, 0x60, 0xf9, 0xd3, 0x19, 0xf1, 0x8e, 0x8d, 0xd4, 0x74,
      0x2b, 0x86, 0xcd, 0xb8, 0xbb, 0x8f, 0x18, 0xfb, 0x89, 0xc2, 0xc7, 0x35,
  });

  // expected.y <= q-1
  EccPointJacobiFq expected = {
      {0x5f65199e, 0x59366f56, 0xb1d35d89, 0xf42fdc1f, 0x07fd66d6, 0x95f32cb3,
       0xc4ef7101, 0xeb426988},
      {0x834171f5, 0xca1c1f05, 0xdcb4d142, 0xbe060756, 0x9185652b, 0xb3b9ede1,
       0x671f682e, 0x22d0c94c},
      {0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
       0x00000000, 0x00000000}};

  EccPointJacobiFq actual = {0};
  EXPECT_TRUE(EFqJRand(&actual, OneTimePad::Generate, &otp));
  EXPECT_EQ(expected, actual);
  EXPECT_EQ(384u, otp.BitsConsumed());
}

TEST(TinyEFqTest, EFqJRandWorksForFailedSquareRoot) {
  // Only last 48 bytes of the given otp will result in x such that
  // Fq.sqrt of (x^3 + a*x + b) exists.
  OneTimePad otp({
      0x01, 0x80, 0x3c, 0xd1, 0x08, 0xd8, 0x8d, 0x73, 0xaf, 0xea, 0x79, 0xc8,
      0x1e, 0x47, 0x83, 0xc6, 0x95, 0x31, 0x39, 0x03, 0xc4, 0x18, 0xf1, 0x2b,
      0x4c, 0x1a, 0x34, 0x50, 0x6d, 0x73, 0x29, 0xd2, 0x0f, 0x40, 0xc4, 0x19,
      0x6f, 0xe2, 0xd7, 0x87, 0x1a, 0x99, 0x68, 0x16, 0x09, 0xc3, 0xe7, 0x7e,
      0x17, 0x7d, 0x64, 0x9b, 0xa5, 0x39, 0x53, 0xa6, 0x88, 0x20, 0xa2, 0x0a,
      0x17, 0x8f, 0xef, 0x57, 0x19, 0xc7, 0xf3, 0x5c, 0x4a, 0xbe, 0x2e, 0xa0,
      0xd8, 0x97, 0xb7, 0x41, 0x71, 0x4d, 0x03, 0x80, 0xf8, 0xfd, 0xcd, 0x06,
      0x34, 0xd5, 0xc6, 0x02, 0x4c, 0xdb, 0x95, 0xcb, 0x07, 0x4d, 0xc8, 0x4b,
      0x4c, 0x2b, 0x14, 0x1e, 0x24, 0x67, 0x07, 0x2d, 0xc4, 0x39, 0xf0, 0xfc,
      0xd2, 0x60, 0x0d, 0x0a, 0x17, 0x7c, 0x51, 0x87, 0x79, 0x98, 0xca, 0xdc,
      0x94, 0xa0, 0x8c, 0xc1, 0x5e, 0x3c, 0xe9, 0x98, 0x52, 0x73, 0x61, 0x82,
      0xec, 0xdc, 0x67, 0x62, 0x0a, 0xb6, 0x60, 0xe9, 0x52, 0xd6, 0xc6, 0xc2,
      0x47, 0xe7, 0xb0, 0x36, 0x0f, 0x85, 0x91, 0xaa, 0x14, 0x76, 0xb0, 0x16,
      0xe5, 0x8d, 0xf1, 0x72, 0x61, 0xb5, 0x54, 0x0a, 0x60, 0xb7, 0x3d, 0x38,
      0xd9, 0x95, 0xe7, 0x60, 0xf9, 0xd3, 0x19, 0xf1, 0x8e, 0x8d, 0xd4, 0x74,
      0x2b, 0x86, 0xcd, 0xb8, 0xbb, 0x8f, 0x18, 0xfb, 0x89, 0xc2, 0xc7, 0x35,
  });

  EccPointJacobiFq expected = {
      {0x5f65199e, 0x59366f56, 0xb1d35d89, 0xf42fdc1f, 0x07fd66d6, 0x95f32cb3,
       0xc4ef7101, 0xeb426988},
      {0x834171f5, 0xca1c1f05, 0xdcb4d142, 0xbe060756, 0x9185652b, 0xb3b9ede1,
       0x671f682e, 0x22d0c94c},
      {0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
       0x00000000, 0x00000000}};

  EccPointJacobiFq actual = {0};

  EXPECT_TRUE(EFqJRand(&actual, OneTimePad::Generate, &otp));
  EXPECT_EQ(expected, actual);
  EXPECT_EQ(1536u, otp.BitsConsumed());
}

}  // namespace
