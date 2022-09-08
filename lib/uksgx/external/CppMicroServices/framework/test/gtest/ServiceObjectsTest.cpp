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

#include "cppmicroservices/ServiceObjects.h"
#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/Framework.h"
#include "cppmicroservices/FrameworkFactory.h"
#include "gtest/gtest.h"

using namespace cppmicroservices;

struct ITestServiceA
{
  virtual ~ITestServiceA() {}
};

TEST(ServiceObjectsTest, TestServiceObjects)
{
  struct TestServiceA : public ITestServiceA
  {};

  FrameworkFactory factory;
  auto framework = factory.NewFramework();
  framework.Start();
  auto context = framework.GetBundleContext();

  auto s1 = std::make_shared<TestServiceA>();
  ServiceRegistration<ITestServiceA> reg1 =
    context.RegisterService<ITestServiceA>(s1);
  auto ref = context.GetServiceReference("ITestServiceA");
  ServiceObjects<void> serviceObject = context.GetServiceObjects(ref);
  ASSERT_TRUE(serviceObject.GetService() != nullptr);

  auto invalid_ref = context.GetServiceReference("InvalidService");
  ASSERT_THROW(context.GetServiceObjects(invalid_ref), std::invalid_argument);

  auto ref_from_so = serviceObject.GetServiceReference();
  ASSERT_TRUE(ref_from_so);

  // Move ctor and assignment
  ServiceObjects<void> serviceObjMove(std::move(serviceObject));
  ASSERT_TRUE(serviceObjMove.GetServiceReference());
  serviceObjMove = context.GetServiceObjects(ref_from_so);
  ASSERT_TRUE(serviceObjMove.GetServiceReference());

  // verify GetService returns null after service unregistration
  reg1.Unregister();
  ASSERT_TRUE(serviceObjMove.GetService() == nullptr);
}
