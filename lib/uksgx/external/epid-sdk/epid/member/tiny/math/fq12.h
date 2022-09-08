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
/// Definition of Fq12 math
/*! \file */

#ifndef EPID_MEMBER_TINY_MATH_FQ12_H_
#define EPID_MEMBER_TINY_MATH_FQ12_H_

#include <stdint.h>

/// \cond
typedef struct Fq12Elem Fq12Elem;
typedef struct VeryLargeInt VeryLargeInt;
/// \endcond

/// Add two elements of Fq12.
/*!
\param[out] result of adding left and right.
\param[in] left The first operand to be added.
\param[in] right The second operand to be added.
*/
void Fq12Add(Fq12Elem* result, Fq12Elem const* left, Fq12Elem const* right);

/// Subtract two elements of Fq12.
/*!
\param[out] result of subtracting left from right.
\param[in] left The operand to be subtracted from.
\param[in] right The operand to subtract.
*/
void Fq12Sub(Fq12Elem* result, Fq12Elem const* left, Fq12Elem const* right);

/// Square an element of Fq12.
/*!
\param[out] result the square of the element.
\param[in] in the element to square.
*/
void Fq12Square(Fq12Elem* result, Fq12Elem const* in);

/// Multiply two elements of Fq12.
/*!
\param[out] result of multiplying left and right.
\param[in] left The first operand to be multiplied.
\param[in] right The second operand to be multiplied.
*/
void Fq12Mul(Fq12Elem* result, Fq12Elem const* left, Fq12Elem const* right);

/// Invert an element of Fq12.
/*!
\param[out] result the inverse of the element.
\param[in] in the element to invert.
*/
void Fq12Inv(Fq12Elem* result, Fq12Elem const* in);

/// Negate an element of Fq12.
/*!
\param[out] result the negative of the element.
\param[in] in the element to negate.
*/
void Fq12Neg(Fq12Elem* result, Fq12Elem const* in);

/// Set an element's value.
/*!
\param[out] result target.
\param[in] val value to set.
*/
void Fq12Set(Fq12Elem* result, uint32_t val);

/// Exponentiate an element of Fq12 by a large integer.
/*!
\param[out] result target.
\param[in] base the base.
\param[in] exp the exponent.
*/
void Fq12Exp(Fq12Elem* result, Fq12Elem const* base, VeryLargeInt const* exp);

/// Multiply of exponentiation of elements of Fq12 by a large integers.
/*!
\param[out] result target.
\param[in] base0 the base.
\param[in] exp0 the exponent.
\param[in] base1 the base.
\param[in] exp1 the exponent.
\param[in] base2 the base.
\param[in] exp2 the exponent.
\param[in] base3 the base.
\param[in] exp3 the exponent.
*/
void Fq12MultiExp(Fq12Elem* result, Fq12Elem const* base0,
                  VeryLargeInt const* exp0, Fq12Elem const* base1,
                  VeryLargeInt const* exp1, Fq12Elem const* base2,
                  VeryLargeInt const* exp2, Fq12Elem const* base3,
                  VeryLargeInt const* exp3);

/// Test if two elements in Fq12 are equal
/*!
\param[in] left The first operand to be tested.
\param[in] right The second operand to be tested.
\returns A value different from zero (i.e., true) if indeed
         the values are equal. Zero (i.e., false) otherwise.
*/
int Fq12Eq(Fq12Elem const* left, Fq12Elem const* right);

/// Calculate the conjugate of an element of Fq2.
/*!
\param[out] result the conjugate of the element.
\param[in] in the element.
*/
void Fq12Conj(Fq12Elem* result, Fq12Elem const* in);

/// Calculate the cyclotomic exponentiation of an element of Fq12
/// by another element of Fq12.
/*!
\param[in,out] result the base of the exponentiation. This will
               receive the result.
\param[in] in the exponent.
\param[in] t pairing parameter t
*/
void Fq12ExpCyc(Fq12Elem* result, Fq12Elem const* in, VeryLargeInt const* t);

/// Calculate the cyclotomic square of an element of fq12.
/*!
\param[in,out] result result of the cyclotomic square.
\param[in] in the base.
*/
void Fq12SqCyc(Fq12Elem* result, Fq12Elem const* in);

/// Multiply two elements of Fq12.
/*!
Requires that b[2] = b[4] = b[5] = 0.
where right = ((b[0], b[2], b[4]), (b[1], b[3], b[5]))

\param[out] result of multiplying left and right.
\param[in] left The first operand to be multiplied.
\param[in] right The second operand to be multiplied.
*/
void Fq12MulSpecial(Fq12Elem* result, Fq12Elem const* left,
                    Fq12Elem const* right);

/// Copy an element's value
/*!
\param[out] result copy target.
\param[in] in copy source.
*/
void Fq12Cp(Fq12Elem* result, Fq12Elem const* in);

/// Clear an element's value.
/*!
\param[out] result element to clear.
*/
void Fq12Clear(Fq12Elem* result);

#endif  // EPID_MEMBER_TINY_MATH_FQ12_H_
