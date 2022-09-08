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
 * \brief EpidGetHashSize implementation.
 */

#include "epid/common/src/hashsize.h"
#include <limits.h>

size_t EpidGetHashSize(HashAlg hash_alg) {
  switch (hash_alg) {
    case kSha256:
      return EPID_SHA256_DIGEST_BITSIZE / CHAR_BIT;
    case kSha384:
      return EPID_SHA384_DIGEST_BITSIZE / CHAR_BIT;
    case kSha512:
      return EPID_SHA512_DIGEST_BITSIZE / CHAR_BIT;
    case kSha512_256:
      return EPID_SHA512_256_DIGEST_BITSIZE / CHAR_BIT;
    case kSha3_256:
      return EPID_SHA3_256_DIGEST_BITSIZE / CHAR_BIT;
    case kSha3_384:
      return EPID_SHA3_384_DIGEST_BITSIZE / CHAR_BIT;
    case kSha3_512:
      return EPID_SHA3_512_DIGEST_BITSIZE / CHAR_BIT;
    default:
      return 0;
  }
}
