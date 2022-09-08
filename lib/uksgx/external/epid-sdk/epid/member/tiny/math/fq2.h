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
/// Definition of Fq2 math
/*! \file */

#ifndef EPID_MEMBER_TINY_MATH_FQ2_H_
#define EPID_MEMBER_TINY_MATH_FQ2_H_

#include <stdint.h>

/// \cond
typedef struct Fq2Elem Fq2Elem;
typedef struct FqElem FqElem;
typedef struct VeryLargeInt VeryLargeInt;
/// \endcond

/// Copy an element's value
/*!
\param[out] result copy target.
\param[in] in copy source.
*/
void Fq2Cp(Fq2Elem* result, Fq2Elem const* in);

/// Set an element's value.
/*!
\param[out] result target.
\param[in] in value to set.
*/
void Fq2Set(Fq2Elem* result, uint32_t in);

/// Clear an element's value.
/*!
\param[out] result element to clear.
*/
void Fq2Clear(Fq2Elem* result);

/// Add two elements of Fq2.
/*!
\param[out] result of adding left and right.
\param[in] left The first operand to be added.
\param[in] right The second operand to be added.
*/
void Fq2Add(Fq2Elem* result, Fq2Elem const* left, Fq2Elem const* right);

/// Exponentiate an element of Fq2 by a large integer.
/*!
\param[out] result target.
\param[in] base the base.
\param[in] exp the exponent.
*/
void Fq2Exp(Fq2Elem* result, Fq2Elem const* base, VeryLargeInt const* exp);

/// Subtract two elements of Fq2.
/*!
\param[out] result of subtracting left from right.
\param[in] left The operand to be subtracted from.
\param[in] right The operand to subtract.
*/
void Fq2Sub(Fq2Elem* result, Fq2Elem const* left, Fq2Elem const* right);

/// Multiply two elements of Fq2.
/*!
\param[out] result of multiplying left and right.
\param[in] left The first operand to be multiplied.
\param[in] right The second operand to be multiplied.
*/
void Fq2Mul(Fq2Elem* result, Fq2Elem const* left, Fq2Elem const* right);

/// Invert an element of Fq2.
/*!
\param[out] result the inverse of the element.
\param[in] in the element to invert.
*/
void Fq2Inv(Fq2Elem* result, Fq2Elem const* in);

/// Negate an element of Fq2.
/*!
\param[out] result the negative of the element.
\param[in] in the element to negate.
*/
void Fq2Neg(Fq2Elem* result, Fq2Elem const* in);

/// Calculate the conjugate of an element of Fq2.
/*!
\param[out] result the conjugate of the element.
\param[in] in the element.
*/
void Fq2Conj(Fq2Elem* result, Fq2Elem const* in);

/// Square an element of Fq2.
/*!
\param[out] result the square of the element.
\param[in] in the element to square.
*/
void Fq2Square(Fq2Elem* result, Fq2Elem const* in);

/// Multiply an element of Fq2 by and element of Fq.
/*!
\param[out] result of multiplying left and right.
\param[in] left The first operand to be multiplied.
\param[in] right The second operand to be multiplied.
*/
void Fq2MulScalar(Fq2Elem* result, Fq2Elem const* left, FqElem const* right);

/// Conditionally Set an element's value to one of two values.
/*!
\param[out] result target.
\param[in] true_val value to set if condition is true.
\param[in] false_val value to set if condition is false.
\param[in] truth_val value of condition.
*/
void Fq2CondSet(Fq2Elem* result, Fq2Elem const* true_val,
                Fq2Elem const* false_val, int truth_val);

/// Test if two elements in Fq2 are equal
/*!
\param[in] left The first operand to be tested.
\param[in] right The second operand to be tested.
\returns A value different from zero (i.e., true) if indeed
         the values are equal. Zero (i.e., false) otherwise.
*/
int Fq2Eq(Fq2Elem const* left, Fq2Elem const* right);

/// Multiply an element of Fq2 by xi.
/*!
This function was formerly called as Fq2Const.

\param[out] result of multiplying in by xi.
\param[in] in The first operand to be multiplied.

*/
void Fq2MulXi(Fq2Elem* result, Fq2Elem const* in);

/// Test if an element is zero.
/*!
\param[in] value the element to test.
\returns A value different from zero (i.e., true) if indeed
         the value is zero. Zero (i.e., false) otherwise.
*/
int Fq2IsZero(Fq2Elem const* value);

#endif  // EPID_MEMBER_TINY_MATH_FQ2_H_
