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
/// Definition of Large Integer math
/*! \file */

#ifndef EPID_MEMBER_TINY_MATH_VLI_H_
#define EPID_MEMBER_TINY_MATH_VLI_H_

#include <stdint.h>
#include "epid/common/bitsupplier.h"
/// \cond
typedef struct VeryLargeInt VeryLargeInt;
typedef struct VeryLargeIntProduct VeryLargeIntProduct;
/// \endcond

/// Add two large integers.
/*!
\param[out] result target.
\param[in] left The first operand to be added.
\param[in] right The second operand to be added.

\returns the carry portion of the addition.
*/
uint32_t VliAdd(VeryLargeInt* result, VeryLargeInt const* left,
                VeryLargeInt const* right);

/// Multiply two large integers.
/*!
\param[out] result of multiplying left and right.
\param[in] left The first operand to be multiplied.
\param[in] right The second operand to be multiplied.
*/
void VliMul(VeryLargeIntProduct* result, VeryLargeInt const* left,
            VeryLargeInt const* right);

/// Right shift a large integers.
/*!
\param[out] result target.
\param[in] in The value to be shifted.
\param[in] shift The number of bits to shift.
*/
void VliRShift(VeryLargeInt* result, VeryLargeInt const* in, uint32_t shift);

/// Subtract two large integers.
/*!
\param[out] result target.
\param[in] left The operand to be subtracted from.
\param[in] right The operand to subtract.
\returns 1 on success, 0 on failure
*/
uint32_t VliSub(VeryLargeInt* result, VeryLargeInt const* left,
                VeryLargeInt const* right);

/// Set a large integer's value.
/*!
\param[out] result target.
\param[in] in value to set.
*/
void VliSet(VeryLargeInt* result, VeryLargeInt const* in);

/// Clear a large integer's value.
/*!
\param[out] result value to clear.
*/
void VliClear(VeryLargeInt* result);

/// Test if a large integer is zero.
/*!
\param[in] in the value to test.
\returns A value different from zero (i.e., true) if indeed
         the value is zero. Zero (i.e., false) otherwise.
*/
int VliIsZero(VeryLargeInt const* in);

/// Conditionally Set a large inter's value to one of two values.
/*!
\param[out] result target.
\param[in] true_val value to set if condition is true.
\param[in] false_val value to set if condition is false.
\param[in] truth_val value of condition.
*/
void VliCondSet(VeryLargeInt* result, VeryLargeInt const* true_val,
                VeryLargeInt const* false_val, int truth_val);

/// Test the value of a bit in a large integer.
/*!
\param[in] in the value to test.
\param[in] bit the bit index.

\returns value of the bit (1 or 0).
*/
uint32_t VliTestBit(VeryLargeInt const* in, uint32_t bit);

/// Generate a random large integer.
/*!
\param[in] result the random value.
\param[in] rnd_func Random number generator.
\param[in] rnd_param Pass through context data for rnd_func.
\returns A value different from zero (i.e., true) if on success.
         Zero (i.e., false) otherwise.
*/
int VliRand(VeryLargeInt* result, BitSupplier rnd_func, void* rnd_param);

/// compare two large integers.
/*!
\param[in] left the left hand value.
\param[in] right the right hand value.

\returns the sign of left - right
*/
int VliCmp(VeryLargeInt const* left, VeryLargeInt const* right);

/// Add two large integers modulo a value.
/*!
\param[out] result target.
\param[in] left The first operand to be added.
\param[in] right The second operand to be added.
\param[in] mod The modulo.
*/
void VliModAdd(VeryLargeInt* result, VeryLargeInt const* left,
               VeryLargeInt const* right, VeryLargeInt const* mod);

/// Subtract two large integers modulo a value.
/*!
\param[out] result target.
\param[in] left The operand to be subtracted from.
\param[in] right The operand to subtract.
\param[in] mod The modulo.
*/
void VliModSub(VeryLargeInt* result, VeryLargeInt const* left,
               VeryLargeInt const* right, VeryLargeInt const* mod);

/// Multiply two large integers modulo a value.
/*!
\param[out] result target.
\param[in] left The first operand to be multiplied.
\param[in] right The second operand to be multiplied.
\param[in] mod The modulo.
*/
void VliModMul(VeryLargeInt* result, VeryLargeInt const* left,
               VeryLargeInt const* right, VeryLargeInt const* mod);

/// Exponentiate a large integer modulo a value.
/*!
\param[out] result target.
\param[in] base the base.
\param[in] exp the exponent.
\param[in] mod The modulo.
*/
void VliModExp(VeryLargeInt* result, VeryLargeInt const* base,
               VeryLargeInt const* exp, VeryLargeInt const* mod);

/// Invert  a large integer modulo a value.
/*!
\param[out] result target.
\param[in] input the value to invert.
\param[in] mod The modulo.
*/
void VliModInv(VeryLargeInt* result, VeryLargeInt const* input,
               VeryLargeInt const* mod);

/// Square a large integer modulo a value.
/*!
\param[out] result target.
\param[in] input the base.
\param[in] mod The modulo.
*/
void VliModSquare(VeryLargeInt* result, VeryLargeInt const* input,
                  VeryLargeInt const* mod);

/// Reduce a value to a modulo.
/*!
\param[out] result target.
\param[in] input the base.
\param[in] mod The modulo.

\warning This function makes significant assumptions about
the range of values input
*/
void VliModBarrett(VeryLargeInt* result, VeryLargeIntProduct const* input,
                   VeryLargeInt const* mod);

#endif  // EPID_MEMBER_TINY_MATH_VLI_H_
