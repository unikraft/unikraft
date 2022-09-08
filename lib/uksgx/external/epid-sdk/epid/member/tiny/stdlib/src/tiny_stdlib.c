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
/// Tiny portable implementations of standard library functions
/*! \file */

#include "epid/member/tiny/stdlib/tiny_stdlib.h"

#ifdef SHARED

void* memset(void* ptr, int value, size_t num) {
  unsigned char* p = ptr;
  size_t i = num;
  while (i != 0) {
    i -= 1;
    p[i] = (unsigned char)value;
  }
  return ptr;
}

int memcmp(const void* ptr1, const void* ptr2, size_t num) {
  const unsigned char* p1 = (const unsigned char*)ptr1;
  const unsigned char* p2 = (const unsigned char*)ptr2;
  int d = 0;
  while (num != 0 && d == 0) {
    d = (*p1 == *p2) ? 0 : ((*p1 < *p2) ? -1 : 1);
    p1 += 1;
    p2 += 1;
    num -= 1;
  }
  return d;
}

#endif  // SHARED
