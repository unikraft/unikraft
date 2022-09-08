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
/// Implements ResizeOctStr
/*! \file */

#include "epid/member/src/resize.h"

#include <stdint.h>
#include "epid/common/src/memory.h"

EpidStatus ResizeOctStr(ConstOctStr a, size_t a_size, OctStr r, size_t r_size) {
  if (!a || !a_size || !r || !r_size) return kEpidBadArgErr;
  if (a_size <= r_size) {
    memset(r, 0, r_size - a_size);
    if (memcpy_S((uint8_t*)r + (r_size - a_size), a_size, a, a_size))
      return kEpidErr;
  } else {
    size_t i;
    for (i = 0; i < a_size - r_size; i++) {
      if (((uint8_t*)a)[i])
        return kEpidBadArgErr;  // a does not fit into r_size
    }
    if (memcpy_S(r, r_size, (uint8_t*)a + (a_size - r_size), r_size))
      return kEpidErr;
  }
  return kEpidNoErr;
}
