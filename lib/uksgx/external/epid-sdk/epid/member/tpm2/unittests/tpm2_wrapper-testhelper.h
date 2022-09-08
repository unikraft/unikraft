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
/// Tpm2Ctx wrapper class.
/*! \file */
#ifndef EPID_MEMBER_TPM2_UNITTESTS_TPM2_WRAPPER_TESTHELPER_H_
#define EPID_MEMBER_TPM2_UNITTESTS_TPM2_WRAPPER_TESTHELPER_H_

#include <stdint.h>
#include <vector>

extern "C" {
#include "epid/common/bitsupplier.h"
#include "epid/common/types.h"
}

typedef struct Tpm2Ctx Tpm2Ctx;
class Epid2ParamsObj;

/// C++ Wrapper to manage memory for Tpm2Ctx via RAII
class Tpm2CtxObj {
 public:
  /// Create a Tpm2Ctx
  Tpm2CtxObj(BitSupplier rnd_func, void* rnd_param, const FpElemStr* f,
             class Epid2ParamsObj const& params);

  // This class instances are not meant to be copied.
  // Explicitly delete copy constructor and assignment operator.
  Tpm2CtxObj(const Tpm2CtxObj&) = delete;
  Tpm2CtxObj& operator=(const Tpm2CtxObj&) = delete;

  /// Destroy the Tpm2Ctx
  ~Tpm2CtxObj();
  /// get a pointer to the stored Tpm2Ctx
  Tpm2Ctx* ctx() const;
  /// cast operator to get the pointer to the stored Tpm2Ctx
  operator Tpm2Ctx*() const;
  /// const cast operator to get the pointer to the stored Tpm2Ctx
  operator const Tpm2Ctx*() const;

 private:
  /// The stored Tpm2Ctx
  Tpm2Ctx* ctx_;
};

#endif  // EPID_MEMBER_TPM2_UNITTESTS_TPM2_WRAPPER_TESTHELPER_H_
