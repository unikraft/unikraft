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

/*
 *  Copyright (C) 2017 by Intel Corporation, All Rights Reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *    - Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *    - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *    - Neither the name of Intel Corporation nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @brief Interface to a SHA-256 implementation.
 *
 *  Overview:   SHA-256 is a NIST approved cryptographic hashing algorithm
 *              specified in FIPS 180. A hash algorithm maps data of arbitrary
 *              size to data of fixed length.
 *
 *  Security:   SHA-256 provides 128 bits of security against collision attacks
 *              and 256 bits of security against pre-image attacks. SHA-256 does
 *              NOT behave like a random oracle, but it can be used as one if
 *              the string being hashed is prefix-free encoded before hashing.
 *
 *  Usage:      1) call tc_sha256_init to initialize a struct
 *              tc_sha256_state_struct before hashing a new string.
 *
 *              2) call tc_sha256_update to hash the next string segment;
 *              tc_sha256_update can be called as many times as needed to hash
 *              all of the segments of a string; the order is important.
 *
 *              3) call tc_sha256_final to out put the digest from a hashing
 *              operation.
 */

#ifndef EPID_MEMBER_TINY_MATH_SHA256_H_
#define EPID_MEMBER_TINY_MATH_SHA256_H_

#include <stddef.h>
#include <stdint.h>

/// Block size
#define SHA256_BLOCK_SIZE (64)
/// Digest size
#define SHA256_DIGEST_SIZE (32)
/// Number of blocks in state
#define SHA256_STATE_BLOCKS (SHA256_DIGEST_SIZE / 4)

/// The SHA state
/// \cond
typedef struct sha256_state {
  unsigned int iv[SHA256_STATE_BLOCKS];
  uint64_t bits_hashed;
  uint8_t leftover[SHA256_BLOCK_SIZE];
  size_t leftover_offset;
} sha256_state;
/// \endcond

/**
 *  @brief SHA256 initialization procedure
 *  Initializes s
 *  @param s Sha256 state struct
 */
void tc_sha256_init(sha256_state* s);

/**
 *  @brief SHA256 update procedure
 *  Hashes data_length bytes addressed by data into state s

 *  @note Assumes s has been initialized by tc_sha256_init
 *  @warning The state buffer 'leftover' is left in memory after processing
 *           If your application intends to have sensitive data in this
 *           buffer, remind to erase it after the data has been processed
 *  @param s Sha256 state struct
 *  @param data message to hash
 *  @param datalen length of message to hash
 */
void tc_sha256_update(sha256_state* s, const uint8_t* data, size_t datalen);

/**
 *  @brief SHA256 final procedure
 *  Inserts the completed hash computation into digest
 *  @return returns 1
 *          returns 0 if:
 *                s == NULL,
 *                s->iv == NULL,
 *                digest == NULL
 *  @note Assumes: s has been initialized by tc_sha256_init
 *        digest points to at least SHA256_DIGEST_SIZE bytes
 *  @warning The state buffer 'leftover' is left in memory after processing
 *           If your application intends to have sensitive data in this
 *           buffer, remind to erase it after the data has been processed
 *  @param digest unsigned eight bit integer
 *  @param s Sha256 state struct
 */
void tc_sha256_final(uint8_t* digest, sha256_state* s);

#endif  // EPID_MEMBER_TINY_MATH_SHA256_H_
