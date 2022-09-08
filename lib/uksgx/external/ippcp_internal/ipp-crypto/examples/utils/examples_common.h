/*******************************************************************************
* Copyright 2019-2021 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

/* Intel® Integrated Performance Primitives Cryptography (Intel® IPP Cryptography) */

/*!
  *
  * \file
  * \brief Common header for Intel IPP Cryptography examples
  *
  */

#ifndef EXAMPLES_COMMON_H_
#define EXAMPLES_COMMON_H_

#include <stdio.h>

/*! Macro that prints status message depending on condition */
#define PRINT_EXAMPLE_STATUS(function_name, description, success_condition)       \
    printf("+--------------------------------------------------------------|\n"); \
    printf(" Function: %s\n", function_name);                                     \
    printf(" Description: %s\n", description);                                    \
    if (success_condition) {                                                      \
        printf(" Status: PASSED!\n");                                             \
    } else {                                                                      \
        printf(" Status: FAILED!\n");                                             \
    }                                                                             \
    printf("+--------------------------------------------------------------|\n");

/*!
 * Helper function to compare expected and actual function return statuses and display
 * an error mesage if those are different.
 *
 * \param[in] Function name to display
 * \param[in] Expected status
 * \param[in] Actual status
 *
 * \return zero if statuses are not equal, otherwise - non-zero value
 */
static int checkStatus(const char* funcName, IppStatus expectedStatus, IppStatus status)
{
   if (expectedStatus != status) {
      printf("%s: unexpected return status\n", funcName);
      printf("Expected: %s\n", ippcpGetStatusString(expectedStatus));
      printf("Received: %s\n", ippcpGetStatusString(status));
      return 0;
   }
   return 1;
}

/*!
 * Helper function to convert bit size into byte size.
 *
 * \param[in] Size in bits
 *
 * \return size in bytes
 */
static int bitSizeInBytes(int nBits)
{
   return (nBits + 7) >> 3;
}

/*!
 * Helper function to convert bit size into word size.
 *
 * \param[in] Size in bits
 *
 * \return size in words
 */

static int bitSizeInWords(int nBits)
{
    return (nBits + 31) >> 5;
}

#endif /* #ifndef EXAMPLES_COMMON_H_ */
