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
/// Definition of math types in tiny Intel(R) EPID.
/*! \file */

#ifndef EPID_MEMBER_TINY_MATH_MATHTYPES_H_
#define EPID_MEMBER_TINY_MATH_MATHTYPES_H_

/// number of 32bit words in a very large integer
#define NUM_ECC_DIGITS 8

#include <stdint.h>

/// Large integer.
/*!
VeryLargeInt* is always expected to point to a buffer
with NUM_ECC_DIGITS uint32_t sized words.
*/
typedef struct VeryLargeInt {
  uint32_t word[NUM_ECC_DIGITS];  ///< Large integer data
} VeryLargeInt;

/// Used for multiplication
typedef struct VeryLargeIntProduct {
  uint32_t word[2 * NUM_ECC_DIGITS];  ///< Large integer data
} VeryLargeIntProduct;

/// Element of Fp.
typedef struct FpElem {
  VeryLargeInt limbs;  ///< An integer in [0, p-1]
} FpElem;

/// Element of Fq.
typedef struct FqElem {
  VeryLargeInt limbs;  ///< An integer in [0, q-1]
} FqElem;

/// Element of Fq2.
typedef struct Fq2Elem {
  FqElem x0;  ///< A coefficent in Fq
  FqElem x1;  ///< A coefficent in Fq
} Fq2Elem;

/// Point in EFq.
typedef struct EccPointFq {
  FqElem x;  ///< x coordinate
  FqElem y;  ///< y coordinate
} EccPointFq;

/// Point in EFq2.
typedef struct EccPointFq2 {
  Fq2Elem x;  ///< x coordinate
  Fq2Elem y;  ///< y coordinate
} EccPointFq2;

/// Element of Fq6.
typedef struct Fq6Elem {
  Fq2Elem y0;  ///< A coefficent in Fq2
  Fq2Elem y1;  ///< A coefficent in Fq2
  Fq2Elem y2;  ///< A coefficent in Fq2
} Fq6Elem;

/// Element of Fq12.
typedef struct Fq12Elem {
  Fq6Elem z0;  ///< A coefficent in Fq6
  Fq6Elem z1;  ///< A coefficent in Fq6
} Fq12Elem;

/// Element of EFq in Jacobi format.
typedef struct EccPointJacobiFq {
  FqElem X;  ///< x coordinate
  FqElem Y;  ///< y coordinate
  FqElem Z;  ///< z coordinate
} EccPointJacobiFq;

/// Element of EFq2 in Jacobi format.
typedef struct EccPointJacobiFq2 {
  Fq2Elem X;  ///< x coordinate
  Fq2Elem Y;  ///< y coordinate
  Fq2Elem Z;  ///< z coordinate
} EccPointJacobiFq2;

/// A scratch buffer for stateful pairing calls.
typedef struct PairingState {
  Fq2Elem g[3][5];  ///< pairing scratch data
} PairingState;

#endif  // EPID_MEMBER_TINY_MATH_MATHTYPES_H_
