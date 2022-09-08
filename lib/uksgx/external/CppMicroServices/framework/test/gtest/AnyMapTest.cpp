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
#include "cppmicroservices/GlobalConfig.h"

#include "gtest/gtest.h"

using namespace cppmicroservices;

TEST(AnyMapTest, CheckExceptions)
{
  // Testing throw of invalid_argument from the free function
  // AtCompoundKey(const AnyMap& m, const AnyMap::key_type& key)
  AnyMap uo(AnyMap::UNORDERED_MAP);
  uo["hi"] = std::string("hi");
  EXPECT_THROW(uo.AtCompoundKey("hi."), std::invalid_argument);
}

TEST(AnyMapTest, AtCompoundKey)
{
  // Testing nested vector<Any> compound access
  AnyMap uo(AnyMap::UNORDERED_MAP);
  std::vector<Any> child{ Any(1), Any(2) };
  std::vector<Any> parent{ Any(child) };
  uo["hi"] = parent;
  ASSERT_EQ(uo.AtCompoundKey("hi.0.0"), 1);
}

TEST(AnyMapTest, IteratorTest)
{
  AnyMap o(AnyMap::ORDERED_MAP);
  o["a"] = 1;
  o["b"] = 2;
  o["c"] = 3;
  AnyMap::const_iter ociter(o.begin());
  AnyMap::const_iter ociter1(o.cbegin());

  AnyMap uo(AnyMap::UNORDERED_MAP);
  uo["1"] = 1;
  uo["2"] = 2;
  AnyMap::const_iter uociter(uo.begin());
  AnyMap::const_iter uociter1(uo.cbegin());

  AnyMap uoci(AnyMap::UNORDERED_MAP_CASEINSENSITIVE_KEYS);
  uoci["do"] = 1;
  uoci["re"] = 2;
  AnyMap::const_iter uoccciiter(uoci.begin());
  AnyMap::const_iter uoccciiter1(uoci.cbegin());

  AnyMap::const_iter ociter_temp(AnyMap(AnyMap::ORDERED_MAP).cbegin());

  // Testing deref and increment operators
  ASSERT_EQ((*ociter1).second.ToString(), std::string("1"));
  ASSERT_EQ((*(++ociter1)).second.ToString(), std::string("2"));
  ASSERT_EQ((*(ociter1++)).second.ToString(), std::string("2"));
  int i = 0;
  for (AnyMap::const_iter oc_it = o.cbegin(); oc_it != o.cend(); ++oc_it) {
    ++i;
    ASSERT_EQ(i, any_cast<int>(oc_it->second));
  }

  // Testing exception when an invalid iterator is dereferenced.
  AnyMap::const_iter nciter, nciter2;
  EXPECT_THROW(*nciter, std::logic_error);
  EXPECT_THROW(++nciter, std::logic_error);
  EXPECT_THROW(nciter++, std::logic_error);
  EXPECT_THROW(US_UNUSED(nciter->second), std::logic_error);
  ASSERT_EQ(nciter, nciter2);

  // Testing ++ operator
  ASSERT_TRUE(any_cast<int>((*(uociter++)).second) > 0);
  ASSERT_TRUE(any_cast<int>(uociter->second) > 0);
  ASSERT_TRUE(any_cast<int>((*(uoccciiter++)).second) > 0);

  // Testing operator==
  auto ociter2(o.cbegin());
  ASSERT_EQ(ociter, ociter2);

  // Testing iterator copy ctor.
  AnyMap::iter oiter(o.begin());
  AnyMap::iter oiter_copy(o.begin());
  AnyMap::iter uoiter(uo.begin());
  AnyMap::iter uoiter_copy(uoiter);
  AnyMap::iter uociiter(uoci.begin());
  AnyMap::iter niter;
  AnyMap::iter niter_copy(niter);
  AnyMap::const_iter nciter3(niter);

  // Testing iterator equality operator
  ASSERT_EQ(oiter, oiter_copy);
  ASSERT_EQ(uoiter, uoiter_copy);
  ASSERT_EQ(niter, niter_copy);

  // Testing iterator deref operator
  ASSERT_EQ((*oiter).second.ToString(), std::string("1"));
  ASSERT_TRUE(any_cast<int>((*uoiter).second) > 0);
  EXPECT_THROW(*niter, std::logic_error);

  // Testing iterator arrow operator
  ASSERT_TRUE(any_cast<int>(uoiter->second) > 0);
  ASSERT_TRUE(any_cast<int>(uociiter->second) > 0);
  EXPECT_THROW(US_UNUSED(niter->second), std::logic_error);

  // Testing iterator pre-increment operator
  ASSERT_EQ((*(++oiter)).second.ToString(), std::string("2"));
  ASSERT_TRUE(any_cast<int>((*(++uoiter)).second) > 0);
  EXPECT_THROW(++niter, std::logic_error);

  // Testing iterator post-increment operator
  ASSERT_EQ((*(oiter++)).second.ToString(), std::string("2"));
  ASSERT_TRUE(any_cast<int>((*(uoiter++)).second) > 0);
  ASSERT_TRUE(any_cast<int>((uociiter++)->second) > 0);
  EXPECT_THROW(niter++, std::logic_error);
}

TEST(AnyMapTest, AnyMap)
{
  AnyMap::ordered_any_map o;
  o["do"] = Any(1);
  o["re"] = Any(2);
  AnyMap o_anymap(o);
  AnyMap o_anymap_copy(o_anymap);

  AnyMap uo_anymap(AnyMap::UNORDERED_MAP);
  uo_anymap["do"] = 1;
  uo_anymap["re"] = 2;

  AnyMap::unordered_any_cimap uco;
  AnyMap uco_anymap(uco);

  AnyMap o_anymap1(AnyMap::ORDERED_MAP);
  o_anymap1 = o_anymap1;
  o_anymap1 = o_anymap_copy;
  AnyMap uo_anymap1(AnyMap::UNORDERED_MAP);
  uo_anymap1 = uo_anymap;
  AnyMap uco_anymap1(AnyMap::UNORDERED_MAP_CASEINSENSITIVE_KEYS);
  uco_anymap1 = uco_anymap;

  // Testing AnyMap::empty()
  ASSERT_TRUE(uco_anymap1.empty());

  // Testing AnyMap::clear()
  AnyMap o_anymap2(o_anymap);
  o_anymap2.clear();
  uco_anymap1.clear();
  uco_anymap1["DO"] = 1;
  uco_anymap1["RE"] = 2;

  // Testing AnyMap::at()
  ASSERT_EQ(any_cast<int>(uo_anymap1.at("re")), 2);

  // Testing AnyMap::operator[] (const)
  const std::string key = "re";
  o_anymap1[key] = 10;
  uo_anymap1[key] = 10;
  uo_anymap1.insert(std::make_pair(std::string("mi"), Any(3)));
  uco_anymap1[key] = 10;

  // Testing AnyMap::find()
  ASSERT_TRUE(o_anymap1.find("re") != o_anymap1.end());
  ASSERT_TRUE(uo_anymap1.find("re") != uo_anymap1.end());

  // Testing AnyMap::GetType()
  ASSERT_EQ(uco_anymap1.GetType(), AnyMap::UNORDERED_MAP_CASEINSENSITIVE_KEYS);

  // Testing any_value_to_* free functions
  std::ostringstream stream1, stream2;
  any_value_to_string(stream1, o_anymap);
  ASSERT_EQ(stream1.str(), "{do : 1, re : 2}");
  any_value_to_json(stream2, o_anymap1);
  ASSERT_EQ(stream2.str(), "{\"do\" : 1, \"re\" : 10}");
}
