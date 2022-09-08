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

#ifndef EPID_MEMBER_TINY_STDLIB_TINY_STDLIB_H_
#define EPID_MEMBER_TINY_STDLIB_TINY_STDLIB_H_

#include <stddef.h>

/// Fill block of memory
/*!
 * Sets the first num bytes of the block of memory pointed by ptr
 * to the specified value (interpreted as an unsigned char)
 *
 * \param ptr Pointer to the block of memory to fill.
 * \param value Value to be set. The value is passed as an int,
 *        but the function fills the block of memory using the
 *        unsigned char conversion of this value.
 * \param num Number of bytes to be set to the value. size_t is
 *        an unsigned integral type.
 * \result ptr is returned.
 */
void* memset(void* ptr, int value, size_t num);

/// Compare two blocks of memory
/*!
 * Compares the first num bytes of the block of memory pointed by
 * ptr1 to the first num bytes pointed by ptr2, returning zero if
 * they all match or a value different from zero representing which
 * is greater if they do not.
 *
 * Notice that, unlike strcmp, the function does not stop comparing
 * after finding a null character.
 *
 * \param ptr1 Pointer to block of memory.
 * \param ptr2 Pointer to block of memory.
 * \param num Number of bytes to compare.
 * \result an integral value indicating the relationship between the
 *         content of the memory blocks:
 * \retval <0 the first byte that does not match in both memory
 *            blocks has a lower value in ptr1 than in ptr2 (if
 *            evaluated as unsigned char values)
 * \retval 0 the contents of both memory blocks are equal
 * \retval >0 the first byte that does not match in both memory blocks
 *            has a greater value in ptr1 than in ptr2 (if evaluated
 *            as unsigned char values)
 */
int memcmp(const void* ptr1, const void* ptr2, size_t num);

#endif  // EPID_MEMBER_TINY_STDLIB_TINY_STDLIB_H_
