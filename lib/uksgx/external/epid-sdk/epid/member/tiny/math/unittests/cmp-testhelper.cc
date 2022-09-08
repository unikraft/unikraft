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
/// Comparators for tiny math types.
/*! \file */

#include "epid/member/tiny/math/unittests/cmp-testhelper.h"

#include <cstring>

extern "C" {
#include "epid/member/tiny/math/efq.h"
#include "epid/member/tiny/math/efq2.h"
#include "epid/member/tiny/math/mathtypes.h"
}

bool operator==(VeryLargeInt const& lhs, VeryLargeInt const& rhs) {
  return 0 == std::memcmp(&lhs, &rhs, sizeof(lhs));
}

bool operator==(VeryLargeIntProduct const& lhs,
                VeryLargeIntProduct const& rhs) {
  return 0 == std::memcmp(&lhs, &rhs, sizeof(lhs));
}
bool operator==(FpElem const& lhs, FpElem const& rhs) {
  return 0 == std::memcmp(&lhs, &rhs, sizeof(lhs));
}

bool operator==(FqElem const& lhs, FqElem const& rhs) {
  return 0 == std::memcmp(&lhs, &rhs, sizeof(lhs));
}

bool operator==(Fq2Elem const& lhs, Fq2Elem const& rhs) {
  return 0 == std::memcmp(&lhs, &rhs, sizeof(lhs));
}

bool operator==(Fq6Elem const& lhs, Fq6Elem const& rhs) {
  return 0 == std::memcmp(&lhs, &rhs, sizeof(lhs));
}

bool operator==(Fq12Elem const& lhs, Fq12Elem const& rhs) {
  return 0 == std::memcmp(&lhs, &rhs, sizeof(lhs));
}

bool operator==(EccPointFq const& lhs, EccPointFq const& rhs) {
  return 0 == std::memcmp(&lhs, &rhs, sizeof(lhs));
}

bool operator==(EccPointJacobiFq const& lhs, EccPointJacobiFq const& rhs) {
  return 0 != EFqEq(&lhs, &rhs);
}

bool operator==(EccPointFq2 const& lhs, EccPointFq2 const& rhs) {
  return 0 == std::memcmp(&lhs, &rhs, sizeof(lhs));
}

bool operator==(EccPointJacobiFq2 const& lhs, EccPointJacobiFq2 const& rhs) {
  return 0 != EFq2Eq(&lhs, &rhs);
}
