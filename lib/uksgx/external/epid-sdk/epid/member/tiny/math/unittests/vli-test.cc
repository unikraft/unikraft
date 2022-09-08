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
/// Unit tests of large integer implementation.
/*! \file */

#include <gtest/gtest.h>
#include <limits.h>  // for CHAR_BIT
#include <cstring>
#include <random>

#include "epid/member/tiny/math/unittests/cmp-testhelper.h"
#include "epid/member/tiny/math/unittests/onetimepad.h"

extern "C" {
#include "epid/member/tiny/math/mathtypes.h"
#include "epid/member/tiny/math/vli.h"
}

namespace {

////////////////////////////////////////////////////////////////////////
// VliAdd

TEST(TinyVliTest, VliAddWorks) {
  VeryLargeInt result = {0};
  VeryLargeInt expected = {0};
  VeryLargeInt left = {0};
  VeryLargeInt right = {0};
  left.word[0] = 1;
  right.word[0] = 2;
  expected.word[0] = 3;
  VliAdd(&result, &left, &right);
  EXPECT_EQ(expected, result);
}
TEST(TinyVliTest, VliAddCalculatesCarry) {
  VeryLargeInt left = {{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
                        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}};
  VeryLargeInt right = {{0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1}};
  VeryLargeInt expected = {{0x0, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1}};
  uint32_t expected_carry = 0x1;
  uint32_t carry = 0;
  VeryLargeInt result = {0};
  carry = VliAdd(&result, &left, &right);
  EXPECT_EQ(expected, result);
  EXPECT_EQ(expected_carry, carry);
}
////////////////////////////////////////////////////////////////////////
// VliMul
TEST(TinyVliTest, VliMultWorks) {
  VeryLargeIntProduct result = {0};
  VeryLargeIntProduct expected = {0};
  VeryLargeInt left = {0}, right = {0};
  left.word[0] = 2;
  right.word[0] = 2;
  expected.word[0] = 4;
  VliMul(&result, &left, &right);
  EXPECT_EQ(expected, result);
}
TEST(TinyVliTest, VliMultWorksWithOverflow) {
  VeryLargeIntProduct result = {0};
  VeryLargeIntProduct expected = {
      {0xfffffffe, 0xfffffffd, 0xfffffffd, 0xfffffffd, 0xfffffffd, 0xfffffffd,
       0xfffffffd, 0xfffffffd, 0x1, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2}};
  VeryLargeInt left = {{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
                        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}};
  VeryLargeInt right = {{0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2}};
  VliMul(&result, &left, &right);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// VliRShift
TEST(TinyVliTest, VliRShiftWorks) {
  VeryLargeInt result = {0}, expected = {0};
  VeryLargeInt in = {0};
  uint32_t shift = 1;
  in.word[0] = 4;
  expected.word[0] = 2;
  VliRShift(&result, &in, shift);
  EXPECT_EQ(expected, result);
}
TEST(TinyVliTest, VliRShiftWorksWithOverlap) {
  VeryLargeInt result = {0}, expected = {0};
  VeryLargeInt in = {0};
  uint32_t shift = 4;
  in.word[0] = 0x00000008;
  in.word[1] = 0xffffffff;
  expected.word[0] = 0xf0000000;
  expected.word[1] = 0x0fffffff;
  VliRShift(&result, &in, shift);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// VliSub
TEST(TinyVliTest, VliSubWorks) {
  VeryLargeInt result = {0}, expected = {0};
  VeryLargeInt left = {0}, right = {0};
  uint32_t borrow = 0;
  uint32_t expected_borrow = 0;
  left.word[0] = 4;
  right.word[0] = 2;
  expected.word[0] = 2;
  borrow = VliSub(&result, &left, &right);
  EXPECT_EQ(expected, result);
  EXPECT_EQ(expected_borrow, borrow);
}
TEST(TinyVliTest, VliSubWorksWithBorrow) {
  VeryLargeInt result = {0};
  VeryLargeInt expected = {{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
                            0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}};
  VeryLargeInt left = {0}, right = {0};
  uint32_t borrow = 0;
  uint32_t expected_borrow = 1;
  left.word[0] = 2;
  right.word[0] = 3;
  borrow = VliSub(&result, &left, &right);
  EXPECT_EQ(expected, result);
  EXPECT_EQ(expected_borrow, borrow);
}

////////////////////////////////////////////////////////////////////////
// VliSet

TEST(TinyVliTest, VliSetWorks) {
  VeryLargeInt result = {0};
  VeryLargeInt in = {{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
                      0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}};
  VliSet(&result, &in);
  EXPECT_EQ(in, result);
}

////////////////////////////////////////////////////////////////////////
// VliClear

TEST(TinyVliTest, VliClearWorks) {
  VeryLargeInt expected = {0};
  VeryLargeInt in_out = {{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
                          0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}};
  VliClear(&in_out);
  EXPECT_EQ(expected, in_out);
}

////////////////////////////////////////////////////////////////////////
// VliIsZero

TEST(TinyVliTest, VliIsZeroAcceptsZero) {
  int is_zero = 0;
  VeryLargeInt in_zero = {0};
  is_zero = VliIsZero(&in_zero);
  EXPECT_TRUE(is_zero);
}
TEST(TinyVliTest, VliIsZeroRejectsNonZero) {
  int is_zero = 0;
  VeryLargeInt in_nonzero = {{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
                              0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}};
  is_zero = VliIsZero(&in_nonzero);
  EXPECT_FALSE(is_zero);
}

////////////////////////////////////////////////////////////////////////
// VliCondSet

TEST(TinyVliTest, VliCondSetWorksForTrue) {
  VeryLargeInt result = {0};
  VeryLargeInt true_val = {{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
                            0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}};
  VeryLargeInt false_val = {{0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                             0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa}};
  VliCondSet(&result, &true_val, &false_val, 1);
  EXPECT_EQ(true_val, result);
}
TEST(TinyVliTest, VliCondSetWorksForFalse) {
  VeryLargeInt result = {0};
  VeryLargeInt true_val = {{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
                            0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}};
  VeryLargeInt false_val = {{0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                             0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa}};
  VliCondSet(&result, &true_val, &false_val, 0);
  EXPECT_EQ(false_val, result);
}

////////////////////////////////////////////////////////////////////////
// VliTestBit

TEST(TinyVliTest, VliTestBitWorks) {
  VeryLargeInt in = {0};
  uint32_t bit_set = 0;
  in.word[0] = 4;
  bit_set = VliTestBit(&in, 1);
  EXPECT_EQ((uint32_t)0, bit_set);
  bit_set = VliTestBit(&in, 2);
  EXPECT_EQ((uint32_t)1, bit_set);
}

////////////////////////////////////////////////////////////////////////
// VliRand

TEST(TinyVliTest, VliRandWorks) {
  OneTimePad my_prng;
  VeryLargeInt expected_rand_val1 = {{1, 0, 0, 0, 0, 0, 0, 0}};
  my_prng.InitUint8({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1});
  VeryLargeInt rand_val1 = {0};
  EXPECT_TRUE(VliRand(&rand_val1, &OneTimePad::Generate, &my_prng));
  EXPECT_EQ(expected_rand_val1, rand_val1);
  EXPECT_EQ(256u, my_prng.BitsConsumed());

  VeryLargeInt expected_rand_val2 = {{0x1c6f5a0f, 0xeaa878b3, 0xc71dab6b,
                                      0x1a101ad6, 0x1fe6394f, 0x1bec36ab,
                                      0x07a3e97f, 0x36507914}};
  VeryLargeInt rand_val2 = {0};
  my_prng.InitUint32({0x14795036, 0x7fe9a307, 0xab36ec1b, 0x4f39e61f,
                      0xd61a101a, 0x6bab1dc7, 0xb378a8ea, 0x0f5a6f1c});
  EXPECT_TRUE(VliRand(&rand_val2, &OneTimePad::Generate, &my_prng));
  EXPECT_EQ(expected_rand_val2, rand_val2);
  EXPECT_EQ(256u, my_prng.BitsConsumed());
}

////////////////////////////////////////////////////////////////////////
// VliCmp

TEST(TinyVliTest, VliCmpWorksForLessThan) {
  VeryLargeInt in_val1 = {0};
  VeryLargeInt in_val2 = {0};
  int res = 0;
  in_val1.word[0] = 1;
  in_val2.word[0] = 2;
  res = VliCmp(&in_val1, &in_val2);
  EXPECT_EQ(-1, res);
}
TEST(TinyVliTest, VliCmpWorksForEqual) {
  VeryLargeInt in_val1 = {0};
  VeryLargeInt in_val2 = {0};
  int res = 0;
  in_val1.word[0] = 2;
  in_val2.word[0] = 2;
  res = VliCmp(&in_val1, &in_val2);
  EXPECT_EQ(0, res);
}
TEST(TinyVliTest, VliCmpWorksGreaterThan) {
  VeryLargeInt in_val1 = {0};
  VeryLargeInt in_val2 = {0};
  int res = 0;
  in_val1.word[0] = 1;
  in_val2.word[0] = 2;
  res = VliCmp(&in_val2, &in_val1);
  EXPECT_EQ(1, res);
}

////////////////////////////////////////////////////////////////////////
// VliModAdd

TEST(TinyVliTest, VliModAddWorks) {
  VeryLargeInt result = {0};
  VeryLargeInt left = {0};
  VeryLargeInt right = {0};
  VeryLargeInt expected = {0};
  VeryLargeInt mod = {0xAED33013, 0xD3292DDB, 0x12980A82, 0x0CDC65FB,
                      0xEE71A49F, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF};
  left.word[0] = 0x9;
  right.word[0] = 0x8;
  expected.word[0] = 0x11;
  VliModAdd(&result, &left, &right, &mod);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// VliModSub

TEST(TinyVliTest, VliModSubWorks) {
  VeryLargeInt result = {0};
  VeryLargeInt left = {0};
  VeryLargeInt right = {0};
  VeryLargeInt expected = {0};
  VeryLargeInt mod = {0xAED33013, 0xD3292DDB, 0x12980A82, 0x0CDC65FB,
                      0xEE71A49F, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF};
  left.word[0] = 0x18;
  right.word[0] = 0x12;
  expected.word[0] = 0x6;
  VliModSub(&result, &left, &right, &mod);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// VliModMul

TEST(TinyVliTest, VliModMultWorks) {
  VeryLargeInt result = {0};
  VeryLargeInt left = {0};
  VeryLargeInt right = {0};
  VeryLargeInt expected = {0};
  VeryLargeInt mod = {0xAED33013, 0xD3292DDB, 0x12980A82, 0x0CDC65FB,
                      0xEE71A49F, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF};
  left.word[0] = 0x10;
  right.word[0] = 0x2;
  expected.word[0] = 0x20;
  VliModMul(&result, &left, &right, &mod);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// VliModExp

TEST(TinyVliTest, VliModExpWorks) {
  VeryLargeInt result = {0};
  VeryLargeInt base = {0};
  VeryLargeInt exp = {0};
  VeryLargeInt mod = {0xAED33013, 0xD3292DDB, 0x12980A82, 0x0CDC65FB,
                      0xEE71A49F, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF};
  VeryLargeInt expected = {0};
  base.word[0] = 0x4;
  exp.word[0] = 0x2;
  expected.word[0] = 0x10;
  VliModExp(&result, &base, &exp, &mod);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// VliModInv

TEST(TinyVliTest, VliModInvWorks) {
  VeryLargeInt a = {0x76abb18a, 0x92c0f7b9, 0x2c1a37e0, 0x7fdf6ca1,
                    0xe3401760, 0x66eb7d52, 0x918d50a7, 0x12a65bd6};
  VeryLargeInt q = {0xAED33013, 0xD3292DDB, 0x12980A82, 0x0CDC65FB,
                    0xEE71A49F, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF};
  VeryLargeInt expected = {0x5a686df6, 0x56b6ab63, 0xdf907c6f, 0x44ad8d51,
                           0xa5513462, 0xc597ef78, 0x93711b39, 0x15171a1e};
  VeryLargeInt result;
  VliModInv(&result, &a, &q);
  EXPECT_EQ(result, expected);
}

////////////////////////////////////////////////////////////////////////
// VliModSquare

TEST(TinyVliTest, VliModSquareWorks) {
  VeryLargeInt result = {0};
  VeryLargeInt input = {0};
  VeryLargeInt mod = {0xAED33013, 0xD3292DDB, 0x12980A82, 0x0CDC65FB,
                      0xEE71A49F, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF};
  VeryLargeInt expected = {0};
  input.word[0] = 0x4;
  expected.word[0] = 0x10;
  VliModSquare(&result, &input, &mod);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// VliModBarrett

TEST(TinyVliTest, VliModBarrettWorks) {
  VeryLargeInt result = {0};
  VeryLargeIntProduct product = {0xAED33013, 0xD3292DDB, 0x12980A82, 0x0CDC65FB,
                                 0xEE71A49F, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF,
                                 0x0,        0x0,        0x0,        0x0,
                                 0x0,        0x0,        0x0,        0x0};
  VeryLargeInt mod = {0xAED33013, 0xD3292DDB, 0x12980A82, 0x0CDC65FB,
                      0xEE71A49F, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF};
  VeryLargeInt expected = {0};
  product.word[0] += 0xF;
  expected.word[0] = 0xF;
  VliModBarrett(&result, &product, &mod);
  EXPECT_EQ(expected, result);
}
}  // namespace
