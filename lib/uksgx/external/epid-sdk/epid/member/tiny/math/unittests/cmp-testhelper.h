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
/// Comparators for tiny math types.
/*! \file */

#ifndef EPID_MEMBER_TINY_MATH_UNITTESTS_CMP_TESTHELPER_H_
#define EPID_MEMBER_TINY_MATH_UNITTESTS_CMP_TESTHELPER_H_

typedef struct VeryLargeInt VeryLargeInt;
typedef struct VeryLargeIntProduct VeryLargeIntProduct;
typedef struct FpElem FpElem;
typedef struct FqElem FqElem;
typedef struct Fq2Elem Fq2Elem;
typedef struct Fq6Elem Fq6Elem;
typedef struct Fq12Elem Fq12Elem;
typedef struct EccPointFq EccPointFq;
typedef struct EccPointJacobiFq EccPointJacobiFq;
typedef struct EccPointFq2 EccPointFq2;
typedef struct EccPointJacobiFq2 EccPointJacobiFq2;

/// compares VeryLargeInt values
bool operator==(VeryLargeInt const& lhs, VeryLargeInt const& rhs);
bool operator==(VeryLargeIntProduct const& lhs, VeryLargeIntProduct const& rhs);

/// compares FpElem values
bool operator==(FpElem const& lhs, FpElem const& rhs);

/// compares FqElem values
bool operator==(FqElem const& lhs, FqElem const& rhs);

/// compares Fq2Elem values
bool operator==(Fq2Elem const& lhs, Fq2Elem const& rhs);

/// compares Fq6Elem values
bool operator==(Fq6Elem const& lhs, Fq6Elem const& rhs);

/// compares Fq12Elem values
bool operator==(Fq12Elem const& lhs, Fq12Elem const& rhs);

/// compares EccPointFq values
bool operator==(EccPointFq const& lhs, EccPointFq const& rhs);

/// compares EccPointJacobiFq values
bool operator==(EccPointJacobiFq const& lhs, EccPointJacobiFq const& rhs);

/// compares EccPointFq2 values
bool operator==(EccPointFq2 const& lhs, EccPointFq2 const& rhs);

/// compares EccPointJacobiFq2 values
bool operator==(EccPointJacobiFq2 const& lhs, EccPointJacobiFq2 const& rhs);

#endif  // EPID_MEMBER_TINY_MATH_UNITTESTS_CMP_TESTHELPER_H_
