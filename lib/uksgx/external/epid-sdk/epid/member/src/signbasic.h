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
/// EpidSignBasic interface.
/*! \file */
#ifndef EPID_MEMBER_SRC_SIGNBASIC_H_
#define EPID_MEMBER_SRC_SIGNBASIC_H_

#include <stddef.h>
#include "epid/common/errors.h"

/// \cond
typedef struct MemberCtx MemberCtx;
typedef struct BasicSignature BasicSignature;
typedef struct BigNumStr BigNumStr;
/// \endcond

/// Creates a basic signature for use in constrained environment.
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
 \param[in] basename
 Optional basename. If basename is NULL a random basename is used.
 Signatures generated using random basenames are anonymous. Signatures
 generated using the same basename are linkable by the verifier. If a
 basename is provided it must already be registered or
 ::kEpidBadArgErr is returned.
 \param[in] basename_len
 The size of basename in bytes. Must be 0 if basename is NULL.
 \param[out] sig
 The generated basic signature
 \param[out] rnd_bsn
 Random basename, can be NULL if basename is provided.

 \returns ::EpidStatus

 \note
 This function should be used in conjunction with EpidNrProve()

 \note
 If the result is not ::kEpidNoErr the content of sig, is undefined.

 \see EpidMemberInit
 \see EpidNrProve
 */
EpidStatus EpidSignBasic(MemberCtx const* ctx, void const* msg, size_t msg_len,
                         void const* basename, size_t basename_len,
                         BasicSignature* sig, BigNumStr* rnd_bsn);

#endif  // EPID_MEMBER_SRC_SIGNBASIC_H_
