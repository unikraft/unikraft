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

#include "cppmicroservices/Bundle.h"
#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/Framework.h"
#include "cppmicroservices/FrameworkFactory.h"
#include "cppmicroservices/LDAPFilter.h"
#include "cppmicroservices/LDAPProp.h"
#include "cppmicroservices/ServiceEvent.h"
#include "cppmicroservices/ServiceTracker.h"

#include "gtest/gtest.h"

using namespace cppmicroservices;

TEST(LDAPExprTest, GetMatchedObjectClasses)
{
  // Improve coverage for LDAPExpr::GetMatchedObjectClasses()
  struct MyInterfaceOne
  {
    virtual ~MyInterfaceOne() {}
  };
  struct MyServiceOne : public MyInterfaceOne
  {};

  auto f = FrameworkFactory().NewFramework();
  f.Init();
  BundleContext context{ f.GetBundleContext() };

  auto serviceOne = std::make_shared<MyServiceOne>();
  context.RegisterService<MyInterfaceOne>(serviceOne);

  LDAPFilter filter("(&(objectclass=alpha)(objectclass=beta))");
  ServiceTracker<MyInterfaceOne> tracker(context, filter, nullptr);
  tracker.Open();

  LDAPFilter filter1("(&(objectclass=beta)(objectclass=alpha))");
  ServiceTracker<MyInterfaceOne> tracker1(context, filter1, nullptr);
  tracker1.Open();

  LDAPFilter filter2("(|(objectclass=alpha)(objectclass=beta))");
  ServiceTracker<MyInterfaceOne> tracker2(context, filter2, nullptr);
  tracker2.Open();

  LDAPFilter filter3("(&(objectclass=alpha)(objectclass=alpha))");
  ServiceTracker<MyInterfaceOne> tracker3(context, filter3, nullptr);
  tracker3.Open();

  LDAPFilter filter4("(|(object=alpha)(object=beta))");
  ServiceTracker<MyInterfaceOne> tracker4(context, filter4, nullptr);
  tracker4.Open();

  ASSERT_TRUE(tracker.GetServiceReferences().size() == 0);
}

TEST(LDAPExprTest, IsSimple)
{
  // Expanding coverage for LDAPExpr::IsSimple by testing the OR filter.
  auto f = FrameworkFactory().NewFramework();
  f.Init();
  BundleContext fCtx{ f.GetBundleContext() };

  auto lambda = [](const ServiceEvent&) { std::cout << "ServiceEvent!"; };

  const std::string ldapFilter = "(|(objectClass=foo)(objectClass=bar))";
  ASSERT_TRUE(fCtx.AddServiceListener(lambda, ldapFilter));
  const std::string ldapFilter2 =
    "(|(&(objectClass=foo)(objectClass=bar))(objectClass=baz))";
  ASSERT_TRUE(fCtx.AddServiceListener(lambda, ldapFilter2));
}

TEST(LDAPExprTest, Evaluate)
{
  // Testing previously uncovered lines in LDAPExpr::Evaluate()
  // case NOT
  LDAPFilter ldapMatch("(!(hosed=1))");
  AnyMap props(AnyMap::UNORDERED_MAP);
  props["hosed"] = std::string("2");
  ASSERT_TRUE(ldapMatch.Match(props));

  // case OR returning false
  LDAPFilter ldapMatch2("(|(hosed=1)(hosed=2))");
  props["hosed"] = std::string("3");
  ASSERT_FALSE(ldapMatch2.Match(props));
}

