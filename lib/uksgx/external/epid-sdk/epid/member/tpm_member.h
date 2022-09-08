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
/// Member creation parameters for TPM implementation.
/*!
 * \file
 */
#ifndef EPID_MEMBER_TPM_MEMBER_H_
#define EPID_MEMBER_TPM_MEMBER_H_
/*!
* \cond
*/

#include "epid/common/bitsupplier.h"
#include "epid/common/types.h"

/*!
\addtogroup EpidMemberModule member
@{
*/

/// TPM specific member parameters
/*!
 \class TPMMemberParams
*/
typedef struct MemberParams {
  FpElemStr const* f;  ///< Secret part of the private key. If NULL an
                       ///  EPS based primary will be used.
} MemberParams;

/*!
 * @}
 * \endcond
 */

#endif  // EPID_MEMBER_TPM_MEMBER_H_
