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
/// Member context interface.
/*! \file */
#ifndef EPID_MEMBER_TINY_SRC_CONTEXT_H_
#define EPID_MEMBER_TINY_SRC_CONTEXT_H_
#include "epid/common/bitsupplier.h"
#include "epid/common/types.h"
#include "epid/member/tiny/math/mathtypes.h"
#include "epid/member/tiny/src/allowed_basenames.h"
#include "epid/member/tiny/src/native_types.h"

/// Size of SigRl with zero entries
#define MIN_SIGRL_SIZE (sizeof(SigRl) - sizeof(SigRlEntry))

#ifdef USE_SIGRL_BY_REFERENCE

// SIZE_MAX is not guaranteed in C89/90
#define SIZE_T_MAX ((size_t)(-1))

/// Maximum number of possible entries in SigRl used by reference
#define MAX_SIGRL_ENTRIES ((SIZE_T_MAX - MIN_SIGRL_SIZE) / sizeof(SigRlEntry))

/// Maximum space needed to store SigRl data in context
#define SIGRL_HEAP_SIZE (0)

#else  // !defined(USE_SIGRL_BY_REFERENCE)

#ifndef MAX_SIGRL_ENTRIES
/// Maximum number of possible entries in SigRl copied by value
#define MAX_SIGRL_ENTRIES (5)
#endif

/// Maximum space needed to store SigRl data in context
#define SIGRL_HEAP_SIZE \
  (MIN_SIGRL_SIZE + MAX_SIGRL_ENTRIES * sizeof(SigRlEntry))

#endif  // !defined(USE_SIGRL_BY_REFERENCE)

#ifndef MAX_ALLOWED_BASENAMES
/// Maximum number of allowed base names
#define MAX_ALLOWED_BASENAMES (5)
#endif

/// Member context definition
typedef struct MemberCtx {
  GroupPubKey pub_key;              ///< group public key
  HashAlg hash_alg;                 ///< Hash algorithm to use
  MembershipCredential credential;  ///< Membership credential
  FpElem f;                         ///< secret f value
  NativeMemberPrecomp precomp;      ///< Precomputed pairing values
  PairingState pairing_state;       ///< pairing state
  int f_is_set;                     ///< f initialized
  int is_provisioned;    ///< member fully provisioned with key material
  BitSupplier rnd_func;  ///< Pseudo random number generation function
  void* rnd_param;       ///< Pointer to user context for rnd_func
  AllowedBasenames* allowed_basenames;  ///< Allowed basenames
  SigRl* sig_rl;          ///< Pointer to Signature based revocation list
  unsigned char heap[1];  ///< Bulk storage space (flexible array)
} MemberCtx;

#endif  // EPID_MEMBER_TINY_SRC_CONTEXT_H_
