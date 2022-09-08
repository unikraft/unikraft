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
/// Unit tests of Fq implementation.
/*! \file */

#include <gtest/gtest.h>

#include "epid/member/tiny/math/unittests/cmp-testhelper.h"
#include "epid/member/tiny/math/unittests/onetimepad.h"

extern "C" {
#include "epid/member/tiny/math/fq.h"
#include "epid/member/tiny/math/mathtypes.h"
}

namespace {

////////////////////////////////////////////////////////////////////////
// FqInField

TEST(TinyFqTest, FqInFieldPasses) {
  FqElem zero = {0};
  FqElem q_minus_one = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB,
                         0xEE71A49F, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  EXPECT_TRUE(FqInField(&zero));
  EXPECT_TRUE(FqInField(&q_minus_one));
}

TEST(TinyFqTest, FqInFieldFails) {
  FqElem q = {{0xAED33013, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
               0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  EXPECT_FALSE(FqInField(&q));
}

////////////////////////////////////////////////////////////////////////
// FqAdd

TEST(TinyFqTest, FqAddWorks) {
  FqElem result = {0}, expected = {0}, left = {0}, right = {0};
  left.limbs.word[5] = 1;
  right.limbs.word[5] = 2;
  expected.limbs.word[5] = 3;
  FqAdd(&result, &left, &right);
  EXPECT_EQ(expected, result);

  FqElem q_minus_one = {0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB,
                        0xEE71A49F, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF};
  right = {2, 0, 0, 0, 0, 0, 0, 0};
  expected = {1, 0, 0, 0, 0, 0, 0, 0};
  FqAdd(&result, &q_minus_one, &right);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// FqSub

TEST(TinyFqTest, FqSubWorks) {
  FqElem result = {0}, expected = {0}, left = {0}, right = {0};
  left.limbs.word[4] = 2;
  right.limbs.word[4] = 1;
  expected.limbs.word[4] = 1;
  FqSub(&result, &left, &right);
  EXPECT_EQ(expected, result);

  left = {1, 0, 0, 0, 0, 0, 0, 0};
  right = {2, 0, 0, 0, 0, 0, 0, 0};
  FqElem q_minus_one = {0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB,
                        0xEE71A49F, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF};
  FqSub(&result, &left, &right);
  EXPECT_EQ(q_minus_one, result);
}

////////////////////////////////////////////////////////////////////////
// FqMul

TEST(TinyFqTest, FqMulWorks) {
  FqElem left = {0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c,
                 0xafa65357, 0x4780716c, 0xffd94b0f, 0x5e643124};
  FqElem right = {0x848cdb73, 0x6399829e, 0xcaa20cc0, 0x1b02bff6,
                  0x2b477bd2, 0xf9d48534, 0xff7929a0, 0xd4745161};
  FqElem expected = {0x28f2f1dd, 0x2cb2b611, 0xa24767b3, 0x4e880c0e,
                     0xed7f7b9e, 0x6ff4a7f2, 0x25fb15d0, 0x7b8c4fed};
  FqElem result = {0};
  FqMul(&result, &left, &right);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// FqExp
TEST(TinyFqTest, FqExpWorks) {
  FqElem result = {0}, expected = {0}, in = {0};
  VeryLargeInt exp = {0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB,
                      0xEE71A49F, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF};
  in.limbs.word[0] = 1;
  expected.limbs.word[0] = 1;
  FqExp(&result, &in, &exp);
  EXPECT_EQ(expected, result);

  exp = {4, 0, 0, 0, 0, 0, 0, 0};
  in = {{0x0000007B, 0, 0, 0, 0, 0, 0, 0}};
  expected = {{0x0DA48871, 0, 0, 0, 0, 0, 0, 0}};
  FqExp(&result, &in, &exp);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// FqCp

TEST(TinyFqTest, FqCpWorks) {
  FqElem a = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
               0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  FqElem result = {0};
  FqCp(&result, &a);
  EXPECT_EQ(a, result);
}

////////////////////////////////////////////////////////////////////////
// FqIsZero

TEST(TinyFqTest, FqIsZeroPasses) {
  FqElem zero = {0};
  EXPECT_TRUE(FqIsZero(&zero));
}

TEST(TinyFqTest, FqIsZeroFails) {
  FqElem non_zero = {{0, 0, 0, 0, 0, 0, 1, 0}};
  EXPECT_FALSE(FqIsZero(&non_zero));
}

////////////////////////////////////////////////////////////////////////
// FqInv

TEST(TinyFqTest, FqInvWorks) {
  FqElem a = {{0x76abb18a, 0x92c0f7b9, 0x2c1a37e0, 0x7fdf6ca1, 0xe3401760,
               0x66eb7d52, 0x918d50a7, 0x12a65bd6}};
  FqElem expected = {{0x5a686df6, 0x56b6ab63, 0xdf907c6f, 0x44ad8d51,
                      0xa5513462, 0xc597ef78, 0x93711b39, 0x15171a1e}};
  FqElem result;
  FqInv(&result, &a);
  EXPECT_EQ(result, expected);
}

////////////////////////////////////////////////////////////////////////
// FqNeg
TEST(TinyFqTest, FqNegWorks) {
  FqElem const pairing_q = {{0xAED33013, 0xD3292DDB, 0x12980A82, 0x0CDC65FB,
                             0xEE71A49F, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  FqElem neg_value = {0};
  FqElem one = {{1, 0, 0, 0, 0, 0, 0, 0}};
  FqElem minus_one = pairing_q;
  --minus_one.limbs.word[0];
  FqNeg(&neg_value, &one);
  EXPECT_EQ(minus_one, neg_value);

  FqElem value = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
                   0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  FqNeg(&neg_value, &value);
  FqNeg(&neg_value, &neg_value);
  EXPECT_EQ(value, neg_value);
}

////////////////////////////////////////////////////////////////////////
// FqSquare

TEST(TinyFqTest, FqSquareWorks) {
  FqElem const pairing_q = {{0xAED33013, 0xD3292DDB, 0x12980A82, 0x0CDC65FB,
                             0xEE71A49F, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  FqElem four = {{4, 0, 0, 0, 0, 0, 0, 0}}, result = {0};
  FqElem minus_two = pairing_q;
  minus_two.limbs.word[0] -= 2;
  FqSquare(&result, &minus_two);
  EXPECT_EQ(four, result);

  FqElem in = {{0x00003039, 0, 0, 0, 0, 0, 0, 0}};
  FqElem expected = {{0x09156CB1, 0, 0, 0, 0, 0, 0, 0}};
  FqSquare(&result, &in);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// FqClear

TEST(TinyFqTest, FqClearWorks) {
  FqElem zero = {0};
  FqElem a = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
               0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  FqClear(&a);
  EXPECT_EQ(zero, a);
}

////////////////////////////////////////////////////////////////////////
// FqSet

TEST(TinyFqTest, FqSetWorks) {
  uint32_t small = 0xffffffff;
  FqElem expected = {{small, 0, 0, 0, 0, 0, 0, 0}};
  FqElem result = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
                    0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  FqSet(&result, small);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// FqEq

TEST(TinyFqTest, FqEqPasses) {
  FqElem a = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
               0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  FqElem c = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
               0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  EXPECT_TRUE(FqEq(&a, &c));
}

TEST(TinyFqTest, FqEqFails) {
  FqElem a = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
               0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  FqElem b = {{0, 0, 0, 0, 0, 0, 1, 0}};
  EXPECT_FALSE(FqEq(&a, &b));
}

////////////////////////////////////////////////////////////////////////
// FqCondSet

TEST(TinyFqTest, FqCondSetWorksForTrue) {
  FqElem a = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
               0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  FqElem b = {{0, 0, 0, 0, 0, 0, 1, 0}};
  FqElem result = {0};
  FqCondSet(&result, &a, &b, true);
  EXPECT_EQ(a, result);
}

TEST(TinyFqTest, FqCondSetWorksForFalse) {
  FqElem a = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
               0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  FqElem b = {{0, 0, 0, 0, 0, 0, 1, 0}};
  FqElem result = {0};
  FqCondSet(&result, &a, &b, false);
  EXPECT_EQ(b, result);
}

////////////////////////////////////////////////////////////////////////
// FqSqrt

TEST(TinyFqTest, FqSqrtWorks) {
  FqElem actual = {0};
  FqElem in = {0x02869F4A, 0xDF28D8AC, 0x2B4BD20F, 0xB158EA77,
               0x89D86073, 0x4D6F2CE1, 0x0211F496, 0x9C977A80};
  FqElem expected = {0x0E41765B, 0x7E2AEDF6, 0x7C01BC1B, 0xB1C0F3E4,
                     0x95CB5637, 0x8B275494, 0x3FBF1556, 0x8DAE1450};

  EXPECT_TRUE(FqSqrt(&actual, &in));
  EXPECT_EQ(expected, actual);
}
TEST(TinyFqTest, FqSqrtWorksForFour) {
  FqElem actual = {0};
  FqElem in = {0x00000004, 0x00000000, 0x00000000, 0x00000000,
               0x00000000, 0x00000000, 0x00000000, 0x00000000};
  FqElem expected = {0xAED33011, 0xD3292DDB, 0x12980A82, 0x0CDC65FB,
                     0xEE71A49F, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF};
  EXPECT_TRUE(FqSqrt(&actual, &in));
  EXPECT_EQ(expected, actual);
}
TEST(TinyFqTest, FqSqrtFailsForFive) {
  FqElem actual = {0};
  FqElem in = {0x00000005, 0x00000000, 0x00000000, 0x00000000,
               0x00000000, 0x00000000, 0x00000000, 0x00000000};
  EXPECT_FALSE(FqSqrt(&actual, &in));
}
////////////////////////////////////////////////////////////////////////
// FqRand

TEST(TinyFqTest, FqRandConsumes384BitsOfEntropy) {
  OneTimePad otp(64);
  FqElem actual = {0};
  EXPECT_TRUE(FqRand(&actual, OneTimePad::Generate, &otp));
  EXPECT_EQ(384u, otp.BitsConsumed());
}

TEST(TinyFqTest, FqRandWorks) {
  OneTimePad otp({// slen bits
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  // q + 1
                  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xF0, 0xCD, 0x46, 0xE5,
                  0xF2, 0x5E, 0xEE, 0x71, 0xA4, 0x9F, 0x0C, 0xDC, 0x65, 0xFB,
                  0x12, 0x98, 0x0A, 0x82, 0xD3, 0x29, 0x2D, 0xDB, 0xAE, 0xD3,
                  0x30, 0x14});
  FqElem expected = {{1, 0, 0, 0, 0, 0, 0, 0}};
  FqElem actual = {0};
  EXPECT_TRUE(FqRand(&actual, OneTimePad::Generate, &otp));
  EXPECT_EQ(expected, actual);
}

////////////////////////////////////////////////////////////////////////
// FqFromHash

TEST(TinyFqTest, FqFromHashWorks) {
  FqElem q_mod_q;
  FqElem zero = {0};
  uint8_t q_str[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xF0, 0xCD,
                       0x46, 0xE5, 0xF2, 0x5E, 0xEE, 0x71, 0xA4, 0x9F,
                       0x0C, 0xDC, 0x65, 0xFB, 0x12, 0x98, 0x0A, 0x82,
                       0xD3, 0x29, 0x2D, 0xDB, 0xAE, 0xD3, 0x30, 0x13};
  FqFromHash(&q_mod_q, q_str, sizeof(q_str));
  EXPECT_EQ(zero, q_mod_q);

  FqElem one = {{1, 0, 0, 0, 0, 0, 0, 0}};
  uint8_t one_str[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
  FqElem one_mod_q;
  FqFromHash(&one_mod_q, one_str, sizeof(one_str));
  EXPECT_EQ(one, one_mod_q);
}

}  // namespace
