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
/// Unit tests of Fp implementation.
/*! \file */

#include <gtest/gtest.h>

#include "epid/member/tiny/math/unittests/cmp-testhelper.h"
#include "epid/member/tiny/math/unittests/onetimepad.h"

extern "C" {
#include "epid/member/tiny/math/fp.h"
#include "epid/member/tiny/math/mathtypes.h"
}

namespace {

////////////////////////////////////////////////////////////////////////
// FpInField

TEST(TinyFpTest, FpInFieldPasses) {
  FpElem zero = {0};
  FpElem p_minus_one = {{0xD10B500C, 0xF62D536C, 0x1299921A, 0x0CDC65FB,
                         0xEE71A49E, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  EXPECT_TRUE(FpInField(&zero));
  EXPECT_TRUE(FpInField(&p_minus_one));
}

TEST(TinyFpTest, FpInFieldFails) {
  FpElem p = {{0xD10B500D, 0xF62D536C, 0x1299921A, 0x0CDC65FB, 0xEE71A49E,
               0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  EXPECT_FALSE(FpInField(&p));
}

////////////////////////////////////////////////////////////////////////
// FpAdd

TEST(TinyFpTest, FpAddWorks) {
  FpElem result = {0}, expected = {0}, left = {0}, right = {0};
  left.limbs.word[5] = 1;
  right.limbs.word[5] = 2;
  expected.limbs.word[5] = 3;
  FpAdd(&result, &left, &right);
  EXPECT_EQ(expected, result);

  FpElem p_minus_one = {0xD10B500C, 0xF62D536C, 0x1299921A, 0x0CDC65FB,
                        0xEE71A49E, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF};
  right = {2, 0, 0, 0, 0, 0, 0, 0};
  expected = {1, 0, 0, 0, 0, 0, 0, 0};
  FpAdd(&result, &p_minus_one, &right);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// FpMul

TEST(TinyFpTest, FpMultWorks) {
  FpElem left = {0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c,
                 0xafa65357, 0x4780716c, 0xffd94b0f, 0x5e643124};
  FpElem right = {0x848cdb73, 0x6399829e, 0xcaa20cc0, 0x1b02bff6,
                  0x2b477bd2, 0xf9d48534, 0xff7929a0, 0xd4745161};
  FpElem expected = {0x3f172ebf, 0xf2219fce, 0x73591802, 0x7a7dbc7f,
                     0xf82ed0df, 0xb8c0c56d, 0x3395ff68, 0x83d64983};
  FpElem result = {0};
  FpMul(&result, &left, &right);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// FpSub

TEST(TinyFpTest, FpSubWorks) {
  FpElem result = {0}, expected = {0}, left = {0}, right = {0};
  left.limbs.word[4] = 2;
  right.limbs.word[4] = 1;
  expected.limbs.word[4] = 1;
  FpSub(&result, &left, &right);
  EXPECT_EQ(expected, result);

  left = {1, 0, 0, 0, 0, 0, 0, 0};
  right = {2, 0, 0, 0, 0, 0, 0, 0};
  FpElem p_minus_one = {0xD10B500C, 0xF62D536C, 0x1299921A, 0x0CDC65FB,
                        0xEE71A49E, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF};
  FpSub(&result, &left, &right);
  EXPECT_EQ(p_minus_one, result);
}

////////////////////////////////////////////////////////////////////////
// FpExp
TEST(TinyFpTest, FpExpWorks) {
  FpElem result = {0}, expected = {0}, in = {0};
  VeryLargeInt exp = {0xD10B500C, 0xF62D536C, 0x1299921A, 0x0CDC65FB,
                      0xEE71A49E, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF};
  in.limbs.word[0] = 1;
  expected.limbs.word[0] = 1;
  FpExp(&result, &in, &exp);
  EXPECT_EQ(expected, result);

  exp = {4, 0, 0, 0, 0, 0, 0, 0};
  in = {{0x0000007B, 0, 0, 0, 0, 0, 0, 0}};
  expected = {{0x0DA48871, 0, 0, 0, 0, 0, 0, 0}};
  FpExp(&result, &in, &exp);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// FpNeg
TEST(TinyFpTest, FpNegWorks) {
  FpElem const pairing_p = {{0xD10B500D, 0xF62D536C, 0x1299921A, 0x0CDC65FB,
                             0xEE71A49E, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  FpElem neg_value = {0};
  FpElem one = {{1, 0, 0, 0, 0, 0, 0, 0}};
  FpElem minus_one = pairing_p;
  --minus_one.limbs.word[0];
  FpNeg(&neg_value, &one);
  EXPECT_EQ(minus_one, neg_value);

  FpElem value = {{0xD10B500C, 0xF62D536C, 0x1299921A, 0x0CDC65FB, 0xEE71A49E,
                   0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  FpNeg(&neg_value, &value);
  FpNeg(&neg_value, &neg_value);
  EXPECT_EQ(value, neg_value);
}

////////////////////////////////////////////////////////////////////////
// FpEq

TEST(TinyFpTest, FpEqPasses) {
  FpElem a = {{0xD10B500C, 0xF62D536C, 0x1299921A, 0x0CDC65FB, 0xEE71A49E,
               0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  FpElem c = {{0xD10B500C, 0xF62D536C, 0x1299921A, 0x0CDC65FB, 0xEE71A49E,
               0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  EXPECT_TRUE(FpEq(&a, &c));
}

TEST(TinyFpTest, FpEqFails) {
  FpElem a = {{0xD10B500C, 0xF62D536C, 0x1299921A, 0x0CDC65FB, 0xEE71A49E,
               0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  FpElem b = {{0, 0, 0, 0, 0, 0, 1, 0}};
  EXPECT_FALSE(FpEq(&a, &b));
}

////////////////////////////////////////////////////////////////////////
// FpInv

TEST(TinyFpTest, FpInvWorks) {
  FpElem a = {{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357,
               0x4780716c, 0xffd94b0f, 0x5e643124}};
  FpElem expected = {{0xd97deaf7, 0xa6011d83, 0x3c381713, 0x92472e34,
                      0x997861e8, 0xc1dfdc87, 0x157eb11b, 0xc9cd1238}};
  FpElem result;
  FpInv(&result, &a);
  EXPECT_EQ(result, expected);
}

////////////////////////////////////////////////////////////////////////
// FpRand

TEST(TinyFpTest, FpRandConsumes384BitsOfEntropy) {
  OneTimePad otp(64);
  FpElem actual = {0};
  EXPECT_TRUE(FpRand(&actual, OneTimePad::Generate, &otp));
  EXPECT_EQ(384u, otp.BitsConsumed());
}

TEST(TinyFpTest, FpRandWorks) {
  OneTimePad otp({// slen bits
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  // p + 1
                  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xF0, 0xCD, 0x46, 0xE5,
                  0xF2, 0x5E, 0xEE, 0x71, 0xA4, 0x9E, 0x0C, 0xDC, 0x65, 0xFB,
                  0x12, 0x99, 0x92, 0x1A, 0xF6, 0x2D, 0x53, 0x6C, 0xD1, 0x0B,
                  0x50, 0x0E});
  FpElem expected = {{1, 0, 0, 0, 0, 0, 0, 0}};
  FpElem actual = {0};
  EXPECT_TRUE(FpRand(&actual, OneTimePad::Generate, &otp));
  EXPECT_EQ(expected, actual);
}

////////////////////////////////////////////////////////////////////////
// FpRandNonzero

TEST(TinyFpTest, FpRandNonzeroConsumes384BitsOfEntropy) {
  OneTimePad otp(64);
  FpElem actual = {0};
  EXPECT_TRUE(FpRandNonzero(&actual, OneTimePad::Generate, &otp));
  EXPECT_EQ(384u, otp.BitsConsumed());
}

TEST(TinyFpTest, FpRandNonzeroWorks) {
  OneTimePad otp({// slen bits
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  // p - 1
                  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xF0, 0xCD, 0x46, 0xE5,
                  0xF2, 0x5E, 0xEE, 0x71, 0xA4, 0x9E, 0x0C, 0xDC, 0x65, 0xFB,
                  0x12, 0x99, 0x92, 0x1A, 0xF6, 0x2D, 0x53, 0x6C, 0xD1, 0x0B,
                  0x50, 0x0C});
  FpElem expected = {{1, 0, 0, 0, 0, 0, 0, 0}};
  FpElem actual = {0};
  EXPECT_TRUE(FpRandNonzero(&actual, OneTimePad::Generate, &otp));
  EXPECT_EQ(expected, actual);
}

////////////////////////////////////////////////////////////////////////
// FpClear

TEST(TinyFpTest, FpClearWorks) {
  FpElem zero = {0};
  FpElem a = {{0xD10B500C, 0xF62D536C, 0x1299921A, 0x0CDC65FB, 0xEE71A49E,
               0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  FpClear(&a);
  EXPECT_EQ(zero, a);
}

////////////////////////////////////////////////////////////////////////
// FpSet

TEST(TinyFpTest, FpSetWorks) {
  uint32_t small = 0xffffffff;
  FpElem expected = {{small, 0, 0, 0, 0, 0, 0, 0}};
  FpElem result = {{0xD10B500C, 0xF62D536C, 0x1299921A, 0x0CDC65FB, 0xEE71A49E,
                    0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  FpSet(&result, small);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// FpFromHash

TEST(TinyFpTest, FpFromHashWorks) {
  FpElem p_mod_p;
  FpElem zero = {0};
  uint8_t p_str[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xF0, 0xCD,
                       0x46, 0xE5, 0xF2, 0x5E, 0xEE, 0x71, 0xA4, 0x9E,
                       0x0C, 0xDC, 0x65, 0xFB, 0x12, 0x99, 0x92, 0x1A,
                       0xF6, 0x2D, 0x53, 0x6C, 0xD1, 0x0B, 0x50, 0x0D};
  FpFromHash(&p_mod_p, p_str, sizeof(p_str));
  EXPECT_EQ(zero, p_mod_p);

  FpElem one = {{1, 0, 0, 0, 0, 0, 0, 0}};
  uint8_t one_str[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
  FpElem one_mod_p;
  FpFromHash(&one_mod_p, one_str, sizeof(one_str));
  EXPECT_EQ(one, one_mod_p);
}

}  // namespace
