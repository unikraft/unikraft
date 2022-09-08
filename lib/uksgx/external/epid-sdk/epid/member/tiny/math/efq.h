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
/// Definition of EFq math
/*! \file */

#ifndef EPID_MEMBER_TINY_MATH_EFQ_H_
#define EPID_MEMBER_TINY_MATH_EFQ_H_
#include <stddef.h>
#include "epid/common/bitsupplier.h"
#include "epid/common/types.h"

/// \cond
typedef struct EccPointFq EccPointFq;
typedef struct EccPointJacobiFq EccPointJacobiFq;
typedef struct FpElem FpElem;
typedef struct FqElem FqElem;
/// \endcond

/// Multiply two points in EFq.
/*!
This function is mitigated against software side-channel
attacks.

\param[out] result of multiplying left and right.
\param[in] base The first operand to be multiplied.
\param[in] exp The second operand to be multiplied.
*/
void EFqMulSSCM(EccPointJacobiFq* result, EccPointJacobiFq const* base,
                FpElem const* exp);

/// Exponentiate a point in EFq by an element of Fp.
/*!
\param[out] result target.
\param[in] base the base.
\param[in] exp the exponent.
\returns A value different from zero (i.e., true) if on success.
         Zero (i.e., false) otherwise.

\returns 1 on success, 0 on failure
*/
int EFqAffineExp(EccPointFq* result, EccPointFq const* base, FpElem const* exp);

/// Sum the results of exponentiating two points in EFq by elements of Fp.
/*!
\param[out] result target.
\param[in] base0 the first base.
\param[in] exp0 the first exponent.
\param[in] base1 the second base.
\param[in] exp1 the second exponent.
\returns A value different from zero (i.e., true) if on success.
         Zero (i.e., false) otherwise.
*/
int EFqAffineMultiExp(EccPointFq* result, EccPointFq const* base0,
                      FpElem const* exp0, EccPointFq const* base1,
                      FpElem const* exp1);

/// Sum the results of exponentiating two points in EFq by elements of Fp.
/*!
\param[out] result target.
\param[in] base0 the first base.
\param[in] exp0 the first exponent.
\param[in] base1 the second base.
\param[in] exp1 the second exponent.
\returns 1 on success, 0 on failure
*/
void EFqMultiExp(EccPointJacobiFq* result, EccPointJacobiFq const* base0,
                 FpElem const* exp0, EccPointJacobiFq const* base1,
                 FpElem const* exp1);

/// Add two points in EFq.
/*!
\param[out] result of adding left and right.
\param[in] left The first operand to be added.
\param[in] right The second operand to be added.
\returns A value different from zero (i.e., true) if on success.
         Zero (i.e., false) otherwise.
*/
int EFqAffineAdd(EccPointFq* result, EccPointFq const* left,
                 EccPointFq const* right);

/// Double a point in EFq.
/*!
\param[out] result target.
\param[in] in the value to double.
\returns A value different from zero (i.e., true) if on success.
         Zero (i.e., false) otherwise.
*/
int EFqAffineDbl(EccPointFq* result, EccPointFq const* in);

/// Double a point in EFq.
/*!
\param[out] result target.
\param[in] in the value to double.
*/
void EFqDbl(EccPointJacobiFq* result, EccPointJacobiFq const* in);

/// Add two points in EFq.
/*!
\param[out] result of adding left and right.
\param[in] left The first operand to be added.
\param[in] right The second operand to be added.
*/
void EFqAdd(EccPointJacobiFq* result, EccPointJacobiFq const* left,
            EccPointJacobiFq const* right);

/// Generate a random point in EFq.
/*!
\param[in] result the random value.
\param[in] rnd_func Random number generator.
\param[in] rnd_param Pass through context data for rnd_func.
\returns A value different from zero (i.e., true) if on success.
         Zero (i.e., false) otherwise.
*/
int EFqRand(EccPointFq* result, BitSupplier rnd_func, void* rnd_param);

