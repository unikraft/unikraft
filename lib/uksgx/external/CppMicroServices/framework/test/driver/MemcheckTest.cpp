/*=============================================================================

Library: CppMicroServices

Copyright (c) The CppMicroServices developers. See the COPYRIGHT
file at the top-level directory of this distribution and at
https://github.com/CppMicroServices/CppMicroServices/COPYRIGHT .

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

=============================================================================*/

#include "TestingMacros.h"

/*
 * This test is meant to be run with an address sanity checker. E.g. address
 * sanitizer (using Clang or GCC) or memcheck.
 */

int MemcheckTest(int /*argc*/, char* /*argv*/ [])
{
  US_TEST_BEGIN("MemcheckTest");

  // this leak is intentional
  auto x = new int(1);
  std::cout << x;

  US_TEST_END()
}
