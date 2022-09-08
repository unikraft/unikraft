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
/// Basic signature computation
/*! \file */

#ifndef EPID_MEMBER_TINY_SRC_SIGNBASIC_H_
#define EPID_MEMBER_TINY_SRC_SIGNBASIC_H_

#include <stddef.h>

#include "epid/common/errors.h"
#include "epid/member/tiny/math/mathtypes.h"

/// \cond
typedef struct MemberCtx MemberCtx;
typedef struct NativeBasicSignature NativeBasicSignature;
/// \endcond

/// Compute Intel(R) EPID Basic Signature.
EpidStatus EpidSignBasic(MemberCtx const* ctx, void const* msg, size_t msg_len,
                         void const* basename, size_t basename_len,
                         NativeBasicSignature* sig);

#endif  // EPID_MEMBER_TINY_SRC_SIGNBASIC_H_
