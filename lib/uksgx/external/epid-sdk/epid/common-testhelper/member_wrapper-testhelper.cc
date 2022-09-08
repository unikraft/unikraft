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
 * \brief Member C++ wrapper implementation.
 */

#include "epid/common-testhelper/member_wrapper-testhelper.h"

#include <stdint.h>
#include <cstdio>
#include <cstring>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>

extern "C" {
#include "epid/common/types.h"
}

#include "epid/common-testhelper/mem_params-testhelper.h"

/// Handle SDK Error with Break
#define BREAK_ON_EPID_ERROR(ret) \
  if (kEpidNoErr != (ret)) {     \
    break;                       \
  }

MemberCtxObj::MemberCtxObj(GroupPubKey const& pub_key, PrivKey const& priv_key,
                           BitSupplier rnd_func, void* rnd_param)
    : ctx_(nullptr) {
  EpidStatus sts = kEpidErr;
  MemberParams params = {0};
  SetMemberParams(rnd_func, rnd_param, &priv_key.f, &params);
  ctx_ = CreateMember(&params);
  sts = EpidProvisionKey(ctx_, &pub_key, &priv_key, nullptr);
  if (kEpidNoErr != sts) {
    DeleteMember(&ctx_);
    printf("%s(%d): %s\n", __FILE__, __LINE__, "test defect:");
    throw std::logic_error(std::string("Failed to call: ") +
                           "EpidProvisionKey()");
  }
  sts = EpidMemberStartup(ctx_);
  if (kEpidNoErr != sts) {
    DeleteMember(&ctx_);
    printf("%s(%d): %s\n", __FILE__, __LINE__, "test defect:");
    throw std::logic_error(std::string("Failed to call: ") +
                           "EpidMemberStartup()");
  }
}

MemberCtxObj::MemberCtxObj(GroupPubKey const& pub_key,
                           MembershipCredential const& cred,
                           BitSupplier rnd_func, void* rnd_param)
    : ctx_(nullptr) {
  EpidStatus sts = kEpidErr;
  MemberParams params = {0};
  SetMemberParams(rnd_func, rnd_param, nullptr, &params);
  ctx_ = CreateMember(&params);
  sts = EpidProvisionCredential(ctx_, &pub_key, &cred, nullptr);
  if (kEpidNoErr != sts) {
    DeleteMember(&ctx_);
    printf("%s(%d): %s\n", __FILE__, __LINE__, "test defect:");
    throw std::logic_error(std::string("Failed to call: ") +
                           "EpidProvisionKey()");
  }
  sts = EpidMemberStartup(ctx_);
  if (kEpidNoErr != sts) {
    DeleteMember(&ctx_);
    printf("%s(%d): %s\n", __FILE__, __LINE__, "test defect:");
    throw std::logic_error(std::string("Failed to call: ") +
                           "EpidMemberStartup()");
  }
}

MemberCtxObj::MemberCtxObj(GroupPubKey const& pub_key, PrivKey const& priv_key,
                           HashAlg hash_alg, BitSupplier rnd_func,
                           void* rnd_param)
    : ctx_(nullptr) {
  EpidStatus sts = kEpidErr;
  MemberParams params = {0};
  SetMemberParams(rnd_func, rnd_param, &priv_key.f, &params);
  ctx_ = CreateMember(&params);
  sts = EpidMemberSetHashAlg(ctx_, hash_alg);
  if (kEpidNoErr != sts) {
    DeleteMember(&ctx_);
    printf("%s(%d): %s\n", __FILE__, __LINE__, "test defect:");
    throw std::logic_error(std::string("Failed to call: ") +
                           "EpidMemberSetHashAlg()");
  }
  sts = EpidProvisionKey(ctx_, &pub_key, &priv_key, nullptr);
  if (kEpidNoErr != sts) {
    DeleteMember(&ctx_);
    printf("%s(%d): %s\n", __FILE__, __LINE__, "test defect:");
    throw std::logic_error(std::string("Failed to call: ") +
                           "EpidProvisionKey()");
  }
  sts = EpidMemberStartup(ctx_);
  if (kEpidNoErr != sts) {
    DeleteMember(&ctx_);
    printf("%s(%d): %s\n", __FILE__, __LINE__, "test defect:");
    throw std::logic_error(std::string("Failed to call: ") +
                           "EpidMemberStartup()");
  }
}

