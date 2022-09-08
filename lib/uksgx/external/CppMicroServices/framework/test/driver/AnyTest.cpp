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

#include "cppmicroservices/Any.h"

#include "TestingMacros.h"

#include <limits>
#include <stdexcept>

using namespace cppmicroservices;

template<typename T>
void TestUnsafeAnyCast(Any& anyObj, T val)
{
  T* valPtr = unsafe_any_cast<T>(&anyObj);
  US_TEST_CONDITION(valPtr != nullptr, "unsafe_any_cast");
  US_TEST_CONDITION(*(valPtr) == val,
                    "compare returned value from unsafe_any_cast");
}

int AnyTest(int /*argc*/, char* /*argv*/ [])
{
  US_TEST_BEGIN("AnyTest");

  Any anyEmpty;
  US_TEST_FOR_EXCEPTION(std::logic_error, anyEmpty.ToString())
  US_TEST_NO_EXCEPTION(anyEmpty.ToStringNoExcept())

  Any anyBool = true;
  US_TEST_CONDITION(anyBool.Type() == typeid(bool), "Any[bool].Type()")
  US_TEST_CONDITION(any_cast<bool>(anyBool) == true, "any_cast<bool>()")
  US_TEST_CONDITION(anyBool.ToString() == "1", "Any[bool].ToString()")
  US_TEST_CONDITION(anyBool.ToJSON() == "true", "Any[bool].ToJSON()")
  TestUnsafeAnyCast<bool>(anyBool, true);
  anyBool = false;
  US_TEST_CONDITION(anyBool.ToString() == "0", "Any[bool].ToString()")
  US_TEST_CONDITION(anyBool.ToJSON() == "false", "Any[bool].ToJSON()")
  TestUnsafeAnyCast<bool>(anyBool, false);

  Any anyInt = 13;
  US_TEST_CONDITION(anyInt.Type() == typeid(int), "Any[int].Type()")
  US_TEST_CONDITION(any_cast<int>(anyInt) == 13, "any_cast<int>()")
  US_TEST_CONDITION(anyInt.ToString() == "13", "Any[int].ToString()")
  US_TEST_CONDITION(anyInt.ToJSON() == "13", "Any[int].ToJSON()")
  TestUnsafeAnyCast<int>(anyInt, 13);

  Any anyChar = 'a';
  US_TEST_CONDITION(anyChar.Type() == typeid(char), "Any[char].Type()")
  US_TEST_CONDITION(any_cast<char>(anyChar) == 'a', "any_cast<char>()")
  US_TEST_CONDITION(anyChar.ToString() == "a", "Any[char].ToString()")
  US_TEST_CONDITION(anyChar.ToJSON() == "a", "Any[char].ToJSON()")
  TestUnsafeAnyCast<char>(anyChar, 'a');

  Any anyFloat = 0.2f;
  US_TEST_CONDITION(anyFloat.Type() == typeid(float), "Any[float].Type()")
  US_TEST_CONDITION(any_cast<float>(anyFloat) - 0.2f <
                      std::numeric_limits<float>::epsilon(),
                    "any_cast<float>()")
  US_TEST_CONDITION(anyFloat.ToString() == "0.2", "Any[float].ToString()")
  US_TEST_CONDITION(anyFloat.ToString() == "0.2", "Any[float].ToJSON()")
  TestUnsafeAnyCast<float>(anyFloat, 0.2f);

  Any anyDouble = 0.5;
  US_TEST_CONDITION(anyDouble.Type() == typeid(double), "Any[double].Type()")
  US_TEST_CONDITION(any_cast<double>(anyDouble) - 0.5 <
                      std::numeric_limits<double>::epsilon(),
                    "any_cast<double>()")
  US_TEST_CONDITION(anyDouble.ToString() == "0.5", "Any[double].ToString()")
  US_TEST_CONDITION(anyDouble.ToString() == "0.5", "Any[double].ToJSON()")
  TestUnsafeAnyCast<double>(anyDouble, 0.5);

  Any anyString = std::string("hello");
  US_TEST_CONDITION(anyString.Type() == typeid(std::string),
                    "Any[std::string].Type()")
  US_TEST_CONDITION(any_cast<std::string>(anyString) == "hello",
                    "any_cast<std::string>()")
  US_TEST_CONDITION(anyString.ToString() == "hello",
                    "Any[std::string].ToString()")
  US_TEST_CONDITION(anyString.ToJSON() == "\"hello\"",
                    "Any[std::string].ToJSON()")
  TestUnsafeAnyCast<std::string>(anyString, std::string("hello"));

  std::vector<int> vecInts;
  vecInts.push_back(1);
  vecInts.push_back(2);
  Any anyVectorOfInts = vecInts;
  US_TEST_CONDITION(anyVectorOfInts.Type() == typeid(std::vector<int>),
                    "Any[std::vector<int>].Type()")
  US_TEST_CONDITION(any_cast<std::vector<int>>(anyVectorOfInts) == vecInts,
                    "any_cast<std::vector<int>>()")
  US_TEST_CONDITION(anyVectorOfInts.ToString() == "[1,2]",
                    "Any[std::vector<int>].ToString()")
  US_TEST_CONDITION(anyVectorOfInts.ToJSON() == "[1,2]",
                    "Any[std::vector<int>].ToJSON()")

  std::list<int> listInts;
  listInts.push_back(1);
  listInts.push_back(2);
  Any anyListOfInts = listInts;
  US_TEST_CONDITION(anyListOfInts.Type() == typeid(std::list<int>),
                    "Any[std::list<int>].Type()")
  US_TEST_CONDITION(any_cast<std::list<int>>(anyListOfInts) == listInts,
                    "any_cast<std::list<int>>()")
  US_TEST_CONDITION(anyListOfInts.ToString() == "[1,2]",
                    "Any[std::list<int>].ToString()")
  US_TEST_CONDITION(anyListOfInts.ToJSON() == "[1,2]",
                    "Any[std::list<int>].ToJSON()")

  std::set<int> setInts;
  setInts.insert(1);
  setInts.insert(2);
  Any anySetOfInts = setInts;
  US_TEST_CONDITION(anySetOfInts.Type() == typeid(std::set<int>),
                    "Any[std::set<int>].Type()")
  US_TEST_CONDITION(any_cast<std::set<int>>(anySetOfInts) == setInts,
                    "any_cast<std::set<int>>()")
  US_TEST_CONDITION(anySetOfInts.ToString() == "[1,2]",
                    "Any[std::set<int>].ToString()")
  US_TEST_CONDITION(anySetOfInts.ToJSON() == "[1,2]",
                    "Any[std::set<int>].ToJSON()")

  std::vector<Any> vecAny;
  vecAny.push_back(1);
  vecAny.push_back(std::string("hello"));
  Any anyVectorOfAnys = vecAny;
  US_TEST_CONDITION(anyVectorOfAnys.Type() == typeid(std::vector<Any>),
                    "Any[std::vector<Any>].Type()")
  US_TEST_CONDITION(anyVectorOfAnys.ToString() == "[1,hello]",
                    "Any[std::vector<Any>].ToString()")
  US_TEST_CONDITION(anyVectorOfAnys.ToJSON() == "[1,\"hello\"]",
                    "Any[std::vector<Any>].ToJSON()")

  std::list<Any> listAny;
  listAny.push_back(1);
  listAny.push_back(std::string("hello"));
  Any anyListOfAnys = listAny;
  US_TEST_CONDITION(anyListOfAnys.Type() == typeid(std::list<Any>),
                    "Any[std::list<Any>].Type()")
  US_TEST_CONDITION(anyListOfAnys.ToString() == "[1,hello]",
                    "Any[std::list<Any>].ToString()")
  US_TEST_CONDITION(anyListOfAnys.ToJSON() == "[1,\"hello\"]",
                    "Any[std::list<Any>].ToJSON()")

  std::map<std::string, int> map1;
  map1["one"] = 1;
  map1["two"] = 2;
  Any anyMap1 = map1;
  US_TEST_CONDITION(anyMap1.Type() == typeid(std::map<std::string, int>),
                    "Any[std::map<std::string,int>].Type()")
  US_TEST_CONDITION((any_cast<std::map<std::string, int>>(anyMap1) == map1),
                    "any_cast<std::map<std::string,int>>()")
  US_TEST_CONDITION(anyMap1.ToString() == "{one : 1, two : 2}",
                    "Any[std::map<std::string,int>].ToString()")
  US_TEST_CONDITION(anyMap1.ToJSON() == "{\"one\" : 1, \"two\" : 2}",
                    "Any[std::map<std::string,int>].ToJSON()")

  std::map<int, Any> map2;
  map2[1] = 0.3;
  map2[3] = std::string("bye");
  Any anyMap2 = map2;
  US_TEST_CONDITION(anyMap2.Type() == typeid(std::map<int, Any>),
                    "Any[std::map<int,Any>].Type()")
  US_TEST_CONDITION(anyMap2.ToString() == "{1 : 0.3, 3 : bye}",
                    "Any[std::map<int,Any>].ToString()")
  US_TEST_CONDITION(anyMap2.ToJSON() == "{\"1\" : 0.3, \"3\" : \"bye\"}",
                    "Any[std::map<int,Any>].ToJSON()")

  std::map<std::string, Any> map3;
  map3["number"] = 5;
  std::vector<int> numbers;
  numbers.push_back(9);
  numbers.push_back(8);
  numbers.push_back(7);
  map3["vector"] = numbers;
  map3["map"] = map2;
  Any anyMap3 = map3;
  US_TEST_CONDITION(anyMap3.Type() == typeid(std::map<std::string, Any>),
                    "Any[std::map<std::string,Any>].Type()")
  US_TEST_CONDITION(
    anyMap3.ToString() ==
      "{map : {1 : 0.3, 3 : bye}, number : 5, vector : [9,8,7]}",
    "Any[std::map<std::string,Any>].ToString()")
  US_TEST_CONDITION(anyMap3.ToJSON() ==
                      "{\"map\" : {\"1\" : 0.3, \"3\" : \"bye\"}, \"number\" : "
                      "5, \"vector\" : [9,8,7]}",
                    "Any[std::map<std::string,Any>].ToJSON()")

  // Test BadAnyCastException exceptions
  const Any uncastableConstAny(0.0);
  Any uncastableAny(0.0);

  US_TEST_FOR_EXCEPTION(cppmicroservices::BadAnyCastException,
                        any_cast<std::string>(uncastableConstAny))

  try {
    (void)any_cast<std::string>(uncastableConstAny);
  } catch (const cppmicroservices::BadAnyCastException& ex) {
    US_TEST_OUTPUT(<< ex.what())
  }

  US_TEST_FOR_EXCEPTION(cppmicroservices::BadAnyCastException,
                        any_cast<std::string>(uncastableAny))

  try {
    (void)any_cast<std::string>(uncastableAny);
  } catch (const cppmicroservices::BadAnyCastException& ex) {
    US_TEST_OUTPUT(<< ex.what())
  }

  US_TEST_FOR_EXCEPTION(cppmicroservices::BadAnyCastException,
                        ref_any_cast<std::string>(uncastableConstAny))

  try {
    (void)ref_any_cast<std::string>(uncastableConstAny);
  } catch (const cppmicroservices::BadAnyCastException& ex) {
    US_TEST_OUTPUT(<< ex.what())
  }

  US_TEST_FOR_EXCEPTION(cppmicroservices::BadAnyCastException,
                        ref_any_cast<std::string>(uncastableAny))

  try {
    (void)ref_any_cast<std::string>(uncastableAny);
  } catch (const cppmicroservices::BadAnyCastException& ex) {
    US_TEST_OUTPUT(<< ex.what())
  }

  US_TEST_END()
}