TEST(LDAPExprTest, Compare)
{
  // Testing wildcard
  LDAPFilter ldapMatch("(hosed=*)");
  AnyMap props(AnyMap::UNORDERED_MAP);
  props["hosed"] = 5;
  ASSERT_TRUE(ldapMatch.Match(props));

  // Testing empty
  ldapMatch = LDAPFilter("(hosed=1)");
  props.clear();
  props["hosed"] = Any();
  ASSERT_FALSE(ldapMatch.Match(props));

  // Testing AnyMap value types.
  props.clear();
  props["hosed"] = std::list<std::string>{ "1", "2" };
  ASSERT_TRUE(ldapMatch.Match(props));

  props.clear();
  props["hosed"] = '1';
  ASSERT_TRUE(ldapMatch.Match(props));

  ldapMatch = LDAPFilter("(val<=true)");
  props.clear();
  props["val"] = true;
  ASSERT_FALSE(ldapMatch.Match(props));

  // Testing integral types.
  ldapMatch = LDAPFilter("(hosed=1)");
  props.clear();
  props["hosed"] = static_cast<short>(1);
  ASSERT_TRUE(ldapMatch.Match(props));
  props.clear();
  props["hosed"] = static_cast<long long int>(1);
  ASSERT_TRUE(ldapMatch.Match(props));
  props.clear();
  props["hosed"] = static_cast<unsigned char>(1);
  ASSERT_TRUE(ldapMatch.Match(props));
  props.clear();
  props["hosed"] = static_cast<unsigned short>(1);
  ASSERT_TRUE(ldapMatch.Match(props));
  props.clear();
  props["hosed"] = static_cast<unsigned int>(1);
  ASSERT_TRUE(ldapMatch.Match(props));
  props.clear();
  props["hosed"] = static_cast<unsigned long int>(1);
  ASSERT_TRUE(ldapMatch.Match(props));
  props.clear();
  props["hosed"] = static_cast<unsigned long long int>(1);
  ASSERT_TRUE(ldapMatch.Match(props));
  ldapMatch = LDAPFilter("(hosed<=200)");
  props.clear();
  props["hosed"] = 1;
  ASSERT_TRUE(ldapMatch.Match(props));
  // Test integer overflow
  ldapMatch = LDAPFilter("(hosed=LONG_MAX)");
  props.clear();
  props["hosed"] = 1;
  ASSERT_FALSE(ldapMatch.Match(props));

  // Testing floating point types.
  ldapMatch = LDAPFilter("(hosed=1)");
  props.clear();
  props["hosed"] = static_cast<float>(1.0);
  ASSERT_TRUE(ldapMatch.Match(props));
  // Test floating point overflow
  ldapMatch = LDAPFilter("(hosed=1.18973e+4932zzz)");
  ASSERT_FALSE(ldapMatch.Match(props));
  ldapMatch = LDAPFilter("(hosed>=0.1)");
  ASSERT_TRUE(ldapMatch.Match(props));
  ldapMatch = LDAPFilter("(hosed<=2.0)");
  ASSERT_TRUE(ldapMatch.Match(props));

  props.clear();
  props["hosed"] = static_cast<double>(1.0);
  ldapMatch = LDAPFilter("(hosed=1)");
  ASSERT_TRUE(ldapMatch.Match(props));
  // Test floating point overflow
  ldapMatch = LDAPFilter("(hosed=1.18973e+4932zzz)");
  ASSERT_FALSE(ldapMatch.Match(props));
  ldapMatch = LDAPFilter("(hosed>=0.1)");
  ASSERT_TRUE(ldapMatch.Match(props));
  ldapMatch = LDAPFilter("(hosed<=2.0)");
  ASSERT_TRUE(ldapMatch.Match(props));
}

TEST(LDAPExprTest, CompareString)
{
  // Testing string greater-equal, less-equal and approx. filters.
  LDAPFilter ldapMatch("(name>=abra)");
  AnyMap props(AnyMap::UNORDERED_MAP);
  props["name"] = std::string("cadabra");
  ASSERT_TRUE(ldapMatch.Match(props));

  ldapMatch = LDAPFilter("(name<=oink)");
  ASSERT_TRUE(ldapMatch.Match(props));

  // Also, test LDAPExpr::FixupString
  ldapMatch = LDAPFilter("(name~=micro)");
  props.clear();
  props["name"] = std::string("MICRO");
  ASSERT_TRUE(ldapMatch.Match(props));
}

