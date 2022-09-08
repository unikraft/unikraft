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
/// Definition of Fp math
/*! \file */

#ifndef EPID_MEMBER_TINY_MATH_FP_H_
#define EPID_MEMBER_TINY_MATH_FP_H_

#include <stddef.h>
#include <stdint.h>
#include "epid/common/bitsupplier.h"

/// \cond
typedef struct FpElem FpElem;
typedef struct VeryLargeInt VeryLargeInt;
/// \endcond

/// Test if an element is in Fp
/*!
\param[in] in the element to test
\returns A value different from zero (i.e., true) indeed
         the value is in the field. Zero (i.e., false) otherwise.
*/
int FpInField(FpElem const* in);

/// Add two elements of Fp
/*!
\param[out] result of adding left and right.
\param[in] left The first operand to be added.
\param[in] right The second operand to be added.
*/
void FpAdd(FpElem* result, FpElem const* left, FpElem const* right);

/// Multiply two elements of Fp.
/*!
\param[out] result of multiplying left and right.
\param[in] left The first operand to be multiplied.
\param[in] right The second operand to be multiplied.
*/
void FpMul(FpElem* result, FpElem const* left, FpElem const* right);

/// Subtract two elements of Fp.
/*!
\param[out] result of subtracting left from right.
\param[in] left The operand to be subtracted from.
\param[in] right The operand to subtract.
*/
void FpSub(FpElem* result, FpElem const* left, FpElem const* right);

/// Exponentiate an element of Fp by a large integer.
/*!
\param[out] result target.
\param[in] base the base.
\param[in] exp the exponent.
*/
void FpExp(FpElem* result, FpElem const* base, VeryLargeInt const* exp);

/// Negate an element of Fp.
/*!
\param[out] result target.
\param[in] in the value to negate.
*/
void FpNeg(FpElem* result, FpElem const* in);

/// Test if two elements in Fp are equal
/*!
\param[in] left The first operand to be tested.
\param[in] right The second operand to be tested.
\returns A value different from zero (i.e., true) if indeed
         the values are equal. Zero (i.e., false) otherwise.
*/
int FpEq(FpElem const* left, FpElem const* right);

/// Invert an element of Fp.
/*!
\param[out] result target.
\param[in] in the value to invert.
*/
void FpInv(FpElem* result, FpElem const* in);

/// Generate a random element of Fp.
/*!
\param[in] result the random value.
\param[in] rnd_func Random number generator.
\param[in] rnd_param Pass through context data for rnd_func.
\returns A value different from zero (i.e., true) if on success.
         Zero (i.e., false) otherwise.
*/
int FpRand(FpElem* result, BitSupplier rnd_func, void* rnd_param);

/// Generate a non-zero random element of Fp.
/*!
\param[in] result the random value.
\param[in] rnd_func Random number generator.
\param[in] rnd_param Pass through context data for rnd_func.
\returns A value different from zero (i.e., true) if on success.
         Zero (i.e., false) otherwise.
*/
int FpRandNonzero(FpElem* result, BitSupplier rnd_func, void* rnd_param);

/// Clear an element of Fp.
/*!
\param[out] result value to clear.
*/
void FpClear(FpElem* result);

/// Set a element of Fp's value.
/*!
\param[out] result target.
\param[in] in value to set.
*/
void FpSet(FpElem* result, uint32_t in);

/// Reinterpret a buffer as an element of Fp
/*!
\param[out] result target.
\param[in] hash buffer to reinterpret.
\param[in] len length of hash in bytes.
*/
void FpFromHash(FpElem* result, unsigned char const* hash, size_t len);

#endif  // EPID_MEMBER_TINY_MATH_FP_H_
