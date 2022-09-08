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
/// Convert values between host and big-/little-endian byte order.
/*! \file */

#ifndef EPID_MEMBER_TINY_STDLIB_ENDIAN_H_
#define EPID_MEMBER_TINY_STDLIB_ENDIAN_H_

#include <stdint.h>

/// Transform big endian uint32_t to host uint32_t
#define be32toh(big_endian_32bits)                                  \
  ((uint32_t)(((((unsigned char*)&(big_endian_32bits))[0]) << 24) + \
              ((((unsigned char*)&(big_endian_32bits))[1]) << 16) + \
              ((((unsigned char*)&(big_endian_32bits))[2]) << 8) +  \
              (((unsigned char*)&(big_endian_32bits))[3])))

/// Transform host uint32_t to big endian uint32_t
#define htobe32(host_32bits)                                 \
  (uint32_t)(((((uint32_t)(host_32bits)) & 0xFF) << 24) |    \
             ((((uint32_t)(host_32bits)) & 0xFF00) << 8) |   \
             ((((uint32_t)(host_32bits)) & 0xFF0000) >> 8) | \
             ((((uint32_t)(host_32bits)) & 0xFF000000) >> 24))

#endif  // EPID_MEMBER_TINY_STDLIB_ENDIAN_H_
