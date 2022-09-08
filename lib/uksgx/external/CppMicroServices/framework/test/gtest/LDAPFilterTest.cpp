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

#include "cppmicroservices/LDAPFilter.h"
#include "cppmicroservices/LDAPProp.h"

#include "cppmicroservices/ServiceReference.h"

#include "gtest/gtest.h"

using namespace cppmicroservices;

TEST(LDAPFilter, ToString)
{
  LDAPFilter filter;
  ASSERT_NO_THROW(filter.ToString());
  ASSERT_NO_THROW(std::cout << "Empty LDAPFilter: " << filter << std::endl;);
}

TEST(LDAPFilter, BooleanOperator)
{
  LDAPFilter emptyFilter;
  ASSERT_FALSE(emptyFilter);

  LDAPFilter validFilter("(cn = Babs Jensen)");
  ASSERT_TRUE(validFilter);
}

TEST(LDAPFilter, Comparison)
{
  LDAPFilter filter1, filter2;
  ASSERT_TRUE(filter1 == filter2);

  LDAPFilter filt("(cn = Babs Jensen)");
  ASSERT_FALSE(filter1 == filt);
}

TEST(LDAPFilter, Equality)
{
  LDAPFilter ldap("(prod=CppMiroServices)");
  LDAPFilter ldap_alt("(prod=CppMiroServices)");
  ASSERT_EQ(ldap, ldap_alt);
}

TEST(LDAPFilter, LDAPProp)
{
  // Testing LDAPProp's operators.
  Any any1 = std::string("hello");
  Any any2 = std::string("bye");
  Any any3 = std::string("Ballpark");
  Any anyInt1 = 30;
  Any anyInt2 = 50;
  LDAPFilter filter(LDAPProp("bla") != "jo" && LDAPProp("foo") == any1 &&
                    LDAPProp("bar") != any2 && LDAPProp("baz") >= anyInt1 &&
                    LDAPProp("bleh") <= anyInt2 &&
                    LDAPProp("doh").Approx(any3));
  const std::string filterStr = "(&(&(&(&(&(!(bla=jo))(foo=hello))(!(bar=bye)))"
                                "(baz>=30))(bleh<=50))(doh~=Ballpark))";
  ASSERT_EQ(filter.ToString(), filterStr);
}

TEST(LDAPFilter, DefaultConstructedMatch)
{
  LDAPFilter filter;
  ASSERT_NO_THROW(filter.Match(AnyMap(any_map::map_type::ORDERED_MAP)));
  ASSERT_NO_THROW(filter.Match(ServiceReferenceU()));
  ASSERT_NO_THROW(filter.Match(Bundle()));
  ASSERT_NO_THROW(filter.MatchCase(AnyMap(any_map::map_type::ORDERED_MAP)));
}
