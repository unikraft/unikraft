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
/// EpidMemberWritePrecomp interface.
/*! \file */
#ifndef EPID_MEMBER_SRC_WRITE_PRECOMP_H_
#define EPID_MEMBER_SRC_WRITE_PRECOMP_H_

#include "epid/common/errors.h"

/// \cond
typedef struct MemberCtx MemberCtx;
typedef struct MemberPrecomp MemberPrecomp;
/// \endcond

/// Serializes the pre-computed member settings.
/*!
\param[in] ctx
The member context.
\param[out] precomp
The Serialized pre-computed member settings.

\returns ::EpidStatus

\note
If the result is not ::kEpidNoErr, the content of precomp is undefined.

\b Example

\ref UserManual_GeneratingAnIntelEpidSignature
*/
EpidStatus EpidMemberWritePrecomp(MemberCtx const* ctx, MemberPrecomp* precomp);

#endif  // EPID_MEMBER_SRC_WRITE_PRECOMP_H_
