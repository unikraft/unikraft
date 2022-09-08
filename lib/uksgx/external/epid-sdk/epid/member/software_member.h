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
/// Member creation parameters for software only implementation.
/*!
 * \file
 */
#ifndef EPID_MEMBER_SOFTWARE_MEMBER_H_
#define EPID_MEMBER_SOFTWARE_MEMBER_H_

#include "epid/common/bitsupplier.h"
#include "epid/common/types.h"

/*!
 \addtogroup EpidMemberModule member
 @{
 */

/// Software only specific member parameters
/*!
 You need to use a cryptographically secure random
 number generator to create a member context using
 ::EpidMemberGetSize and ::EpidMemberInit. The ::BitSupplier is provided
 as a function prototype for your own implementation
 of the random number generator.
*/
typedef struct MemberParams {
  BitSupplier rnd_func;  ///< Random number generator.
  void* rnd_param;       ///< User data that will be passed to the user_data
                         ///  parameter of the random number generator.
  FpElemStr const* f;    ///< Secret part of the private key. If NULL a random
                         ///  value will be generated using rnd_func.
} MemberParams;
/*! @} */

#endif  // EPID_MEMBER_SOFTWARE_MEMBER_H_
