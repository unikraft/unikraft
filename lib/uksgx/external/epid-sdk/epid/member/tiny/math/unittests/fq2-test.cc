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
/// Unit tests of Fq2 implementation.
/*! \file */

#include <gtest/gtest.h>

#include "epid/member/tiny/math/unittests/cmp-testhelper.h"

extern "C" {
#include "epid/member/tiny/math/fq2.h"
#include "epid/member/tiny/math/mathtypes.h"
}

namespace {

////////////////////////////////////////////////////////////////////////
// Fq2Cp

TEST(TinyFq2Test, Fq2CpWorks) {
  FqElem fq = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
                0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  Fq2Elem a = {fq, fq};
  Fq2Elem result = {0};
  Fq2Cp(&result, &a);
  EXPECT_EQ(a, result);
}

////////////////////////////////////////////////////////////////////////
// Fq2Set

TEST(TinyFq2Test, Fq2SetWorks) {
  uint32_t small = 0xffffffff;
  Fq2Elem expected = {0};
  expected.x0.limbs.word[0] = small;
  FqElem fq = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
                0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  Fq2Elem result = {fq, fq};
  Fq2Set(&result, small);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// Fq2Clear

TEST(TinyFq2Test, Fq2ClearWorks) {
  Fq2Elem expected = {0};
  FqElem fq = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
                0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  Fq2Elem result = {fq, fq};
  Fq2Clear(&result);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// Fq2Add

TEST(TinyFq2Test, Fq2AddWorks) {
  Fq2Elem result = {0};
  Fq2Elem left = {{0, 0, 0, 0, 0, 5, 0, 0}, {0, 0, 0, 0, 0, 0, 9, 0}};
  Fq2Elem right = {{0, 0, 0, 0, 0, 20, 0, 0}, {0, 0, 0, 0, 0, 0, 30, 0}};
  Fq2Elem expected = {{0, 0, 0, 0, 0, 25, 0, 0}, {0, 0, 0, 0, 0, 0, 39, 0}};
  Fq2Add(&result, &left, &right);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// Fq2Exp

TEST(TinyFq2Test, Fq2ExpWorks) {
  Fq2Elem one = {1};
  Fq2Elem in = {0};
  VeryLargeInt exp = {0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB,
                      0xEE71A49F, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF};
  Fq2Elem result = {{0xb06f3418, 0x16043dfc, 0x7884a9ca, 0x0ab1a427, 0x309307b5,
                     0x23133f6d, 0x250c3dc4, 0x005c4818},
                    {0x3b6f652c, 0x6435c0fd, 0x509b8d30, 0x13fe4bef, 0x5f4ffc14,
                     0x81dde5fd, 0x02b77b89, 0x5b38b2a5}};
  Fq2Exp(&result, &in, &exp);
  EXPECT_EQ(in, result);

  in = {exp, exp};
  exp = {0};
  Fq2Exp(&result, &in, &exp);
  EXPECT_EQ(one, result);

  exp = {2};
  in = {{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357, 0x4780716c,
         0xffd94b0f, 0x5e643124},
        {0x4d23497f, 0x189daf4d, 0x0ac5c478, 0x3583e2b0, 0x34dd5651, 0x1bb8f3e0,
         0x1e1f4181, 0x8aa45bf5}};
  Fq2Elem expected = {{0xb06f3418, 0x16043dfc, 0x7884a9ca, 0x0ab1a427,
                       0x309307b5, 0x23133f6d, 0x250c3dc4, 0x005c4818},
                      {0x3b6f652c, 0x6435c0fd, 0x509b8d30, 0x13fe4bef,
                       0x5f4ffc14, 0x81dde5fd, 0x02b77b89, 0x5b38b2a5}};
  Fq2Exp(&result, &in, &exp);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// Fq2Sub

TEST(TinyFq2Test, Fq2SubWorks) {
  Fq2Elem result = {0};
  Fq2Elem left = {{0, 0, 0, 0, 0, 20, 0, 0}, {0, 0, 0, 0, 0, 0, 30, 0}};
  Fq2Elem right = {{0, 0, 0, 0, 0, 5, 0, 0}, {0, 0, 0, 0, 0, 0, 9, 0}};
  Fq2Elem expected = {{0, 0, 0, 0, 0, 15, 0, 0}, {0, 0, 0, 0, 0, 0, 21, 0}};
  Fq2Sub(&result, &left, &right);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// Fq2Mul

TEST(TinyFq2Test, Fq2MultWorks) {
  Fq2Elem expected = {{0x37861727, 0x52822db7, 0x8005ec64, 0xc0b0bc96,
                       0xd60e07a4, 0x65eee0a2, 0x780dbc26, 0x7b36e4cb},
                      {0x8201d4ed, 0xbf8ed473, 0xc5c09cbe, 0xba9d0095,
                       0x3d91414a, 0xa8ebb728, 0x66bc029b, 0x5b6ca52b}};
  Fq2Elem left = {{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357,
                   0x4780716c, 0xffd94b0f, 0x5e643124},
                  {0x4d23497f, 0x189daf4d, 0x0ac5c478, 0x3583e2b0, 0x34dd5651,
                   0x1bb8f3e0, 0x1e1f4181, 0x8aa45bf5}};
  Fq2Elem right = {{0x848cdb73, 0x6399829e, 0xcaa20cc0, 0x1b02bff6, 0x2b477bd2,
                    0xf9d48534, 0xff7929a0, 0xd4745161},
                   {0xe323d956, 0xf8a05a85, 0xe02d5e1e, 0xfd533966, 0xe7d31209,
                    0xc7786143, 0x91b441f6, 0x7409d67d}};
  Fq2Elem actual = {0};
  Fq2Mul(&actual, &left, &right);
  EXPECT_EQ(expected, actual);
}

////////////////////////////////////////////////////////////////////////
// Fq2Inv

TEST(TinyFq2Test, Fq2InvWorks) {
  Fq2Elem expected = {{0xffa2241d, 0x37481517, 0x4bc3ad3a, 0x3c57cefe,
                       0xb7a57680, 0xaef8eb15, 0x6a1f7923, 0x2e6a7077},
                      {0xdc8eb865, 0xe264dc4c, 0xa2a174dc, 0xa18b8fe8,
                       0x31ee8433, 0xdea6fa81, 0x7ec16a0e, 0x1b0f8f81}};
  Fq2Elem left = {{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357,
                   0x4780716c, 0xffd94b0f, 0x5e643124},
                  {0x4d23497f, 0x189daf4d, 0x0ac5c478, 0x3583e2b0, 0x34dd5651,
                   0x1bb8f3e0, 0x1e1f4181, 0x8aa45bf5}};
  Fq2Elem actual = {0};
  Fq2Inv(&actual, &left);
  EXPECT_EQ(expected, actual);
}

TEST(TinyFq2Test, Fq2InvWorksInPlace) {
  Fq2Elem expected = {{0xffa2241d, 0x37481517, 0x4bc3ad3a, 0x3c57cefe,
                       0xb7a57680, 0xaef8eb15, 0x6a1f7923, 0x2e6a7077},
                      {0xdc8eb865, 0xe264dc4c, 0xa2a174dc, 0xa18b8fe8,
                       0x31ee8433, 0xdea6fa81, 0x7ec16a0e, 0x1b0f8f81}};
  Fq2Elem left = {{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357,
                   0x4780716c, 0xffd94b0f, 0x5e643124},
                  {0x4d23497f, 0x189daf4d, 0x0ac5c478, 0x3583e2b0, 0x34dd5651,
                   0x1bb8f3e0, 0x1e1f4181, 0x8aa45bf5}};
  Fq2Inv(&left, &left);
  EXPECT_EQ(expected, left);
}
////////////////////////////////////////////////////////////////////////
// Fq2Neg

TEST(TinyFq2Test, Fq2NegWorks) {
  FqElem const q = {{0xAED33013, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
                     0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  Fq2Elem neg_value = {0};
  Fq2Elem one = {{1}, {1}};
  Fq2Elem minus_one = {q, q};
  --minus_one.x0.limbs.word[0];
  --minus_one.x1.limbs.word[0];
  Fq2Neg(&neg_value, &one);
  EXPECT_EQ(minus_one, neg_value);

  Fq2Elem value = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
                    0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF},
                   {0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
                    0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  Fq2Neg(&neg_value, &value);
  Fq2Neg(&neg_value, &neg_value);
  EXPECT_EQ(value, neg_value);
  value = {{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357,
            0x4780716c, 0xffd94b0f, 0x5e643124},
           {0x4d23497f, 0x189daf4d, 0x0ac5c478, 0x3583e2b0, 0x34dd5651,
            0x1bb8f3e0, 0x1e1f4181, 0x8aa45bf5}};
  Fq2Elem expected = {{0x8c035971, 0xaf40febd, 0x3d89f632, 0x24887d6e,
                       0x3ecb5147, 0xff6580f2, 0x0023a5bd, 0xa19bcedb},
                      {0x61afe694, 0xba8b7e8e, 0x07d2460a, 0xd758834b,
                       0xb9944e4d, 0x2b2cfe7e, 0xe1ddaf4c, 0x755ba40a}};
  Fq2Neg(&neg_value, &value);
  EXPECT_EQ(expected, neg_value);
}

////////////////////////////////////////////////////////////////////////
// Fq2Conj

TEST(TinyFq2Test, Fq2ConjWorks) {
  Fq2Elem expected = {{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c,
                       0xafa65357, 0x4780716c, 0xffd94b0f, 0x5e643124},
                      {0x61afe694, 0xba8b7e8e, 0x07d2460a, 0xd758834b,
                       0xb9944e4d, 0x2b2cfe7e, 0xe1ddaf4c, 0x755ba40a}};
  Fq2Elem left = {{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357,
                   0x4780716c, 0xffd94b0f, 0x5e643124},
                  {0x4d23497f, 0x189daf4d, 0x0ac5c478, 0x3583e2b0, 0x34dd5651,
                   0x1bb8f3e0, 0x1e1f4181, 0x8aa45bf5}};
  Fq2Elem actual = {0};
  Fq2Conj(&actual, &left);
  EXPECT_EQ(expected, actual);
}

////////////////////////////////////////////////////////////////////////
// Fq2Square

TEST(TinyFq2Test, Fq2SquareWorks) {
  Fq2Elem expected = {{0xb06f3418, 0x16043dfc, 0x7884a9ca, 0x0ab1a427,
                       0x309307b5, 0x23133f6d, 0x250c3dc4, 0x005c4818},
                      {0x3b6f652c, 0x6435c0fd, 0x509b8d30, 0x13fe4bef,
                       0x5f4ffc14, 0x81dde5fd, 0x02b77b89, 0x5b38b2a5}};
  Fq2Elem result = {0};
  Fq2Elem in = {{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357,
                 0x4780716c, 0xffd94b0f, 0x5e643124},
                {0x4d23497f, 0x189daf4d, 0x0ac5c478, 0x3583e2b0, 0x34dd5651,
                 0x1bb8f3e0, 0x1e1f4181, 0x8aa45bf5}};
  Fq2Square(&result, &in);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// Fq2MulScalar

TEST(TinyFq2Test, Fq2MulScalarWorks) {
  Fq2Elem expected = {{0x28f2f1dd, 0x2cb2b611, 0xa24767b3, 0x4e880c0e,
                       0xed7f7b9e, 0x6ff4a7f2, 0x25fb15d0, 0x7b8c4fed},
                      {0}};
  Fq2Elem left = {{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357,
                   0x4780716c, 0xffd94b0f, 0x5e643124},
                  {0}};
  FqElem scalar = {0x848cdb73, 0x6399829e, 0xcaa20cc0, 0x1b02bff6,
                   0x2b477bd2, 0xf9d48534, 0xff7929a0, 0xd4745161};
  Fq2Elem actual = {0};
  Fq2MulScalar(&actual, &left, &scalar);
  EXPECT_EQ(expected, actual);
}

TEST(TinyFq2Test, Fq2MulScalarWorksInPlace) {
  Fq2Elem expected = {{0x28f2f1dd, 0x2cb2b611, 0xa24767b3, 0x4e880c0e,
                       0xed7f7b9e, 0x6ff4a7f2, 0x25fb15d0, 0x7b8c4fed},
                      {0}};
  Fq2Elem left = {{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357,
                   0x4780716c, 0xffd94b0f, 0x5e643124},
                  {0}};
  FqElem scalar = {0x848cdb73, 0x6399829e, 0xcaa20cc0, 0x1b02bff6,
                   0x2b477bd2, 0xf9d48534, 0xff7929a0, 0xd4745161};
  Fq2MulScalar(&left, &left, &scalar);
  EXPECT_EQ(expected, left);
}
////////////////////////////////////////////////////////////////////////
// Fq2CondSet

TEST(TinyFq2Test, Fq2CondSetWorksForTrue) {
  Fq2Elem a = {{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357,
                0x4780716c, 0xffd94b0f, 0x5e64364},
               {0x4d23497f, 0x189daf4d, 0x0ac5c478, 0x3583e2b0, 0x34dd5651,
                0x1bb8f3e0, 0x1e1f4181, 0x8aa45bf5}};
  Fq2Elem b = {{0, 0, 0, 0, 0, 0, 1, 0}, {0, 0, 0, 0, 0, 0, 1, 0}};
  Fq2Elem result = {0};
  Fq2CondSet(&result, &a, &b, true);
  EXPECT_EQ(a, result);
}

TEST(TinyFq2Test, Fq2CondSetWorksForFalse) {
  Fq2Elem a = {{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357,
                0x4780716c, 0xffd94b0f, 0x5e64364},
               {0x4d23497f, 0x189daf4d, 0x0ac5c478, 0x3583e2b0, 0x34dd5651,
                0x1bb8f3e0, 0x1e1f4181, 0x8aa45bf5}};
  Fq2Elem b = {{0, 0, 0, 0, 0, 0, 1, 0}, {0, 0, 0, 0, 0, 0, 1, 0}};
  Fq2Elem result = {0};
  Fq2CondSet(&result, &a, &b, false);
  EXPECT_EQ(b, result);
}

////////////////////////////////////////////////////////////////////////
// Fq2Eq

TEST(TinyFq2Test, Fq2EqPasses) {
  FqElem fq_a = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
                  0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  FqElem fq_b = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
                  0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  Fq2Elem a = {fq_a, fq_a};
  Fq2Elem b = {fq_b, fq_b};
  EXPECT_TRUE(Fq2Eq(&a, &b));
}

TEST(TinyFq2Test, Fq2EqFails) {
  FqElem fq_a = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
                  0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  FqElem fq_b = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
                  0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  Fq2Elem a = {fq_a, fq_a};
  Fq2Elem b = {fq_b, fq_b};
  --b.x1.limbs.word[5];
  EXPECT_FALSE(Fq2Eq(&a, &b));
}

////////////////////////////////////////////////////////////////////////
// Fq2MulXi

TEST(TinyFq2Test, Fq2MultXiWorks) {
  const Fq2Elem a = {{0xbca2b7aa, 0xc0e43294, 0x6199e561, 0xefdb7a39,
                      0xd57bcbba, 0x03154f2a, 0xdf9e1797, 0xf52d29c1},
                     {0x77cb909b, 0x906d8657, 0xfea2ffb3, 0x7810e964,
                      0x022e47c1, 0x862bdbe6, 0xe4f5d59b, 0xa677247d}};
  const Fq2Elem expected = {{0x52a6aea6, 0x1e31b0f6, 0xb1f8c08d, 0x5ac9a512,
                             0xba57ab15, 0x3918d010, 0xda4968c5, 0x43e32f05},
                            {0x4e9378ba, 0x3b6ce38c, 0x39afcfc3, 0xc644810d,
                             0xfcf511ff, 0x81a12238, 0xa98fe133, 0x421b72bd}};

  Fq2Elem res;
  Fq2MulXi(&res, &a);
  EXPECT_EQ(expected, res);
}

////////////////////////////////////////////////////////////////////////
// Fq2IsZero

TEST(TinyFq2Test, Fq2IsZeroPasses) {
  Fq2Elem zero = {0};
  EXPECT_TRUE(Fq2IsZero(&zero));
}

TEST(TinyFq2Test, Fq2IsZeroFails) {
  Fq2Elem non_zero = {0};
  ++non_zero.x0.limbs.word[6];
  EXPECT_FALSE(Fq2IsZero(&non_zero));
}

}  // namespace
