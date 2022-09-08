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
/// Basename management implementation
/*! \file */

#include "epid/member/tiny/src/allowed_basenames.h"
#include "epid/member/tiny/stdlib/tiny_stdlib.h"

#if defined(SHA256_SUPPORT)
#define BASENAME_SHA_ALG kSha256
#elif defined(SHA512_SUPPORT)
#define BASENAME_SHA_ALG kSha512
#endif

size_t BasenamesGetSize(size_t num_basenames) {
  return sizeof(AllowedBasenames) - sizeof(sha_digest) +
         sizeof(sha_digest) * (num_basenames);
}

void InitBasenames(AllowedBasenames* basename_container, size_t num_basenames) {
  basename_container->current_bsn_number = 0;
  basename_container->max_bsn_number = num_basenames;
  memset(basename_container->basename_digest->digest, 0,
         sizeof(basename_container->basename_digest) * num_basenames);
}

int IsBasenameAllowed(AllowedBasenames const* basename_container,
                      void const* basename, size_t length) {
  size_t d = 0;
  tiny_sha sha_state;
  sha_digest digest;
  // calculate hash of input basename
  tinysha_init(BASENAME_SHA_ALG, &sha_state);
  tinysha_update(&sha_state, basename, length);
  tinysha_final(digest.digest, &sha_state);
  // compare hash of input basename with stored hashes
  for (d = 0; d < basename_container->current_bsn_number; d++) {
    if (!memcmp(digest.digest, &basename_container->basename_digest[d].digest,
                tinysha_digest_size(&sha_state))) {
      return 1;
    }
  }
  return 0;
}

int AllowBasename(AllowedBasenames* basename_container, void const* basename,
                  size_t length) {
  tiny_sha sha_state;
  sha_digest digest;
  if (basename_container->max_bsn_number <=
      basename_container->current_bsn_number) {
    return 0;
  }
  // calculate hash of input basename
  tinysha_init(BASENAME_SHA_ALG, &sha_state);
  tinysha_update(&sha_state, basename, length);
  tinysha_final(digest.digest, &sha_state);
  // copy hash of input basename into digest buffer
  basename_container->basename_digest[basename_container->current_bsn_number] =
      digest;
  basename_container->current_bsn_number++;
  return 1;
}
