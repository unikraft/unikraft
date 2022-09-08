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
/// Interface to a SHA-512 implementation.
/*! \file */

#ifndef EPID_MEMBER_TINY_MATH_SHA512_H_
#define EPID_MEMBER_TINY_MATH_SHA512_H_

#include <stddef.h>
#include <stdint.h>

/// block size
#define SHA512_BLOCK_SIZE (128)
/// digest size
#define SHA512_DIGEST_SIZE (64)
/// number of words in SHA state
#define SHA512_DIGEST_WORDS (8)

/// The SHA state
/// \cond
typedef struct sha512_state {
  uint64_t iv[SHA512_DIGEST_WORDS];
  uint64_t bits_hashed_low;
  uint64_t bits_hashed_high;
  unsigned char leftover[SHA512_BLOCK_SIZE];
  unsigned int leftover_offset;
} sha512_state;
/// \endcond

/// Initializes the hash state
/*!

  \param[in,out] s
  The hash state to initialize.
 */
void tinysha512_init(sha512_state* s);

/// Hashes data into state using SHA-512
/*!

  \warning
  The state buffer 'leftover' is left in memory after processing. If
  your application intends to have sensitive data in this buffer,
  remember to erase it after the data has been processed

  \param[in,out] s
  The hash state. Must be non-null or behavior is undefined.

  \param[in] data
  The data to hash into s.

  \param[in] data_length
  The size of data in bytes.
 */
void tinysha512_update(sha512_state* s, void const* data, size_t data_length);

/// Computes the SHA-512 hash in the digest buffer
/*!

  \note Assumes SHA512_DIGEST_SIZE bytes are available to accept the
  digest.

  \warning
  The state buffer 'leftover' is left in memory after processing. If
  your application intends to have sensitive data in this buffer,
  remember to erase it after the data has been processed

  \param[out] digest
  The computed digest. Must be non-null or behavior is undefined.

  \param[in] s
  The hash state. Must be non-null or behavior is undefined.
 */
void tinysha512_final(unsigned char* digest, sha512_state* s);

#endif  // EPID_MEMBER_TINY_MATH_SHA512_H_
