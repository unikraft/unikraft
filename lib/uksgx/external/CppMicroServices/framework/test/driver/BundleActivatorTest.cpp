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
#include "cppmicroservices/Constants.h"
#include "cppmicroservices/Framework.h"
#include "cppmicroservices/FrameworkEvent.h"
#include "cppmicroservices/FrameworkFactory.h"

#include "TestUtils.h"
#include "TestingConfig.h"
#include "TestingMacros.h"

using namespace cppmicroservices;

// bundle.activator property not specified in the manifest and bundle has no activator class
void TestNoPropertyNoClass(BundleContext& context)
{
  auto bundle = testing::InstallLib(context, "TestBundleR");
  US_TEST_CONDITION_REQUIRED(bundle, "Test for existing bundle TestBundleR");
  US_TEST_NO_EXCEPTION(bundle.Start());
}

// bundle.activator property not specified in the manifest and bundle has an activator class
void TestNoPropertyWithClass(BundleContext& context)
{
  auto bundle = testing::InstallLib(context, "TestBundleBA_X1");
  US_TEST_CONDITION_REQUIRED(bundle,
                             "Test for existing bundle TestBundleBA_X1");
  US_TEST_NO_EXCEPTION(bundle.Start());
  // verify bundle activator was not called => service not registered
  ServiceReferenceU ref = bundle.GetBundleContext().GetServiceReference(
    "cppmicroservices::TestBundleBA_X1Service");
  US_TEST_CONDITION(!ref, "Invalid reference");
}

// bundle.activator property set with wrong type in the manifest and bundle has an activator class
void TestWrongPropertyTypeWithClass(BundleContext& context)
{
  auto bundle = testing::InstallLib(context, "TestBundleBA_S1");
  US_TEST_CONDITION_REQUIRED(bundle,
                             "Test for existing bundle TestBundleBA_S1");
  // Add a framework listener and verify the FrameworkEvent
  bool receivedExpectedEvent = false;
  const FrameworkListener fl = [&](const FrameworkEvent& evt) {
    std::exception_ptr eptr = evt.GetThrowable();
    if ((evt.GetType() == FrameworkEvent::FRAMEWORK_WARNING) &&
        (eptr != nullptr)) {
      try {
        std::rethrow_exception(eptr);
      } catch (const BadAnyCastException& /*ex*/) {
        receivedExpectedEvent = true;
      }
    }
  };
  context.AddFrameworkListener(fl);
  US_TEST_NO_EXCEPTION(bundle.Start());
  US_TEST_CONDITION(receivedExpectedEvent == true, "Test for FrameworkEvent");
  context.RemoveFrameworkListener(fl);
  // verify bundle activator was not called => service not registered
  ServiceReferenceU ref = bundle.GetBundleContext().GetServiceReference(
    "cppmicroservices::TestBundleBA_S1Service");
  US_TEST_CONDITION(!ref, "Invalid reference");
}

// bundle.activator property set to false in the manifest and bundle has no activator class
void TestPropertyFalseWithoutClass(BundleContext& context)
{
  auto bundle = testing::InstallLib(context, "TestBundleBA_00");
  US_TEST_CONDITION_REQUIRED(bundle,
                             "Test for existing bundle TestBundleBA_00");
  US_TEST_NO_EXCEPTION(bundle.Start());
}

// bundle.activator property set to false in the manifest and bundle has an activator class
void TestPropertyFalseWithClass(BundleContext& context)
{
  auto bundle = testing::InstallLib(context, "TestBundleBA_01");
  US_TEST_CONDITION_REQUIRED(bundle,
                             "Test for existing bundle TestBundleBA_01");
  US_TEST_NO_EXCEPTION(bundle.Start());
  // verify bundle activator was not called => service not registered
  ServiceReferenceU ref = bundle.GetBundleContext().GetServiceReference(
    "cppmicroservices::TestBundleBA_01Service");
  US_TEST_CONDITION(!ref, "Invalid reference");
}

// bundle.activator property set to true in the manifest and bundle has no activator class
void TestPropertyTrueWithoutClass(BundleContext& context)
{
  auto bundle = testing::InstallLib(context, "TestBundleBA_10");
  US_TEST_CONDITION_REQUIRED(bundle,
                             "Test for existing bundle TestBundleBA_10");
  US_TEST_FOR_EXCEPTION(std::runtime_error, bundle.Start());
}

// bundle.activator property set to true in the manifest and bundle has an activator class
void TestPropertyTrueWithClass(BundleContext& context)
{
  auto bundle = testing::InstallLib(context, "TestBundleA");
  US_TEST_CONDITION_REQUIRED(bundle, "Test for existing bundle TestBundleA");
  US_TEST_NO_EXCEPTION(bundle.Start());
  // verify bundle activator was called => service is registered
  ServiceReferenceU ref = bundle.GetBundleContext().GetServiceReference(
    "cppmicroservices::TestBundleAService");
  US_TEST_CONDITION(ref, "Valid reference");
}

int BundleActivatorTest(int /*argc*/, char* /*argv*/ [])
{
  US_TEST_BEGIN("BundleActivatorTest");

  auto framework = FrameworkFactory().NewFramework();
  framework.Start();
  auto bc = framework.GetBundleContext();

  // Test points to validate Bundle behavior based on bundle.activator property
  TestNoPropertyNoClass(bc);
  TestNoPropertyWithClass(bc);
  TestWrongPropertyTypeWithClass(bc);
  TestPropertyFalseWithoutClass(bc);
  TestPropertyFalseWithClass(bc);
  TestPropertyTrueWithoutClass(bc);
  TestPropertyTrueWithClass(bc);

  framework.Stop();
  framework.WaitForStop(std::chrono::milliseconds::zero());

  US_TEST_END()
}
