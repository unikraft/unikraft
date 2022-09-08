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

#include "cppmicroservices/ShrinkableVector.h"

#include "TestUtils.h"
#include "TestingConfig.h"
#include "TestingMacros.h"

namespace cppmicroservices {

// Fake a BundleHooks class so we can create
// ShrinkableVector instances
class BundleHooks
{

public:
  template<class E>
  static ShrinkableVector<E> MakeVector(std::vector<E>& c)
  {
    return ShrinkableVector<E>(c);
  }
};
}

using namespace cppmicroservices;

int ShrinkableVectorTest(int /*argc*/, char* /*argv*/ [])
{
  US_TEST_BEGIN("ShrinkableVectorTest");

  ShrinkableVector<int>::container_type vec{ 1, 2, 3 };

  auto shrinkable = BundleHooks::MakeVector(vec);

  US_TEST_CONDITION(vec.size() == 3, "Original size")
  US_TEST_CONDITION(vec.size() == shrinkable.size(), "Equal size")
  US_TEST_CONDITION(shrinkable.at(0) == 1, "At access")
  US_TEST_CONDITION(shrinkable.back() == 3, "back() access")

  shrinkable.pop_back();
  US_TEST_CONDITION(shrinkable.back() == 2, "back() access")

  US_TEST_FOR_EXCEPTION(std::out_of_range, shrinkable.at(3))

  US_TEST_END()
}