TEST(LDAPExprTest, PatSubstr)
{
  LDAPFilter ldapMatch("(name=ab*d)");
  AnyMap props(AnyMap::UNORDERED_MAP);
  props["name"] = std::string("ab");
  ASSERT_FALSE(ldapMatch.Match(props));

  ldapMatch = LDAPFilter("(name=abcd)");
  props.clear();
  props["name"] = std::string("ab");
  ASSERT_FALSE(ldapMatch.Match(props));
}

TEST(LDAPExprTest, ParseExceptions)
{
  // Test various exceptions thrown.
  // Test error condition in LDAPExpr::LDAPExpr()
  EXPECT_THROW(LDAPFilter("(name=abra)zxdzx"), std::invalid_argument);
  // Test error condition in LDAPExpr::ParseExpr()
  EXPECT_THROW(LDAPFilter("(!(name=abra)(name=beta))"), std::invalid_argument);
  // Test attribute name empty error condition in
  // LDAPExpr::ParseSimple() and LDAPExpr::ParseState::getAttributeName()
  EXPECT_THROW(LDAPFilter("(=abra)"), std::invalid_argument);
  // Testing undefined operator error condition in LDAPExpr::ParseSimple()
  EXPECT_THROW(LDAPFilter("(name>abra)"), std::invalid_argument);
  // Testing malformed filter error condition in LDAPExpr::ParseSimple()
  EXPECT_THROW(LDAPFilter("(name=abra("), std::invalid_argument);
  // Testing empty filter error condition in LDAPExpr::ParseState::ParseState()
  EXPECT_THROW(LDAPFilter(""), std::invalid_argument);
  // Testing out of range exception in LDAPExpr::ParseState::peek()
  EXPECT_THROW(LDAPFilter("(name=abra"), std::invalid_argument);
  // Testing '\\' case in LDAPExpr::ParseState::getAttributeValue()
  ASSERT_EQ(LDAPFilter("(name=ab\\a)"), LDAPFilter("(name=aba)"));
}

TEST(LDAPExprTest, BitWiseOperatorOr)
{
  LDAPPropExpr checkedExpr((LDAPProp("key1") == "value1") ||
                           (LDAPProp("key2") == "value2"));

  LDAPPropExpr expr(LDAPProp("key1") == "value1");

  expr |= LDAPProp("key2") == "value2";
  ASSERT_EQ(expr.operator std::string(), checkedExpr.operator std::string());
}

TEST(LDAPExprTest, BitWiseOperatorAnd)
{
  LDAPPropExpr checkedExpr((LDAPProp("key1") == "value1") &&
                           (LDAPProp("key2") == "value2"));

  LDAPPropExpr expr(LDAPProp("key1") == "value1");

  expr &= LDAPProp("key2") == "value2";

  ASSERT_EQ(expr.operator std::string(), checkedExpr.operator std::string());
}

TEST(LDAPExprTest, OperatorAssignment)
{
  LDAPPropExpr checkedExpr((LDAPProp("key1") == "value1") ||
                           (LDAPProp("key2") == "value2"));
  LDAPPropExpr expr(LDAPProp("key1") == "value1");

  expr = expr || LDAPProp("key2") == "value2";

  ASSERT_EQ(expr.operator std::string(), checkedExpr.operator std::string());
}

TEST(LDAPExprTest, AssignToDefaultConstructed)
{
  LDAPPropExpr checkedExpr((LDAPProp("key2") == "value2"));

  LDAPPropExpr defaultConstructed;
  ASSERT_TRUE(defaultConstructed.IsNull());

  defaultConstructed |= LDAPProp("key2") == "value2";
  ASSERT_EQ(defaultConstructed.operator std::string(),
            checkedExpr.operator std::string());

  LDAPPropExpr expr(LDAPProp("key2") == "value2");
  expr |= LDAPPropExpr();
  ASSERT_EQ(expr.operator std::string(), checkedExpr.operator std::string());
}
