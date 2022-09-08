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

#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/Framework.h"
#include "cppmicroservices/FrameworkEvent.h"
#include "cppmicroservices/FrameworkFactory.h"
#include "cppmicroservices/ServiceObjects.h"
#include "gtest/gtest.h"

using namespace cppmicroservices;

namespace ServiceNS {

struct ITestServiceA
{
  virtual int getValue() const = 0;
  virtual ~ITestServiceA() {}
};

struct ITestServiceB
{
  virtual int getValue() const = 0;
  virtual ~ITestServiceB() {}
};
}

// This test exercises the 2 ways to register a service
//   a. using the name of the interface i.e. "Foo::Bar"
//   b. using the type of the interface i.e. <Foo::Bar>
// multiplied with 2 ways to get the service reference
//   a. using the name of the interface
//   b. using the type of the interface
// NOTE: We do have tests for this in different places,
// but this test consolidates all these modes together.
TEST(ServiceReferenceTest, TestRegisterAndGetServiceReferenceTest)
{
  struct TestServiceA : public ServiceNS::ITestServiceA
  {
    int getValue() const { return 42; }
  };

  struct TestServiceB : public ServiceNS::ITestServiceB
  {
    int getValue() const { return 1729; }
  };

  FrameworkFactory factory;
  auto framework = factory.NewFramework();
  framework.Start();
  auto context = framework.GetBundleContext();

  auto impl = std::make_shared<TestServiceA>();
  (void)context.RegisterService<ServiceNS::ITestServiceA>(impl);

  auto sr1 = context.GetServiceReference<ServiceNS::ITestServiceA>();
  ASSERT_EQ(sr1.GetInterfaceId(), "ServiceNS::ITestServiceA");
  auto service1 = context.GetService(sr1);
  ASSERT_EQ(service1->getValue(), 42);

  auto sr2 = context.GetServiceReference("ServiceNS::ITestServiceA");
  ASSERT_EQ(sr2.GetInterfaceId(), "ServiceNS::ITestServiceA");
  auto interfacemap2 = context.GetService(sr2);
  auto service_void2 = interfacemap2->at("ServiceNS::ITestServiceA");
  auto service2 =
    std::static_pointer_cast<ServiceNS::ITestServiceA>(service_void2);
  ASSERT_EQ(service2->getValue(), 42);

  InterfaceMap im;
  im["ServiceNS::ITestServiceB"] = std::make_shared<TestServiceB>();
  (void)context.RegisterService(std::make_shared<const InterfaceMap>(im));

  auto sr3 = context.GetServiceReference("ServiceNS::ITestServiceB");
  ASSERT_EQ(sr3.GetInterfaceId(), "ServiceNS::ITestServiceB");
  auto interfacemap3 = context.GetService(sr3);
  auto service_void3 = interfacemap3->at("ServiceNS::ITestServiceB");
  auto service3 =
    std::static_pointer_cast<ServiceNS::ITestServiceB>(service_void3);
  ASSERT_EQ(service3->getValue(), 1729);

  auto sr4 = context.GetServiceReference<ServiceNS::ITestServiceB>();
  ASSERT_EQ(sr4.GetInterfaceId(), "ServiceNS::ITestServiceB");
  auto service4 = context.GetService(sr4);
  ASSERT_EQ(service4->getValue(), 1729);

  framework.Stop();
  framework.WaitForStop(std::chrono::milliseconds::zero());
}