/// Set a point's value.
/*!
\param[out] result target.
\param[in] x value to set.
\param[in] y value to set.
*/
void EFqSet(EccPointJacobiFq* result, FqElem const* x, FqElem const* y);

/// Test if a point is infinity.
/*!
\param[in] in the point to test.
\returns A value different from zero (i.e., true) indeed
         the value is infinity. Zero (i.e., false) otherwise.
*/
int EFqIsInf(EccPointJacobiFq const* in);

/// Convert a point from Affine to Jacobi representation.
/*!
\param[out] result target.
\param[in] in value to set.
*/
void EFqFromAffine(EccPointJacobiFq* result, EccPointFq const* in);

/// Convert a point from Jacobi to Affine representation.
/*!
\param[out] result target.
\param[in] in value to set.
\returns A value different from zero (i.e., true) if on success.
         Zero (i.e., false) otherwise.
*/
int EFqToAffine(EccPointFq* result, EccPointJacobiFq const* in);

/// Negate a point on EFq.
/*!
\param[out] result the negative of the element.
\param[in] in the element to negate.
*/
void EFqNeg(EccPointJacobiFq* result, EccPointJacobiFq const* in);

/// Test if two points on EFq are equal
/*!
\param[in] left The first operand to be tested.
\param[in] right The second operand to be tested.
\returns A value different from zero (i.e., true) if indeed
         the values are equal. Zero (i.e., false) otherwise.
*/
int EFqEq(EccPointJacobiFq const* left, EccPointJacobiFq const* right);

/// Hashes an arbitrary message to a point on EFq.
/*!
\param[out] result target.
\param[in] msg buffer to reinterpret.
\param[in] len length of msg in bytes.
\param[in] hashalg hash algorithm to use.
\returns A value different from zero (i.e., true) if on success.
         Zero (i.e., false) otherwise.
*/
int EFqHash(EccPointFq* result, unsigned char const* msg, size_t len,
            HashAlg hashalg);

/// Copy a point's value
/*!
\param[out] result copy target.
\param[in] in copy source.
*/
void EFqCp(EccPointFq* result, EccPointFq const* in);

/// Test if two points on EFq are equal
/*!
\param[in] left The first operand to be tested.
\param[in] right The second operand to be tested.
\returns A value different from zero (i.e., true) if indeed
         the values are equal. Zero (i.e., false) otherwise.
*/
int EFqEqAffine(EccPointFq const* left, EccPointFq const* right);

/// Conditionally Set a point's value to one of two values.
/*!
\param[out] result target.
\param[in] true_val value to set if condition is true.
\param[in] false_val value to set if condition is false.
\param[in] truth_val value of condition.
*/
void EFqCondSet(EccPointJacobiFq* result, EccPointJacobiFq const* true_val,
                EccPointJacobiFq const* false_val, int truth_val);

/// Copy a point's value
/*!
\param[out] result copy target.
\param[in] in copy source.
*/
void EFqJCp(EccPointJacobiFq* result, EccPointJacobiFq const* in);

/// Set an element's value to infinity.
/*!
\param[out] result element to set.
*/
void EFqInf(EccPointJacobiFq* result);

/// Test if a point is on EFq.
/*!
\param[in] in the point to test.
\returns A value different from zero (i.e., true) indeed
         the point is on the curve. Zero (i.e., false) otherwise.
*/
int EFqOnCurve(EccPointFq const* in);

/// Test if a point is on EFq.
/*!
\param[in] in the point to test.
\returns A value different from zero (i.e., true) indeed
         the point is on the curve. Zero (i.e., false) otherwise.
*/
int EFqJOnCurve(EccPointJacobiFq const* in);

/// Generate a random point in EFq.
/*!
\param[in] result the random value.
\param[in] rnd_func Random number generator.
\param[in] rnd_param Pass through context data for rnd_func.
\returns A value different from zero (i.e., true) if on success.
         Zero (i.e., false) otherwise.
*/
int EFqJRand(EccPointJacobiFq* result, BitSupplier rnd_func, void* rnd_param);

#endif  // EPID_MEMBER_TINY_MATH_EFQ_H_
