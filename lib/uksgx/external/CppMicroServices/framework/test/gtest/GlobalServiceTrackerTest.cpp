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

#include "cppmicroservices/GlobalConfig.h"

#include "cppmicroservices/Framework.h"
#include "cppmicroservices/FrameworkEvent.h"
#include "cppmicroservices/FrameworkFactory.h"
#include "cppmicroservices/ServiceTracker.h"
#include <cppmicroservices/ServiceInterface.h>

#include "gtest/gtest.h"

#include <memory>

// A dummy interface to use with service trackers
namespace {
class Foo
{
public:
  virtual ~Foo() {}
};
}

// Since the Foo interface is embedded in the test executable, its symbols are
// not exported. Using CPPMICROSERVICES_DECLARE_SERVICE_INTERFACE ensures that
// the symbols are exported correctly for use by CppMicroServices.
CPPMICROSERVICES_DECLARE_SERVICE_INTERFACE(
  Foo,
  "org.cppmicroservices.test.servicetracker.Foo");

TEST(GlobalServiceTrackerTest, Destroy)
{
  auto f = cppmicroservices::FrameworkFactory().NewFramework();
  ASSERT_TRUE(f);
  f.Start();

  static std::shared_ptr<cppmicroservices::ServiceTracker<Foo>> globalTracker(
    std::make_shared<cppmicroservices::ServiceTracker<Foo>>(
      f.GetBundleContext()));
  globalTracker->Open();

  f.Stop();
  f.WaitForStop(std::chrono::milliseconds::zero());
  // A test failure results in the executable crashing with an access violation.
}
