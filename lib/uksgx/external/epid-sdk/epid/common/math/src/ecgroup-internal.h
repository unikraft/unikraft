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
 * \brief Elliptic curve group private interface.
 */

#ifndef EPID_COMMON_MATH_SRC_ECGROUP_INTERNAL_H_
#define EPID_COMMON_MATH_SRC_ECGROUP_INTERNAL_H_

#include "epid/common/math/src/finitefield-internal.h"
#include "ext/ipp/include/ippcp.h"

/// Elpitic Curve Group
struct EcGroup {
  /// Internal implementation of elliptic curve group
  IppsGFpECState* ipp_ec;
  /// Scratch buffer for operations over elliptic curve group
  OctStr scratch_buffer;
  /// Information about finite field of elliptic curve group
  struct FiniteField* ff;
};

/// Elpitic Curve Point
struct EcPoint {
  /// Internal implementation of elliptic curve point
  IppsGFpECPoint* ipp_ec_pt;
  /// length of the finite field element of elliptic curve group
  int element_len;
};
#endif  // EPID_COMMON_MATH_SRC_ECGROUP_INTERNAL_H_
