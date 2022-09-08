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
/// Basename management implementation
/*! \file */

#include "epid/member/src/allowed_basenames.h"

#include <stdint.h>

#include "epid/common/src/memory.h"

typedef struct AllowedBasename {
  struct AllowedBasename* next;  ///< pointer to the next base name
  size_t length;                 ///< size of base name
  uint8_t name[1];               ///< base name (flexible array)
} AllowedBasename;

typedef struct AllowedBasenames { AllowedBasename* data; } AllowedBasenames;

/// Creates empty list of allowed basenames
EpidStatus CreateBasenames(AllowedBasenames** basename_container) {
  AllowedBasenames* new_container = NULL;
  if (!basename_container) {
    return kEpidBadArgErr;
  }
  new_container = SAFE_ALLOC(sizeof(AllowedBasenames));
  if (!new_container) {
    return kEpidMemAllocErr;
  }
  new_container->data = NULL;
  *basename_container = new_container;

  return kEpidNoErr;
}

/// Checks if given basename is in the allowed list
bool IsBasenameAllowed(AllowedBasenames const* basenames, void const* basename,
                       size_t length) {
  if (!basenames || !length) {
    return false;
  } else {
    AllowedBasename* rootnode = basenames->data;
    while (rootnode != NULL) {
      if (rootnode->length == length) {
        if (!memcmp(rootnode->name, basename, length)) {
          return true;
        }
      }
      rootnode = rootnode->next;
    }
  }
  return false;
}

/// Adds a new allowed basename
EpidStatus AllowBasename(AllowedBasenames* basenames, void const* basename,
                         size_t length) {
  AllowedBasename* newnode = NULL;

  if (length > (SIZE_MAX - sizeof(AllowedBasename)) + 1) {
    return kEpidBadArgErr;
  }
  if (!basenames || !basename) {
    return kEpidBadArgErr;
  }

  newnode = SAFE_ALLOC(sizeof(AllowedBasename) + (length - 1));
  if (!newnode) {
    return kEpidMemAllocErr;
  }

  newnode->next = NULL;
  newnode->length = length;
  // Memory copy is used to copy a flexible array
  if (0 != memcpy_S(newnode->name, length, basename, length)) {
    SAFE_FREE(newnode);
    return kEpidBadArgErr;
  }

  if (!basenames->data) {
    basenames->data = newnode;
  } else {
    AllowedBasename* currentnode = basenames->data;
    while (NULL != currentnode->next) {
      currentnode = currentnode->next;
    }
    currentnode->next = newnode;
  }
  return kEpidNoErr;
}

/// Deletes list of allowed basenames
void DeleteBasenames(AllowedBasenames** basename_container) {
  if (basename_container && *basename_container) {
    AllowedBasename* rootnode = (*basename_container)->data;
    while (rootnode) {
      AllowedBasename* deletenode = rootnode;
      rootnode = rootnode->next;
      SAFE_FREE(deletenode);
    }
    (*basename_container)->data = NULL;
    SAFE_FREE(*basename_container);
  }
}
