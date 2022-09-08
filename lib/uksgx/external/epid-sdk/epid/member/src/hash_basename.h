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
/// Basename hashing helper API
/*! \file */
#ifndef EPID_MEMBER_SRC_HASH_BASENAME_H_
#define EPID_MEMBER_SRC_HASH_BASENAME_H_

#include <stddef.h>
#include <stdint.h>
#include "epid/common/errors.h"
#include "epid/common/types.h"  // HashAlg

/// \cond
typedef struct EcGroup EcGroup;
typedef struct G1ElemStr G1ElemStr;
/// \endcond

/// Calculates hash of basename
/*!

  \param[in] G1
  The elliptic curve group.

  \param[in] hash_alg
  The hash algorithm.

  \param[in] basename
  The basename.

  \param[in] basename_len
  The size of the basename in bytes.

  \param[out] B_str
  The resulting hashed basename.

  \param[out] iterations
  The number of hash iterations needed to find a valid hash. Can be NULL.

  \returns ::EpidStatus

*/
EpidStatus HashBaseName(EcGroup* G1, HashAlg hash_alg, void const* basename,
                        size_t basename_len, G1ElemStr* B_str,
                        uint32_t* iterations);

#endif  // EPID_MEMBER_SRC_HASH_BASENAME_H_
