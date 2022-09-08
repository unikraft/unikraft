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

#ifndef EPID_MEMBER_TINY_SRC_ALLOWED_BASENAMES_H_
#define EPID_MEMBER_TINY_SRC_ALLOWED_BASENAMES_H_

#include <stddef.h>
#include "epid/member/tiny/math/hashwrap.h"

typedef struct AllowedBasenames {
  size_t current_bsn_number;  ///< Number of basenames registered
  size_t max_bsn_number;      ///< Maximum number of basenames to store
  sha_digest
      basename_digest[1];  ///< digest of registrered basenames (flexible array)
} AllowedBasenames;

/// Get allowed basenames container size
size_t BasenamesGetSize(size_t num_basenames);

/// Initilize allowed basenames container
void InitBasenames(AllowedBasenames* basename_container, size_t num_basenames);

/// Checks if given basename is allowed
int IsBasenameAllowed(AllowedBasenames const* basename_container,
                      void const* basename, size_t length);

/// Adds a new allowed basename
int AllowBasename(AllowedBasenames* basename_container, void const* basename,
                  size_t length);

#endif  // EPID_MEMBER_TINY_SRC_ALLOWED_BASENAMES_H_
