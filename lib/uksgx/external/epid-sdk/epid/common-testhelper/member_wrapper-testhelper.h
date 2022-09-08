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
 * \brief Member C++ wrapper interface.
 */
#ifndef EPID_COMMON_TESTHELPER_MEMBER_WRAPPER_TESTHELPER_H_
#define EPID_COMMON_TESTHELPER_MEMBER_WRAPPER_TESTHELPER_H_

extern "C" {
#include "epid/member/api.h"
}

/// C++ Wrapper to manage memory for MemberCtx via RAII
class MemberCtxObj {
 public:
  /// Create a MemberCtx
  explicit MemberCtxObj(GroupPubKey const& pub_key, PrivKey const& priv_key,
                        BitSupplier rnd_func, void* rnd_param);
  /// Create a MemberCtx
  explicit MemberCtxObj(GroupPubKey const& pub_key, PrivKey const& priv_key,
                        HashAlg hash_alg, BitSupplier rnd_func,
                        void* rnd_param);
  /// Create a MemberCtx
  explicit MemberCtxObj(BitSupplier rnd_func, void* rnd_param);
  /// Create a MemberCtx
  explicit MemberCtxObj(MemberParams const* params);
  /// Create a MemberCtx
  explicit MemberCtxObj(GroupPubKey const& pub_key,
                        MembershipCredential const& cred, BitSupplier rnd_func,
                        void* rnd_param);
  /// Create a MemberCtx given precomputation blob
  MemberCtxObj(GroupPubKey const& pub_key, PrivKey const& priv_key,
               MemberPrecomp const& precomp, BitSupplier rnd_func,
               void* rnd_param);
  /// Create a MemberCtx given precomputation blob
  MemberCtxObj(GroupPubKey const& pub_key, PrivKey const& priv_key,
               HashAlg hash_alg, MemberPrecomp const& precomp,
               BitSupplier rnd_func, void* rnd_param);
  /// Create a MemberCtx given precomputation blob
  MemberCtxObj(GroupPubKey const& pub_key, MembershipCredential const& cred,
               MemberPrecomp const& precomp, BitSupplier rnd_func,
               void* rnd_param);
  /// Create a MemberCtx given precomputation blob
  MemberCtxObj(GroupPubKey const& pub_key, MembershipCredential const& cred,
               HashAlg hash_alg, BitSupplier rnd_func, void* rnd_param);
  // This class instances are not meant to be copied.
  // Explicitly delete copy constructor and assignment operator.
  MemberCtxObj(const MemberCtxObj&) = delete;
  MemberCtxObj& operator=(const MemberCtxObj&) = delete;

  /// Destroy the MemberCtx
  ~MemberCtxObj();
  /// get a pointer to the stored MemberCtx
  MemberCtx* ctx() const;
  /// cast operator to get the pointer to the stored MemberCtx
  operator MemberCtx*() const;
  /// const cast operator to get the pointer to the stored MemberCtx
  operator const MemberCtx*() const;

 private:
  /// Creates and initializes new member given parameters.
  MemberCtx* CreateMember(MemberParams const* params) const;
  /// Deletes the member created by CreateMember().
  void DeleteMember(MemberCtx** ctx) const;
  /// The stored MemberCtx
  MemberCtx* ctx_;
};

#endif  // EPID_COMMON_TESTHELPER_MEMBER_WRAPPER_TESTHELPER_H_
