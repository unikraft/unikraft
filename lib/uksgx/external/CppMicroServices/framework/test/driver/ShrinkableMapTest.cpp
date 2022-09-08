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

#include "cppmicroservices/ShrinkableMap.h"

#include "TestUtils.h"
#include "TestingConfig.h"
#include "TestingMacros.h"

namespace cppmicroservices {

// Fake a ServiceHooks class so we can create
// ShrinkableMap instances
class ServiceHooks
{

public:
  template<class K, class V>
  static ShrinkableMap<K, V> MakeMap(std::map<K, V>& m)
  {
    return ShrinkableMap<K, V>(m);
  }
};
}

using namespace cppmicroservices;

int ShrinkableMapTest(int /*argc*/, char* /*argv*/ [])
{
  US_TEST_BEGIN("ShrinkableMapTest");

  ShrinkableMap<int, std::string>::container_type m{ { 1, "one" },
                                                     { 2, "two" },
                                                     { 3, "three" } };

  auto shrinkable = ServiceHooks::MakeMap(m);

  US_TEST_CONDITION(m.size() == 3, "Original size")
  US_TEST_CONDITION(m.size() == shrinkable.size(), "Equal size")
  US_TEST_CONDITION(shrinkable.at(1) == "one", "At access")

  shrinkable.erase(shrinkable.find(1));
  US_TEST_CONDITION(m.size() == 2, "New size")
  US_TEST_FOR_EXCEPTION(std::out_of_range, shrinkable.at(1))
  US_TEST_CONDITION(shrinkable.at(2) == "two", "back() access")

  US_TEST_END()
}
