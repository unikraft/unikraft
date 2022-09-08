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
/// Definition of Fq6 math
/*! \file */

#ifndef EPID_MEMBER_TINY_MATH_FQ6_H_
#define EPID_MEMBER_TINY_MATH_FQ6_H_
#include <stdint.h>

/// \cond
typedef struct Fq2Elem Fq2Elem;
typedef struct Fq6Elem Fq6Elem;
/// \endcond

/// Add two elements of Fq6.
/*!
\param[out] result of adding left and right.
\param[in] left The first operand to be added.
\param[in] right The second operand to be added.
*/
void Fq6Add(Fq6Elem* result, Fq6Elem const* left, Fq6Elem const* right);

/// Subtract two elements of Fq6.
/*!
\param[out] result of subtracting left from right.
\param[in] left The operand to be subtracted from.
\param[in] right The operand to subtract.
*/
void Fq6Sub(Fq6Elem* result, Fq6Elem const* left, Fq6Elem const* right);

/// Multiply two elements of Fq6.
/*!
\param[out] result of multiplying left and right.
\param[in] left The first operand to be multiplied.
\param[in] right The second operand to be multiplied.
*/
void Fq6Mul(Fq6Elem* result, Fq6Elem const* left, Fq6Elem const* right);

/// Invert an element of Fq6.
/*!
\param[out] result the inverse of the element.
\param[in] in the element to invert.
*/
void Fq6Inv(Fq6Elem* result, Fq6Elem const* in);

/// Negate an element of Fq6.
/*!
\param[out] result the negative of the element.
\param[in] in the element to negate.
*/
void Fq6Neg(Fq6Elem* result, Fq6Elem const* in);

/// Clear an element's value.
/*!
\param[out] result element to clear.
*/
void Fq6Clear(Fq6Elem* result);

/// Multiply an element of Fq6 by and element of Fq2.
/*!
\param[out] result of multiplying left and right.
\param[in] in The first operand to be multiplied.
\param[in] scalar The second operand to be multiplied.
*/
void Fq6MulScalar(Fq6Elem* result, Fq6Elem const* in, Fq2Elem const* scalar);

/// Multiply an element of Fq6 by V.
/*!
This function was formerly called as Fq2Const.

\param[out] result of multiplying in and V.
\param[in] in The first operand to be multiplied.
*/
void Fq6MulV(Fq6Elem* result, Fq6Elem const* in);

/// Test if two elements in Fq6 are equal
/*!
\param[in] left The first operand to be tested.
\param[in] right The second operand to be tested.
\returns A value different from zero (i.e., true) if indeed
         the values are equal. Zero (i.e., false) otherwise.
*/
int Fq6Eq(Fq6Elem const* left, Fq6Elem const* right);

/// Test if an element is zero.
/*!
\param[in] in the element to test.
\returns A value different from zero (i.e., true) if indeed
         the value is zero. Zero (i.e., false) otherwise.
*/
int Fq6IsZero(Fq6Elem const* in);

/// Square an element of Fq6.
/*!
\param[out] result the square of the element.
\param[in] in the element to square.
*/
void Fq6Square(Fq6Elem* result, Fq6Elem const* in);

/// Copy an element's value
/*!
\param[out] result copy target.
\param[in] in copy source.
*/
void Fq6Cp(Fq6Elem* result, Fq6Elem const* in);

/// Conditionally Set an element's value to one of two values.
/*!
\param[out] result target.
\param[in] true_val value to set if condition is true.
\param[in] false_val value to set if condition is false.
\param[in] truth_val value of condition.
*/
void Fq6CondSet(Fq6Elem* result, Fq6Elem const* true_val,
                Fq6Elem const* false_val, int truth_val);

/// Set an element's value.
/*!
\param[out] result target.
\param[in] in value to set.
*/
void Fq6Set(Fq6Elem* result, uint32_t in);

#endif  // EPID_MEMBER_TINY_MATH_FQ6_H_
