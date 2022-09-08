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

#include "cppmicroservices/AnyMap.h"

#include "TestingMacros.h"

#include <stdexcept>

using namespace cppmicroservices;

int AnyMapTest(int /*argc*/, char* /*argv*/ [])
{
  US_TEST_BEGIN("AnyMapTest");

  US_TEST_FOR_EXCEPTION_BEGIN(std::logic_error)
  AnyMap m(static_cast<AnyMap::map_type>(100));
  US_TEST_FOR_EXCEPTION_END(std::logic_error)

  AnyMap om(AnyMap::ORDERED_MAP);
  US_TEST_CONDITION(om.size() == 0, "Empty ordered map")
  US_TEST_CONDITION(om.empty(), "Empty ordered map")
  US_TEST_CONDITION(om.count("key1") == 0, "No key1 key")

  auto it =
    om.insert(std::make_pair(std::string("key1"), Any(std::string("val1"))));
  US_TEST_CONDITION(it.second, "Insert key1");
  US_TEST_CONDITION(it.first->first == "key1", "Insert iter correct")
  US_TEST_CONDITION(it.first->second == std::string("val1"),
                    "Insert iter correct")
  US_TEST_CONDITION(om["key1"] == std::string("val1"), "Get inserted item")

  /* Create a AnyMap with the following JSON representation:
   *
   * {
   *   key1 : "val1",
   *   uoci : {
   *     FiRST : 1,
   *     SECOND : 2,
   *     vec : [
   *       "one",
   *       2,
   *       {
   *         hi : "hi",
   *         there : "there"
   *       }
   *     ]
   *   },
   *   dot.key : 5
   * }
   *
   */

  AnyMap uoci(AnyMap::UNORDERED_MAP_CASEINSENSITIVE_KEYS);
  uoci["FiRST"] = 1;
  uoci["SECOND"] = 2;

  AnyMap uo(AnyMap::UNORDERED_MAP);
  uo["hi"] = std::string("hi");
  uo["there"] = std::string("there");

  std::vector<Any> anyVec{ Any(std::string("one")), Any(2), Any(uo) };
  uoci["vec"] = anyVec;

  om["uoci"] = uoci;
  om["dot.key"] = 5;

  US_TEST_CONDITION(om.AtCompoundKey("key1") == std::string("val1"), "Get key1")
  US_TEST_CONDITION(om.AtCompoundKey("uoci.first") == 1, "Get uoci.first")
  US_TEST_CONDITION(om.AtCompoundKey("uoci.second") == 2, "Get uoci.SECOND")
  US_TEST_CONDITION(om.AtCompoundKey("uoci.Vec.0") == std::string("one"),
                    "Get uoci.Vec.0")
  US_TEST_CONDITION(om.AtCompoundKey("uoci.Vec.2.there") ==
                      std::string("there"),
                    "Get uoci.Vec.2.there")

  std::set<std::string> keys;
  for (auto p : uoci) {
    keys.insert(p.first);
  }
  auto key = keys.begin();
  US_TEST_CONDITION_REQUIRED(keys.size() == 3, "Map key size == 3")
  US_TEST_CONDITION(*key++ == "FiRST", "Keys[0] == FiRST")
  US_TEST_CONDITION(*key++ == "SECOND", "Keys[1] == SECOND")
  US_TEST_CONDITION(*key++ == "vec", "Keys[2] == vec")

  US_TEST_FOR_EXCEPTION_BEGIN(std::out_of_range)
  om.at("Key1");
  US_TEST_FOR_EXCEPTION_END(std::out_of_range)

  US_TEST_FOR_EXCEPTION_BEGIN(std::out_of_range)
  om.AtCompoundKey("dot.key");
  US_TEST_FOR_EXCEPTION_END(std::out_of_range)

  US_TEST_FOR_EXCEPTION_BEGIN(std::invalid_argument)
  uoci.AtCompoundKey("Vec.bla");
  US_TEST_FOR_EXCEPTION_END(std::invalid_argument)

  US_TEST_FOR_EXCEPTION_BEGIN(std::invalid_argument)
  uoci.AtCompoundKey("Vec.1.bla");
  US_TEST_FOR_EXCEPTION_END(std::invalid_argument)

  US_TEST_END()
}