MemberCtxObj::MemberCtxObj(BitSupplier rnd_func, void* rnd_param)
    : ctx_(nullptr) {
  MemberParams params = {0};
  SetMemberParams(rnd_func, rnd_param, nullptr, &params);
  ctx_ = CreateMember(&params);
}

MemberCtxObj::MemberCtxObj(MemberParams const* params) : ctx_(nullptr) {
  ctx_ = CreateMember(params);
}

MemberCtxObj::MemberCtxObj(GroupPubKey const& pub_key, PrivKey const& priv_key,
                           MemberPrecomp const& precomp, BitSupplier rnd_func,
                           void* rnd_param)
    : ctx_(nullptr) {
  EpidStatus sts = kEpidErr;
  MemberParams params = {0};
  SetMemberParams(rnd_func, rnd_param, &priv_key.f, &params);
  ctx_ = CreateMember(&params);
  sts = EpidProvisionKey(ctx_, &pub_key, &priv_key, &precomp);
  if (kEpidNoErr != sts) {
    DeleteMember(&ctx_);
    printf("%s(%d): %s\n", __FILE__, __LINE__, "test defect:");
    throw std::logic_error(std::string("Failed to call: ") +
                           "EpidProvisionKey()");
  }
  sts = EpidMemberStartup(ctx_);
  if (kEpidNoErr != sts) {
    DeleteMember(&ctx_);
    printf("%s(%d): %s\n", __FILE__, __LINE__, "test defect:");
    throw std::logic_error(std::string("Failed to call: ") +
                           "EpidMemberStartup()");
  }
}

MemberCtxObj::MemberCtxObj(GroupPubKey const& pub_key, PrivKey const& priv_key,
                           HashAlg hash_alg, MemberPrecomp const& precomp,
                           BitSupplier rnd_func, void* rnd_param)
    : ctx_(nullptr) {
  EpidStatus sts = kEpidErr;
  MemberParams params = {0};
  SetMemberParams(rnd_func, rnd_param, &priv_key.f, &params);
  ctx_ = CreateMember(&params);
  sts = EpidMemberSetHashAlg(ctx_, hash_alg);
  if (kEpidNoErr != sts) {
    DeleteMember(&ctx_);
    printf("%s(%d): %s\n", __FILE__, __LINE__, "test defect:");
    throw std::logic_error(std::string("Failed to call: ") +
                           "EpidProvisionKey()");
  }
  sts = EpidProvisionKey(ctx_, &pub_key, &priv_key, &precomp);
  if (kEpidNoErr != sts) {
    DeleteMember(&ctx_);
    printf("%s(%d): %s\n", __FILE__, __LINE__, "test defect:");
    throw std::logic_error(std::string("Failed to call: ") +
                           "EpidProvisionKey()");
  }
  sts = EpidMemberStartup(ctx_);
  if (kEpidNoErr != sts) {
    DeleteMember(&ctx_);
    printf("%s(%d): %s\n", __FILE__, __LINE__, "test defect:");
    throw std::logic_error(std::string("Failed to call: ") +
                           "EpidMemberStartup()");
  }
}

