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
/// Unit tests of Fq6 implementation.
/*! \file */

#include <gtest/gtest.h>

#include "epid/member/tiny/math/unittests/cmp-testhelper.h"

extern "C" {
#include "epid/member/tiny/math/fq6.h"
#include "epid/member/tiny/math/mathtypes.h"
}

namespace {

////////////////////////////////////////////////////////////////////////
// Fq6Add

TEST(TinyFq6Test, Fq6AddWorks) {
  Fq6Elem expected = {{{0xf8898202, 0xb45883e0, 0x8d18168d, 0xf67a4288,
                        0xec7c2a8a, 0xfa6f0441, 0xff5583e2, 0x32d88286},
                       {0x304722d5, 0x113e09d3, 0xeaf32297, 0x32d71c16,
                        0x1cb0685b, 0xe3315524, 0xafd38377, 0xfeae3272}},
                      {{0xc82d8518, 0x99cec06c, 0xe8d7d62b, 0x2f00d5c5,
                        0x0620146e, 0x2741e62b, 0x9e7557ca, 0xeb019297},
                       {0x15cfc66e, 0x55ba87c0, 0xf1060150, 0xb91180f6,
                        0xb0d77f5f, 0xc7fa8733, 0x7dc697f1, 0x1e6b8e24}},
                      {{0x1400db26, 0x0760a0b1, 0x1a64565f, 0x27b5641e,
                        0xd96484ed, 0x54b02297, 0xa2a57172, 0xc3a04c1c},
                       {0x17ff87fb, 0x6294fcdb, 0x8f7c4bc2, 0xd0802fe6,
                        0xee3d7f1b, 0xf19d63af, 0xdb1d00ae, 0x80df53e6}}};
  Fq6Elem left = {{{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357,
                    0x4780716c, 0xffd94b0f, 0x5e643124},
                   {0x4d23497f, 0x189daf4d, 0x0ac5c478, 0x3583e2b0, 0x34dd5651,
                    0x1bb8f3e0, 0x1e1f4181, 0x8aa45bf5}},
                  {{0xaf9cf50f, 0xdfb22086, 0x170f8bcb, 0x75bbd889, 0x5d391be8,
                    0x88aa7982, 0xafae53e0, 0xecd73957},
                   {0x2e2e734a, 0xf6e7e2b7, 0x90b6df9f, 0x2eef83b7, 0x5000252c,
                    0x1837550e, 0x1f45a674, 0x93d557cd}},
                  {{0xcb5e99ef, 0x42374c61, 0x323344a2, 0x6aa052c7, 0x6e58060a,
                    0x38ee9923, 0xa18c3407, 0x582edecf},
                   {0x873d4160, 0xb0e68183, 0x6cade2bc, 0x77ba8f60, 0x26ec9f79,
                    0xc8b2c612, 0x79e80d5f, 0x180fdb61}}};
  Fq6Elem right = {{{0x848cdb73, 0x6399829e, 0xcaa20cc0, 0x1b02bff6, 0x2b477bd2,
                     0xf9d48534, 0xff7929a0, 0xd4745161},
                    {0xe323d956, 0xf8a05a85, 0xe02d5e1e, 0xfd533966, 0xe7d31209,
                     0xc7786143, 0x91b441f6, 0x7409d67d}},
                   {{0xc763c01c, 0x8d45cdc1, 0xe46054e2, 0xc6216337, 0x97589d24,
                     0xe57d5f07, 0xeec3f4b6, 0xfe2a593f},
                    {0x96748337, 0x31fbd2e4, 0x72e72c33, 0x96fe633a, 0x4f48fed2,
                     0xf6a92484, 0x5e7de24a, 0x8a963657}},
                   {{0x48a24137, 0xc529544f, 0xe83111bc, 0xbd151156, 0x6b0c7ee2,
                     0x1bc18974, 0x01193d6b, 0x6b716d4d},
                    {0x90c2469b, 0xb1ae7b57, 0x22ce6905, 0x58c5a086, 0xc750dfa2,
                     0x28ea9d9d, 0x6134f34f, 0x68cf7885}}};
  Fq6Elem actual = {0};
  Fq6Add(&actual, &left, &right);
  EXPECT_EQ(expected, actual);
}

////////////////////////////////////////////////////////////////////////
// Fq6Sub

TEST(TinyFq6Test, Fq6SubWorks) {
  Fq6Elem expected = {{{0x4d162b42, 0x9377da5b, 0x1d041212, 0xda2d8e91,
                        0x72d07c24, 0x9491de97, 0x005d123b, 0x89efdfc3},
                       {0x69ff7029, 0x1ffd54c7, 0x2a986659, 0x3830a949,
                        0x4d0a4447, 0x5440929c, 0x8c6aff8a, 0x169a8577}},
                      {{0x970c6506, 0x259580a0, 0x4547416c, 0xbc76db4c,
                        0xb4522362, 0xea130cd9, 0xc0e74ff6, 0xeeace017},
                       {0x97b9f013, 0xc4ec0fd2, 0x1dcfb36c, 0x97f1207d,
                        0x00b72659, 0x218e308a, 0xc0c7c429, 0x093f2175}},
                      {{0x318f88cb, 0x503725ee, 0x5c9a3d68, 0xba67a76b,
                        0xf1bd2bc6, 0x6413020d, 0xa06fe769, 0xecbd7182},
                       {0xa54e2ad8, 0xd2613407, 0x5c778439, 0x2bd154d5,
                        0x4e0d6476, 0xe6ae1ad3, 0x18b00add, 0xaf4062dc}}};
  Fq6Elem left = {{{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357,
                    0x4780716c, 0xffd94b0f, 0x5e643124},
                   {0x4d23497f, 0x189daf4d, 0x0ac5c478, 0x3583e2b0, 0x34dd5651,
                    0x1bb8f3e0, 0x1e1f4181, 0x8aa45bf5}},
                  {{0xaf9cf50f, 0xdfb22086, 0x170f8bcb, 0x75bbd889, 0x5d391be8,
                    0x88aa7982, 0xafae53e0, 0xecd73957},
                   {0x2e2e734a, 0xf6e7e2b7, 0x90b6df9f, 0x2eef83b7, 0x5000252c,
                    0x1837550e, 0x1f45a674, 0x93d557cd}},
                  {{0xcb5e99ef, 0x42374c61, 0x323344a2, 0x6aa052c7, 0x6e58060a,
                    0x38ee9923, 0xa18c3407, 0x582edecf},
                   {0x873d4160, 0xb0e68183, 0x6cade2bc, 0x77ba8f60, 0x26ec9f79,
                    0xc8b2c612, 0x79e80d5f, 0x180fdb61}}};
  Fq6Elem right = {{{0x848cdb73, 0x6399829e, 0xcaa20cc0, 0x1b02bff6, 0x2b477bd2,
                     0xf9d48534, 0xff7929a0, 0xd4745161},
                    {0xe323d956, 0xf8a05a85, 0xe02d5e1e, 0xfd533966, 0xe7d31209,
                     0xc7786143, 0x91b441f6, 0x7409d67d}},
                   {{0xc763c01c, 0x8d45cdc1, 0xe46054e2, 0xc6216337, 0x97589d24,
                     0xe57d5f07, 0xeec3f4b6, 0xfe2a593f},
                    {0x96748337, 0x31fbd2e4, 0x72e72c33, 0x96fe633a, 0x4f48fed2,
                     0xf6a92484, 0x5e7de24a, 0x8a963657}},
                   {{0x48a24137, 0xc529544f, 0xe83111bc, 0xbd151156, 0x6b0c7ee2,
                     0x1bc18974, 0x01193d6b, 0x6b716d4d},
                    {0x90c2469b, 0xb1ae7b57, 0x22ce6905, 0x58c5a086, 0xc750dfa2,
                     0x28ea9d9d, 0x6134f34f, 0x68cf7885}}};
  Fq6Elem actual = {0};
  Fq6Sub(&actual, &left, &right);
  EXPECT_EQ(expected, actual);
}

////////////////////////////////////////////////////////////////////////
// Fq6Mul

TEST(TinyFq6Test, Fq6MultWorks) {
  Fq6Elem expected = {{{0xf45502f6, 0x81c1eb6d, 0xb43cb376, 0xd0da40d4,
                        0xd6156988, 0x6cd4b676, 0x68079d3e, 0x5030f7d9},
                       {0x8e5857ea, 0x042705fb, 0xac87e99c, 0x10dba783,
                        0xc8a7141b, 0xf742b852, 0x007d20f9, 0x0f6497d4}},
                      {{0xf30a6588, 0xa558efef, 0xe2dac9b0, 0x1586ed6a,
                        0x21de4041, 0xc02e9d19, 0x6707fbb5, 0xbe0be53f},
                       {0x13920431, 0x4935916a, 0x939a8ef8, 0x12230898,
                        0xecbf17a9, 0x0934361c, 0x148c5974, 0x73a766d6}},
                      {{0xb989db6c, 0xc1c3c771, 0xc1a12757, 0xad67b5d9,
                        0xed10d66b, 0x9611f300, 0x3874e3cf, 0x9384e9bf},
                       {0x7acace21, 0x0efe39f0, 0x495ed747, 0xd48f3503,
                        0x6b032eeb, 0xb6430ee5, 0x1a864a16, 0xe497f2b6}}};
  Fq6Elem left = {{{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357,
                    0x4780716c, 0xffd94b0f, 0x5e643124},
                   {0x4d23497f, 0x189daf4d, 0x0ac5c478, 0x3583e2b0, 0x34dd5651,
                    0x1bb8f3e0, 0x1e1f4181, 0x8aa45bf5}},
                  {{0xaf9cf50f, 0xdfb22086, 0x170f8bcb, 0x75bbd889, 0x5d391be8,
                    0x88aa7982, 0xafae53e0, 0xecd73957},
                   {0x2e2e734a, 0xf6e7e2b7, 0x90b6df9f, 0x2eef83b7, 0x5000252c,
                    0x1837550e, 0x1f45a674, 0x93d557cd}},
                  {{0xcb5e99ef, 0x42374c61, 0x323344a2, 0x6aa052c7, 0x6e58060a,
                    0x38ee9923, 0xa18c3407, 0x582edecf},
                   {0x873d4160, 0xb0e68183, 0x6cade2bc, 0x77ba8f60, 0x26ec9f79,
                    0xc8b2c612, 0x79e80d5f, 0x180fdb61}}};
  Fq6Elem right = {{{0x848cdb73, 0x6399829e, 0xcaa20cc0, 0x1b02bff6, 0x2b477bd2,
                     0xf9d48534, 0xff7929a0, 0xd4745161},
                    {0xe323d956, 0xf8a05a85, 0xe02d5e1e, 0xfd533966, 0xe7d31209,
                     0xc7786143, 0x91b441f6, 0x7409d67d}},
                   {{0xc763c01c, 0x8d45cdc1, 0xe46054e2, 0xc6216337, 0x97589d24,
                     0xe57d5f07, 0xeec3f4b6, 0xfe2a593f},
                    {0x96748337, 0x31fbd2e4, 0x72e72c33, 0x96fe633a, 0x4f48fed2,
                     0xf6a92484, 0x5e7de24a, 0x8a963657}},
                   {{0x48a24137, 0xc529544f, 0xe83111bc, 0xbd151156, 0x6b0c7ee2,
                     0x1bc18974, 0x01193d6b, 0x6b716d4d},
                    {0x90c2469b, 0xb1ae7b57, 0x22ce6905, 0x58c5a086, 0xc750dfa2,
                     0x28ea9d9d, 0x6134f34f, 0x68cf7885}}};
  Fq6Elem actual = {0};
  Fq6Mul(&actual, &left, &right);
  EXPECT_EQ(expected, actual);
}

////////////////////////////////////////////////////////////////////////
// Fq6Inv

TEST(TinyFq6Test, Fq6InvWorks) {
  Fq6Elem expected = {{{0x4b9d2361, 0x22a3edb2, 0x4234b1bc, 0x15d4ba45,
                        0x035c34fb, 0xd041dd56, 0x0ac1e096, 0x0352ad8d},
                       {0x4e25c70f, 0x3dc912a9, 0x97a1aa56, 0x32b6e93d,
                        0x19c793ce, 0x951b4bb8, 0x3982345f, 0xec9e815e}},
                      {{0x3d413679, 0x4cdeca84, 0x33e1c617, 0xc8851545,
                        0x51625f1e, 0xcb8421ce, 0xd0a7ea78, 0xbb8a0cd6},
                       {0x77d7ca2e, 0x6a51fadc, 0xec063983, 0xf9a311be,
                        0x28fe40b2, 0x30433e0f, 0x1ed97598, 0x9a82a67f}},
                      {{0x58db3fa8, 0xa999913c, 0x5bef2263, 0x1dba2103,
                        0x0082518b, 0x3d5a20fc, 0x41913c0f, 0x708bddf4},
                       {0x20a372b3, 0x2ebb41eb, 0x9fdb3fa2, 0x473986ff,
                        0x0f5835a9, 0x06121385, 0x8e24ac7e, 0x95b8ca14}}};
  Fq6Elem left = {{{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357,
                    0x4780716c, 0xffd94b0f, 0x5e643124},
                   {0x4d23497f, 0x189daf4d, 0x0ac5c478, 0x3583e2b0, 0x34dd5651,
                    0x1bb8f3e0, 0x1e1f4181, 0x8aa45bf5}},
                  {{0xaf9cf50f, 0xdfb22086, 0x170f8bcb, 0x75bbd889, 0x5d391be8,
                    0x88aa7982, 0xafae53e0, 0xecd73957},
                   {0x2e2e734a, 0xf6e7e2b7, 0x90b6df9f, 0x2eef83b7, 0x5000252c,
                    0x1837550e, 0x1f45a674, 0x93d557cd}},
                  {{0xcb5e99ef, 0x42374c61, 0x323344a2, 0x6aa052c7, 0x6e58060a,
                    0x38ee9923, 0xa18c3407, 0x582edecf},
                   {0x873d4160, 0xb0e68183, 0x6cade2bc, 0x77ba8f60, 0x26ec9f79,
                    0xc8b2c612, 0x79e80d5f, 0x180fdb61}}};
  Fq6Elem actual = {0};
  Fq6Inv(&actual, &left);
  EXPECT_EQ(expected, actual);
}

////////////////////////////////////////////////////////////////////////
// Fq6Neg

TEST(TinyFq6Test, Fq6NegWorks) {
  Fq6Elem expected = {{{0x8c035971, 0xaf40febd, 0x3d89f632, 0x24887d6e,
                        0x3ecb5147, 0xff6580f2, 0x0023a5bd, 0xa19bcedb},
                       {0x61afe694, 0xba8b7e8e, 0x07d2460a, 0xd758834b,
                        0xb9944e4d, 0x2b2cfe7e, 0xe1ddaf4c, 0x755ba40a}},
                      {{0xff363b04, 0xf3770d54, 0xfb887eb6, 0x97208d71,
                        0x913888b6, 0xbe3b78dc, 0x504e9cec, 0x1328c6a8},
                       {0x80a4bcc9, 0xdc414b24, 0x81e12ae2, 0xddece243,
                        0x9e717f72, 0x2eae9d50, 0xe0b74a59, 0x6c2aa832}},
                      {{0xe3749624, 0x90f1e179, 0xe064c5e0, 0xa23c1333,
                        0x80199e94, 0x0df7593b, 0x5e70bcc6, 0xa7d12130},
                       {0x2795eeb3, 0x2242ac58, 0xa5ea27c6, 0x9521d69a,
                        0xc7850525, 0x7e332c4c, 0x8614e36d, 0xe7f0249e}}};
  Fq6Elem left = {{{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357,
                    0x4780716c, 0xffd94b0f, 0x5e643124},
                   {0x4d23497f, 0x189daf4d, 0x0ac5c478, 0x3583e2b0, 0x34dd5651,
                    0x1bb8f3e0, 0x1e1f4181, 0x8aa45bf5}},
                  {{0xaf9cf50f, 0xdfb22086, 0x170f8bcb, 0x75bbd889, 0x5d391be8,
                    0x88aa7982, 0xafae53e0, 0xecd73957},
                   {0x2e2e734a, 0xf6e7e2b7, 0x90b6df9f, 0x2eef83b7, 0x5000252c,
                    0x1837550e, 0x1f45a674, 0x93d557cd}},
                  {{0xcb5e99ef, 0x42374c61, 0x323344a2, 0x6aa052c7, 0x6e58060a,
                    0x38ee9923, 0xa18c3407, 0x582edecf},
                   {0x873d4160, 0xb0e68183, 0x6cade2bc, 0x77ba8f60, 0x26ec9f79,
                    0xc8b2c612, 0x79e80d5f, 0x180fdb61}}};
  Fq6Elem actual = {0};
  Fq6Neg(&actual, &left);
  EXPECT_EQ(expected, actual);
}

////////////////////////////////////////////////////////////////////////
// Fq6Clear

TEST(TinyFq6Test, Fq6ClearWorks) {
  Fq6Elem expected = {0};
  FqElem fq = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
                0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  Fq6Elem result = {fq, fq, fq, fq, fq, fq};
  Fq6Clear(&result);
  EXPECT_EQ(expected, result);
}

////////////////////////////////////////////////////////////////////////
// Fq6MulScalar

TEST(TinyFq6Test, Fq6MulScalarWorks) {
  Fq6Elem expected = {{{0x37861727, 0x52822db7, 0x8005ec64, 0xc0b0bc96,
                        0xd60e07a4, 0x65eee0a2, 0x780dbc26, 0x7b36e4cb},
                       {0x8201d4ed, 0xbf8ed473, 0xc5c09cbe, 0xba9d0095,
                        0x3d91414a, 0xa8ebb728, 0x66bc029b, 0x5b6ca52b}},
                      {{0x37861727, 0x52822db7, 0x8005ec64, 0xc0b0bc96,
                        0xd60e07a4, 0x65eee0a2, 0x780dbc26, 0x7b36e4cb},
                       {0x8201d4ed, 0xbf8ed473, 0xc5c09cbe, 0xba9d0095,
                        0x3d91414a, 0xa8ebb728, 0x66bc029b, 0x5b6ca52b}},
                      {{0}, {0}}};
  Fq6Elem left = {{{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357,
                    0x4780716c, 0xffd94b0f, 0x5e643124},
                   {0x4d23497f, 0x189daf4d, 0x0ac5c478, 0x3583e2b0, 0x34dd5651,
                    0x1bb8f3e0, 0x1e1f4181, 0x8aa45bf5}},
                  {{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357,
                    0x4780716c, 0xffd94b0f, 0x5e643124},
                   {0x4d23497f, 0x189daf4d, 0x0ac5c478, 0x3583e2b0, 0x34dd5651,
                    0x1bb8f3e0, 0x1e1f4181, 0x8aa45bf5}},
                  {{0}, {0}}};
  Fq2Elem scalar = {{0x848cdb73, 0x6399829e, 0xcaa20cc0, 0x1b02bff6, 0x2b477bd2,
                     0xf9d48534, 0xff7929a0, 0xd4745161},
                    {0xe323d956, 0xf8a05a85, 0xe02d5e1e, 0xfd533966, 0xe7d31209,
                     0xc7786143, 0x91b441f6, 0x7409d67d}};
  Fq6Elem actual = {0};
  Fq6MulScalar(&actual, &left, &scalar);
  EXPECT_EQ(expected, actual);
}

////////////////////////////////////////////////////////////////////////
// Fq6MulV

TEST(TinyFq6Test, Fq6MulVWorks) {
  const Fq6Elem a = {{{0x747a9232, 0xb91bf0cd, 0x1c5aa24e, 0x1c3c7860,
                       0x40b4d7ca, 0x26094519, 0xe29e50db, 0x67da0b88},
                      {0x1e540333, 0xc03c243c, 0x55d91627, 0xfcc36017,
                       0x5876fc39, 0xf0590ba2, 0x47ce59ac, 0x9cc0760e}},
                     {{0xaf2c16be, 0x4036bf3d, 0xd8ecc7eb, 0x4fdd0128,
                       0x4faf6ca1, 0xc55bc411, 0xa26b3ac5, 0x1338fc1c},
                      {0x0936baf1, 0xc2254b98, 0x84993186, 0xa7a46e79,
                       0xf2ed3380, 0xa1a5dad1, 0x2046667a, 0x348362a5}},
                     {{0xbca2b7aa, 0xc0e43294, 0x6199e561, 0xefdb7a39,
                       0xd57bcbba, 0x03154f2a, 0xdf9e1797, 0xf52d29c1},
                      {0x77cb909b, 0x906d8657, 0xfea2ffb3, 0x7810e964,
                       0x022e47c1, 0x862bdbe6, 0xe4f5d59b, 0xa677247d}}};
  const Fq6Elem expected = {{{0x52a6aea6, 0x1e31b0f6, 0xb1f8c08d, 0x5ac9a512,
                              0xba57ab15, 0x3918d010, 0xda4968c5, 0x43e32f05},
                             {0x4e9378ba, 0x3b6ce38c, 0x39afcfc3, 0xc644810d,
                              0xfcf511ff, 0x81a12238, 0xa98fe133, 0x421b72bd}},
                            {{0x747a9232, 0xb91bf0cd, 0x1c5aa24e, 0x1c3c7860,
                              0x40b4d7ca, 0x26094519, 0xe29e50db, 0x67da0b88},
                             {0x1e540333, 0xc03c243c, 0x55d91627, 0xfcc36017,
                              0x5876fc39, 0xf0590ba2, 0x47ce59ac, 0x9cc0760e}},
                            {{0xaf2c16be, 0x4036bf3d, 0xd8ecc7eb, 0x4fdd0128,
                              0x4faf6ca1, 0xc55bc411, 0xa26b3ac5, 0x1338fc1c},
                             {0x0936baf1, 0xc2254b98, 0x84993186, 0xa7a46e79,
                              0xf2ed3380, 0xa1a5dad1, 0x2046667a, 0x348362a5}}};

  Fq6Elem res;
  Fq6MulV(&res, &a);
  EXPECT_EQ(expected, res);
}

////////////////////////////////////////////////////////////////////////
// Fq6Eq

TEST(TinyFq6Test, Fq6EqPasses) {
  FqElem fq_a = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
                  0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  FqElem fq_b = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
                  0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  Fq6Elem a = {fq_a, fq_a, fq_a, fq_a, fq_a, fq_a};
  Fq6Elem b = {fq_b, fq_b, fq_b, fq_b, fq_b, fq_b};
  EXPECT_TRUE(Fq6Eq(&a, &b));
}

TEST(TinyFq6Test, Fq6EqFails) {
  FqElem fq_a = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
                  0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  FqElem fq_b = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
                  0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  Fq6Elem a = {fq_a, fq_a, fq_a, fq_a, fq_a, fq_a};
  Fq6Elem b = {fq_b, fq_b, fq_b, fq_b, fq_b, fq_b};
  --b.y2.x1.limbs.word[5];
  EXPECT_FALSE(Fq6Eq(&a, &b));
}

////////////////////////////////////////////////////////////////////////
// Fq6IsZero

TEST(TinyFq6Test, Fq6IsZeroPasses) {
  Fq6Elem zero = {0};
  EXPECT_TRUE(Fq6IsZero(&zero));
}

TEST(TinyFq6Test, Fq6IsZeroFails) {
  Fq6Elem non_zero = {0};
  ++non_zero.y2.x1.limbs.word[4];
  EXPECT_FALSE(Fq6IsZero(&non_zero));
}

////////////////////////////////////////////////////////////////////////
// Fq6Square

TEST(TinyFq6Test, Fq6SquareWorks) {
  Fq6Elem expected = {{{0xd33e098c, 0xf56a0ddf, 0x1a07b1f1, 0x5c9f80e4,
                        0x0f4cab4b, 0x18c856b8, 0x4c614650, 0xdd44c9f1},
                       {0x5210a238, 0x934c3474, 0x8d6f7915, 0x7bfe3252,
                        0x907c0c7d, 0x6cb28259, 0xeab6a4de, 0xb37400f3}},
                      {{0xed8e6c82, 0xfcb0be62, 0x686dcd3b, 0xfdfbb0b6,
                        0x30744d0a, 0x408cd48e, 0x1cd63084, 0xa246ad21},
                       {0xc823a4b3, 0x4add0b26, 0x68c1bd68, 0x67140391,
                        0xb0fe9dd7, 0xbae08890, 0xac5aac76, 0xb2547efa}},
                      {{0xb1eae872, 0xc22e7b27, 0x3b972d7a, 0x894e0e3a,
                        0xbc67f9b5, 0xc8a56674, 0x82ba5f56, 0xfbeb546b},
                       {0x6e6c8c17, 0x634cdd65, 0xc3678b6f, 0x08e820dc,
                        0x166dbab9, 0xb0c34885, 0x0b952acd, 0xffc1173e}}};
  Fq6Elem left = {{{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357,
                    0x4780716c, 0xffd94b0f, 0x5e643124},
                   {0x4d23497f, 0x189daf4d, 0x0ac5c478, 0x3583e2b0, 0x34dd5651,
                    0x1bb8f3e0, 0x1e1f4181, 0x8aa45bf5}},
                  {{0xaf9cf50f, 0xdfb22086, 0x170f8bcb, 0x75bbd889, 0x5d391be8,
                    0x88aa7982, 0xafae53e0, 0xecd73957},
                   {0x2e2e734a, 0xf6e7e2b7, 0x90b6df9f, 0x2eef83b7, 0x5000252c,
                    0x1837550e, 0x1f45a674, 0x93d557cd}},
                  {{0xcb5e99ef, 0x42374c61, 0x323344a2, 0x6aa052c7, 0x6e58060a,
                    0x38ee9923, 0xa18c3407, 0x582edecf},
                   {0x873d4160, 0xb0e68183, 0x6cade2bc, 0x77ba8f60, 0x26ec9f79,
                    0xc8b2c612, 0x79e80d5f, 0x180fdb61}}};
  Fq6Elem actual = {0};
  Fq6Square(&actual, &left);
  EXPECT_EQ(expected, actual);
}

////////////////////////////////////////////////////////////////////////
// Fq6Cp

TEST(TinyFq6Test, Fq6CpWorks) {
  FqElem fq = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
                0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  Fq6Elem a = {fq, fq, fq, fq, fq, fq};
  Fq6Elem result = {0};
  Fq6Cp(&result, &a);
  EXPECT_EQ(a, result);
}

////////////////////////////////////////////////////////////////////////
// Fq6CondSet

TEST(TinyFq6Test, Fq6CondSetWorksForTrue) {
  Fq6Elem a = {{{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357,
                 0x4780716c, 0xffd94b0f, 0x5e64364},
                {0x4d23497f, 0x189daf4d, 0x0ac5c478, 0x3583e2b0, 0x34dd5651,
                 0x1bb8f3e0, 0x1e1f4181, 0x8aa45bf5}},
               {{0xaf9cf50f, 0xdfb22086, 0x170f8bcb, 0x75bbd889, 0x5d391be8,
                 0x88aa7982, 0xafae53e0, 0xecd73957},
                {0x2e2e734a, 0xf6e7e2b7, 0x90b6df9f, 0x2eef83b7, 0x5000252c,
                 0x1837550e, 0x1f45a674, 0x93d557cd}},
               {{0xcb5e99ef, 0x42374c61, 0x323344a2, 0x6aa052c7, 0x6e58060a,
                 0x38ee9923, 0xa18c3407, 0x582edecf},
                {0x873d4160, 0xb0e68183, 0x6cade2bc, 0x77ba8f60, 0x26ec9f79,
                 0xc8b2c66, 0x79e80d5f, 0x180fdb61}}};
  Fq6Elem b = {{{1, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 0, 0, 0, 0, 0, 0}},
               {{0, 0, 1, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 0, 0, 0, 0}},
               {{0, 0, 0, 0, 1, 0, 0, 0}, {0, 0, 0, 0, 0, 1, 0, 0}}};

  Fq6Elem result = {0};
  Fq6CondSet(&result, &a, &b, true);
  EXPECT_EQ(a, result);
}

TEST(TinyFq6Test, Fq6CondSetWorksForFalse) {
  Fq6Elem a = {{{0x22cfd6a2, 0x23e82f1e, 0xd50e1450, 0xe853e88c, 0xafa65357,
                 0x4780716c, 0xffd94b0f, 0x5e64364},
                {0x4d23497f, 0x189daf4d, 0x0ac5c478, 0x3583e2b0, 0x34dd5651,
                 0x1bb8f3e0, 0x1e1f4181, 0x8aa45bf5}},
               {{0xaf9cf50f, 0xdfb22086, 0x170f8bcb, 0x75bbd889, 0x5d391be8,
                 0x88aa7982, 0xafae53e0, 0xecd73957},
                {0x2e2e734a, 0xf6e7e2b7, 0x90b6df9f, 0x2eef83b7, 0x5000252c,
                 0x1837550e, 0x1f45a674, 0x93d557cd}},
               {{0xcb5e99ef, 0x42374c61, 0x323344a2, 0x6aa052c7, 0x6e58060a,
                 0x38ee9923, 0xa18c3407, 0x582edecf},
                {0x873d4160, 0xb0e68183, 0x6cade2bc, 0x77ba8f60, 0x26ec9f79,
                 0xc8b2c66, 0x79e80d5f, 0x180fdb61}}};
  Fq6Elem b = {{{1, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 0, 0, 0, 0, 0, 0}},
               {{0, 0, 1, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 0, 0, 0, 0}},
               {{0, 0, 0, 0, 1, 0, 0, 0}, {0, 0, 0, 0, 0, 1, 0, 0}}};

  Fq6Elem result = {0};
  Fq6CondSet(&result, &a, &b, false);
  EXPECT_EQ(b, result);
}

////////////////////////////////////////////////////////////////////////
// Fq6Set

TEST(TinyFq6Test, Fq6SetWorks) {
  uint32_t small = 0xffffffff;
  Fq6Elem expected = {0};
  expected.y0.x0.limbs.word[0] = small;
  FqElem fq = {{0xAED33012, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F,
                0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF}};
  Fq6Elem result = {fq, fq, fq, fq, fq, fq};
  Fq6Set(&result, small);
  EXPECT_EQ(expected, result);
}

}  // namespace
