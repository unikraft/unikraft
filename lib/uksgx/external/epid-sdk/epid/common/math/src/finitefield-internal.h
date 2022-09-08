/*############################################################################
  # Copyright 2016-2017 Intel Corporation
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

/*!
 * \file
 * \brief Finite field private interface.
 */

#ifndef EPID_COMMON_MATH_SRC_FINITEFIELD_INTERNAL_H_
#define EPID_COMMON_MATH_SRC_FINITEFIELD_INTERNAL_H_

#include "epid/common/math/bignum.h"
#include "epid/common/math/src/bignum-internal.h"
#include "ext/ipp/include/ippcp.h"

/// Finite Field
struct FiniteField {
  /// Internal implementation of finite field
  IppsGFpState* ipp_ff;
  /// Previous finitefield
  struct FiniteField* ground_ff;
  /// Degree of basic field
  int basic_degree;
  /// Degree of current field
  int ground_degree;
  /// Size of element in BNU units
  int element_len;
  /// Minimum number of bytes needed to serialize an element
  size_t element_strlen_required;
  /*!
  Galois field prime or free standing coefficient of
  irreducible polynomial of finite field extension.
  */
  BigNum* modulus_0;
};

/// Finite Field Element
struct FfElement {
  /// Internal implementation of finite field element
  IppsGFpElement* ipp_ff_elem;
  /// Element size of Finite Field element
  int element_len;
  /// Degree of Finite Field element
  int degree;
};

EpidStatus SetFfElementOctString(ConstOctStr ff_elem_str, int strlen,
                                 struct FfElement* ff_elem,
                                 struct FiniteField* ff);

#endif  // EPID_COMMON_MATH_SRC_FINITEFIELD_INTERNAL_H_
