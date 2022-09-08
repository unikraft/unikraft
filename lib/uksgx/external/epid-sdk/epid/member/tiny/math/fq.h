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
/// Definition of Fq math
/*! \file */

#ifndef EPID_MEMBER_TINY_MATH_FQ_H_
#define EPID_MEMBER_TINY_MATH_FQ_H_

#include <stddef.h>
#include <stdint.h>
#include "epid/common/bitsupplier.h"

/// \cond
typedef struct FqElem FqElem;
typedef struct VeryLargeInt VeryLargeInt;
/// \endcond

/// Test if an element is in Fq.
/*!
\param[in] in the element to test.
\returns A value different from zero (i.e., true) indeed
         the value is in the field. Zero (i.e., false) otherwise.
*/
int FqInField(FqElem const* in);

/// Add two elements of Fq.
/*!
\param[out] result of adding left and right.
\param[in] left The first operand to be added.
\param[in] right The second operand to be added.
*/
void FqAdd(FqElem* result, FqElem const* left, FqElem const* right);

/// Subtract two elements of Fq.
/*!
\param[out] result of subtracting left from right.
\param[in] left The operand to be subtracted from.
\param[in] right The operand to subtract.
*/
void FqSub(FqElem* result, FqElem const* left, FqElem const* right);

/// Multiply two elements of Fq.
/*!
\param[out] result of multiplying left and right.
\param[in] left The first operand to be multiplied.
\param[in] right The second operand to be multiplied.
*/
void FqMul(FqElem* result, FqElem const* left, FqElem const* right);

/// Exponentiate an element of Fq by a large integer.
/*!
\param[out] result target.
\param[in] base the base.
\param[in] exp the exponent.
*/
void FqExp(FqElem* result, FqElem const* base, VeryLargeInt const* exp);

/// Copy an element's value
/*!
\param[out] result copy target.
\param[in] in copy source.
*/
void FqCp(FqElem* result, FqElem const* in);

/// Test if an element is zero.
/*!
\param[in] value the element to test.
\returns A value different from zero (i.e., true) if indeed
         the value is zero. Zero (i.e., false) otherwise.
*/
int FqIsZero(FqElem const* value);

/// Invert an element of Fq.
/*!
\param[out] result the inverse of the element.
\param[in] in the element to invert.
*/
void FqInv(FqElem* result, FqElem const* in);

/// Negate an element of Fq.
/*!
This function was formerly called as FqConst.

\param[out] result the negative of the element.
\param[in] in the element to negate.
*/
void FqNeg(FqElem* result, FqElem const* in);

/// Square an element of Fq.
/*!
\param[out] result the square of the element.
\param[in] in the element to square.
*/
void FqSquare(FqElem* result, FqElem const* in);

/// Clear an element's value.
/*!
\param[out] result element to clear.
*/
void FqClear(FqElem* result);

/// Set an element's value.
/*!
\param[out] result target.
\param[in] in value to set.
*/
void FqSet(FqElem* result, uint32_t in);

/// Test if two elements in Fq are equal
/*!
\param[in] left The first operand to be tested.
\param[in] right The second operand to be tested.
\returns A value different from zero (i.e., true) if indeed
         the values are equal. Zero (i.e., false) otherwise.
*/
int FqEq(FqElem const* left, FqElem const* right);

/// Conditionally Set an element's value to one of two values.
/*!
\param[out] result target.
\param[in] true_val value to set if condition is true.
\param[in] false_val value to set if condition is false.
\param[in] truth_val value of condition.
*/
void FqCondSet(FqElem* result, FqElem const* true_val, FqElem const* false_val,
               int truth_val);

/// Compute the Square root of an element of Fq.
/*!
\param[out] result the square root of the element.
\param[in] in the element to find the square root of.
\returns A value different from zero (i.e., true) if the square root
         exists.  Zero (i.e., false) otherwise.
*/
int FqSqrt(FqElem* result, FqElem const* in);

/// Generate a random element of Fq.
/*!
\param[in] result the random value.
\param[in] rnd_func Random number generator.
\param[in] rnd_param Pass through context data for rnd_func.
\returns A value different from zero (i.e., true) if on success.
         Zero (i.e., false) otherwise.
*/
int FqRand(FqElem* result, BitSupplier rnd_func, void* rnd_param);

/// Reinterpret a buffer as an element of Fq
/*!
\param[out] result target.
\param[in] hash buffer to reinterpret.
\param[in] len length of hash in bytes.
*/
void FqFromHash(FqElem* result, unsigned char const* hash, size_t len);

#endif  // EPID_MEMBER_TINY_MATH_FQ_H_
