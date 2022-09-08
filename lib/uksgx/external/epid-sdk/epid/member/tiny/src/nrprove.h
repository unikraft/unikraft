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
/// EpidNrProve interface.
/*! \file */
#ifndef EPID_MEMBER_TINY_SRC_NRPROVE_H_
#define EPID_MEMBER_TINY_SRC_NRPROVE_H_

#include <stddef.h>
#include "epid/common/errors.h"

/// \cond
typedef struct MemberCtx MemberCtx;
typedef struct NativeBasicSignature NativeBasicSignature;
typedef struct SigRlEntry SigRlEntry;
typedef struct NrProof NrProof;
/// \endcond

/// Calculates a non-revoked proof for a single signature based revocation
/// list entry.
/*!
 Used in constrained environments where, due to limited memory, it may not
 be possible to process through a large and potentially unbounded revocation
 list.

 \param[in] ctx
 The member context.
 \param[in] msg
 The message.
 \param[in] msg_len
 The length of message in bytes.
 \param[in] sig
 The basic signature.
 \param[in] sigrl_entry
 The signature based revocation list entry.
 \param[out] proof
 The generated non-revoked proof.

 \returns ::EpidStatus

 \note
 This function should be used in conjunction with EpidSignBasic().

 \note
 If the result is not ::kEpidNoErr, the content of proof is undefined.

 \see EpidMemberInit
 \see EpidSignBasic
 */
EpidStatus EpidNrProve(MemberCtx const* ctx, void const* msg, size_t msg_len,
                       NativeBasicSignature const* sig,
                       SigRlEntry const* sigrl_entry, NrProof* proof);

#endif  // EPID_MEMBER_TINY_SRC_NRPROVE_H_