MemberCtxObj::MemberCtxObj(GroupPubKey const& pub_key,
                           MembershipCredential const& cred,
                           MemberPrecomp const& precomp, BitSupplier rnd_func,
                           void* rnd_param)
    : ctx_(nullptr) {
  EpidStatus sts = kEpidErr;
  MemberParams params = {0};
  SetMemberParams(rnd_func, rnd_param, nullptr, &params);
  ctx_ = CreateMember(&params);
  sts = EpidProvisionCredential(ctx_, &pub_key, &cred, &precomp);
  if (kEpidNoErr != sts) {
    DeleteMember(&ctx_);
    printf("%s(%d): %s\n", __FILE__, __LINE__, "test defect:");
    throw std::logic_error(std::string("Failed to call: ") +
                           "EpidProvisionCredential()");
  }
  sts = EpidMemberStartup(ctx_);
  if (kEpidNoErr != sts) {
    DeleteMember(&ctx_);
    printf("%s(%d): %s\n", __FILE__, __LINE__, "test defect:");
    throw std::logic_error(std::string("Failed to call: ") +
                           "EpidMemberStartup()");
  }
}

MemberCtxObj::MemberCtxObj(GroupPubKey const& pub_key,
                           MembershipCredential const& cred, HashAlg hash_alg,
                           BitSupplier rnd_func, void* rnd_param)
    : ctx_(nullptr) {
  EpidStatus sts = kEpidErr;
  MemberParams params = {0};
  SetMemberParams(rnd_func, rnd_param, nullptr, &params);
  ctx_ = CreateMember(&params);
  sts = EpidMemberSetHashAlg(ctx_, hash_alg);
  if (kEpidNoErr != sts) {
    DeleteMember(&ctx_);
    printf("%s(%d): %s\n", __FILE__, __LINE__, "test defect:");
    throw std::logic_error(std::string("Failed to call: ") +
                           "EpidProvisionKey()");
  }
  sts = EpidProvisionCredential(ctx_, &pub_key, &cred, nullptr);
  if (kEpidNoErr != sts) {
    DeleteMember(&ctx_);
    printf("%s(%d): %s\n", __FILE__, __LINE__, "test defect:");
    throw std::logic_error(std::string("Failed to call: ") +
                           "EpidProvisionCredential()");
  }
  sts = EpidMemberStartup(ctx_);
  if (kEpidNoErr != sts) {
    DeleteMember(&ctx_);
    printf("%s(%d): %s\n", __FILE__, __LINE__, "test defect:");
    throw std::logic_error(std::string("Failed to call: ") +
                           "EpidMemberStartup()");
  }
}

MemberCtxObj::~MemberCtxObj() { DeleteMember(&ctx_); }

MemberCtx* MemberCtxObj::ctx() const { return ctx_; }

MemberCtxObj::operator MemberCtx*() const { return ctx_; }

MemberCtxObj::operator const MemberCtx*() const { return ctx_; }

MemberCtx* MemberCtxObj::CreateMember(MemberParams const* params) const {
  size_t context_size = 0;
  EpidStatus sts = kEpidErr;
  MemberCtx* member_ctx = NULL;

  sts = EpidMemberGetSize(params, &context_size);
  if (kEpidNoErr != sts) {
    printf("%s(%d): %s\n", __FILE__, __LINE__, "test defect:");
    throw std::logic_error(std::string("Failed to call: ") +
                           "EpidMemberGetSize()");
  }

  // allocate and zero initialize, no throw on failure
  member_ctx = (MemberCtx*)new (std::nothrow) uint8_t[context_size]();
  if (!member_ctx) {
    printf("%s(%d): %s\n", __FILE__, __LINE__, "test defect:");
    std::ostringstream err;
    err << "Failed to allocate " << context_size
        << "bytes after EpidMemberGetSize()";
    throw std::logic_error(err.str());
  }

  sts = EpidMemberInit(params, member_ctx);
  if (kEpidNoErr != sts) {
    delete[](uint8_t*) member_ctx;
    printf("%s(%d): %s\n", __FILE__, __LINE__, "test defect:");
    throw std::logic_error(std::string("Failed to call: ") +
                           "EpidMemberInit()");
  }
  return member_ctx;
}

void MemberCtxObj::DeleteMember(MemberCtx** ctx) const {
  if (ctx) {
    EpidMemberDeinit(*ctx);
    delete[](uint8_t*)(*ctx);
    *ctx = NULL;
  }
}
