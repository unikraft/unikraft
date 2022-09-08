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
/// Declares ResizeOctStr
/*! \file */

#ifndef EPID_MEMBER_SRC_RESIZE_H_
#define EPID_MEMBER_SRC_RESIZE_H_

#include <string.h>
#include "epid/common/errors.h"
#include "epid/common/types.h"

/// Resizes octet string number
/*!
Prepends input number with zeros when increasing the size.
Removes leading zeros when decreasing the size.
Return error if input number does not fit the new size.

\param[in] a
Octet string to resize
\param[in] a_size
Size of the a
\param[out] r
Result octet string
\param[in] r_size
New size

\returns ::EpidStatus
*/
EpidStatus ResizeOctStr(ConstOctStr a, size_t a_size, OctStr r, size_t r_size);

#endif  // EPID_MEMBER_SRC_RESIZE_H_
