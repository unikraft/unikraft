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
/// Basename management APIs
/*! \file */

#ifndef EPID_MEMBER_SRC_ALLOWED_BASENAMES_H_
#define EPID_MEMBER_SRC_ALLOWED_BASENAMES_H_

#include <stddef.h>
#include "epid/common/errors.h"
#include "epid/common/stdtypes.h"

/// \cond
typedef struct AllowedBasenames AllowedBasenames;
typedef struct AllowedBasename AllowedBasename;
/// \endcond

/// Creates empty list of allowed basenames
EpidStatus CreateBasenames(AllowedBasenames** basename_container);

/// Checks if given basename is in the allowed list
bool IsBasenameAllowed(AllowedBasenames const* basenames, void const* basename,
                       size_t length);

/// Adds a new allowed basename
EpidStatus AllowBasename(AllowedBasenames* basenames, void const* basename,
                         size_t length);

/// Deletes list of allowed basenames
void DeleteBasenames(AllowedBasenames** basename_container);

#endif  // EPID_MEMBER_SRC_ALLOWED_BASENAMES_H_
