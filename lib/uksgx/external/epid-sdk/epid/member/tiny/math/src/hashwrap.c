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
/// Implementation of hash wrap function
/*! \file */
#include "epid/member/tiny/math/hashwrap.h"

void tinysha_init(HashAlg sha_type, tiny_sha* s) {
  switch (sha_type) {
#ifdef SHA512_SUPPORT
    case kSha512:
      tinysha512_init(&s->sha_state_t.sha512s);
      break;
#endif
#ifdef SHA256_SUPPORT
    case kSha256:
      tc_sha256_init(&s->sha_state_t.sha256s);
      break;
#endif
    default:
      s->hash_alg = kInvalidHashAlg;
      return;
  }
  s->hash_alg = sha_type;
}

void tinysha_update(tiny_sha* s, void const* data, size_t data_length) {
  switch (s->hash_alg) {
#ifdef SHA512_SUPPORT
    case kSha512:
      tinysha512_update(&s->sha_state_t.sha512s, data, data_length);
      break;
#endif
#ifdef SHA256_SUPPORT
    case kSha256:
      tc_sha256_update(&s->sha_state_t.sha256s, data, data_length);
      break;
#endif
    default:
      break;
  }
}

void tinysha_final(unsigned char* digest, tiny_sha* s) {
  switch (s->hash_alg) {
#ifdef SHA512_SUPPORT
    case kSha512:
      tinysha512_final(digest, &s->sha_state_t.sha512s);
      break;
#endif
#ifdef SHA256_SUPPORT
    case kSha256:
      tc_sha256_final(digest, &s->sha_state_t.sha256s);
      break;
#endif
    default:
      break;
  }
}

size_t tinysha_digest_size(tiny_sha* s) {
  switch (s->hash_alg) {
#ifdef SHA512_SUPPORT
    case kSha512:
      return SHA512_DIGEST_SIZE;
#endif
#ifdef SHA256_SUPPORT
    case kSha256:
      return SHA256_DIGEST_SIZE;
#endif
    default:
      return 0;
  }
}
