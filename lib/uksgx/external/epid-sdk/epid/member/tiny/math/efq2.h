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
/// Definition of EFq2 math
/*! \file */

#ifndef EPID_MEMBER_TINY_MATH_EFQ2_H_
#define EPID_MEMBER_TINY_MATH_EFQ2_H_

/// \cond
typedef struct EccPointFq2 EccPointFq2;
typedef struct EccPointJacobiFq2 EccPointJacobiFq2;
typedef struct FpElem FpElem;
/// \endcond

/// Test if a point is infinity.
/*!
\param[in] in the point to test.
\returns A value different from zero (i.e., true) indeed
         the value is infinity. Zero (i.e., false) otherwise.
*/
int EFq2IsInf(EccPointJacobiFq2 const* in);

/// Convert a point from Affine to Jacobi representation.
/*!
\param[out] result target.
\param[in] in value to set.
*/
void EFq2FromAffine(EccPointJacobiFq2* result, EccPointFq2 const* in);

/// Convert a point from Jacobi to Affine representation.
/*!
\param[out] result target.
\param[in] in value to set.
\returns 1 on success, 0 on failure
*/
int EFq2ToAffine(EccPointFq2* result, EccPointJacobiFq2 const* in);

/// Double a point in EFq2.
/*!
\param[out] result target.
\param[in] in the value to double.
*/
void EFq2Dbl(EccPointJacobiFq2* result, EccPointJacobiFq2 const* in);

/// Add two points in EFq2.
/*!
\param[out] result of adding left and right.
\param[in] left The first operand to be added.
\param[in] right The second operand to be added.
*/
void EFq2Add(EccPointJacobiFq2* result, EccPointJacobiFq2 const* left,
             EccPointJacobiFq2 const* right);

/// Negate a point on EFq2.
/*!
\param[out] result the negative of the element.
\param[in] in the element to negate.
*/
void EFq2Neg(EccPointJacobiFq2* result, EccPointJacobiFq2 const* in);

/// Multiply two points in EFq.
/*!
This function is mitigated against software side-channel
attacks.

\param[out] result of multiplying left and right.
\param[in] left The first operand to be multiplied.
\param[in] right The second operand to be multiplied.
*/
void EFq2MulSSCM(EccPointJacobiFq2* result, EccPointJacobiFq2 const* left,
                 FpElem const* right);

/// Test if two points on EFq2 are equal
/*!
\param[in] left The first operand to be tested.
\param[in] right The second operand to be tested.
\returns A value different from zero (i.e., true) if indeed
         the values are equal. Zero (i.e., false) otherwise.
*/
int EFq2Eq(EccPointJacobiFq2 const* left, EccPointJacobiFq2 const* right);

/// Test if a point is in EFq2.
/*!
\param[in] in the point to test.
\returns A value different from zero (i.e., true) indeed
         the point is on the curve. Zero (i.e., false) otherwise.
*/
int EFq2OnCurve(EccPointFq2 const* in);

#endif  // EPID_MEMBER_TINY_MATH_EFQ2_H_
