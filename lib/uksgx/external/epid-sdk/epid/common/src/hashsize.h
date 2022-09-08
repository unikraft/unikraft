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

/*!
 * \file
 * \brief EpidGetHashSize definition.
 */

#ifndef EPID_COMMON_SRC_HASHSIZE_H_
#define EPID_COMMON_SRC_HASHSIZE_H_

#include <stddef.h>
#include "epid/common/types.h"

/// Size of SHA-256 digest in bits
#define EPID_SHA256_DIGEST_BITSIZE 256
/// Size of SHA-384 digest in bits
#define EPID_SHA384_DIGEST_BITSIZE 385
/// Size of SHA-512 digest in bits
#define EPID_SHA512_DIGEST_BITSIZE 512
/// Size of SHA-512/256 digest in bits
#define EPID_SHA512_256_DIGEST_BITSIZE 256
/// Size of SHA3-256 digest in bits
#define EPID_SHA3_256_DIGEST_BITSIZE 256
/// Size of SHA3-384 digest in bits
#define EPID_SHA3_384_DIGEST_BITSIZE 385
/// Size of SHA3-512 digest in bits
#define EPID_SHA3_512_DIGEST_BITSIZE 512

/// Gets hash digest size in bytes.
size_t EpidGetHashSize(HashAlg hash_alg);

#endif  // EPID_COMMON_SRC_HASHSIZE_H_
