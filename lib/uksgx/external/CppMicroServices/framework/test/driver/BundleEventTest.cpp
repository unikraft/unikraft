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

#include "cppmicroservices/BundleEvent.h"
#include "cppmicroservices/Framework.h"
#include "cppmicroservices/FrameworkFactory.h"

#include "TestUtils.h"
#include "TestingConfig.h"
#include "TestingMacros.h"

#include <iostream>

using namespace cppmicroservices;

int BundleEventTest(int /*argc*/, char* /*argv*/ [])
{
  US_TEST_BEGIN("BundleEventTest");

  // The OSGi spec assigns specific values to event types for future extensibility.
  // Ensure we don't deviate from those assigned values.
  US_TEST_CONDITION_REQUIRED(BundleEvent::Type::BUNDLE_INSTALLED ==
                               static_cast<BundleEvent::Type>(1),
                             "Test assigned event type values");
  US_TEST_CONDITION_REQUIRED(BundleEvent::Type::BUNDLE_STARTED ==
                               static_cast<BundleEvent::Type>(2),
                             "Test assigned event type values");
  US_TEST_CONDITION_REQUIRED(BundleEvent::Type::BUNDLE_STOPPED ==
                               static_cast<BundleEvent::Type>(4),
                             "Test assigned event type values");
  US_TEST_CONDITION_REQUIRED(BundleEvent::Type::BUNDLE_UPDATED ==
                               static_cast<BundleEvent::Type>(8),
                             "Test assigned event type values");
  US_TEST_CONDITION_REQUIRED(BundleEvent::Type::BUNDLE_UNINSTALLED ==
                               static_cast<BundleEvent::Type>(16),
                             "Test assigned event type values");
  US_TEST_CONDITION_REQUIRED(BundleEvent::Type::BUNDLE_RESOLVED ==
                               static_cast<BundleEvent::Type>(32),
                             "Test assigned event type values");
  US_TEST_CONDITION_REQUIRED(BundleEvent::Type::BUNDLE_UNRESOLVED ==
                               static_cast<BundleEvent::Type>(64),
                             "Test assigned event type values");
  US_TEST_CONDITION_REQUIRED(BundleEvent::Type::BUNDLE_STARTING ==
                               static_cast<BundleEvent::Type>(128),
                             "Test assigned event type values");
  US_TEST_CONDITION_REQUIRED(BundleEvent::Type::BUNDLE_STOPPING ==
                               static_cast<BundleEvent::Type>(256),
                             "Test assigned event type values");
  US_TEST_CONDITION_REQUIRED(BundleEvent::Type::BUNDLE_LAZY_ACTIVATION ==
                               static_cast<BundleEvent::Type>(512),
                             "Test assigned event type values");

  // @todo mock the framework. We only need a Bundle object to construct a BundleEvent object.
  auto const f = FrameworkFactory().NewFramework();

  BundleEvent invalid_event;
  US_TEST_CONDITION_REQUIRED((!invalid_event),
                             "Test for invalid BundleEvent construction.");
  US_TEST_CONDITION_REQUIRED(invalid_event.GetType() ==
                               BundleEvent::Type::BUNDLE_UNINSTALLED,
                             "invalid event GetType()");
  US_TEST_CONDITION_REQUIRED(!invalid_event.GetBundle(),
                             "invalid event GetBundle()");
  US_TEST_CONDITION_REQUIRED(!invalid_event.GetOrigin(),
                             "invalid event GetOrigin()");
  std::cout << invalid_event << std::endl;

  BundleEvent valid_event(BundleEvent::Type::BUNDLE_UNINSTALLED, f);
  US_TEST_CONDITION_REQUIRED(valid_event, "valid BundleEvent construction");
  US_TEST_CONDITION_REQUIRED(valid_event.GetType() ==
                               BundleEvent::Type::BUNDLE_UNINSTALLED,
                             "valid event GetType()");
  US_TEST_CONDITION_REQUIRED(valid_event.GetBundle() == f,
                             "valid event GetBundle()");
  std::cout << valid_event << std::endl;

  // copy test
  BundleEvent dup_valid_event(valid_event);
  US_TEST_CONDITION_REQUIRED(dup_valid_event == valid_event,
                             "Test BundleEvent copy ctor");

  // copy assignment test
  dup_valid_event = invalid_event;
  US_TEST_CONDITION_REQUIRED((dup_valid_event == invalid_event),
                             "Test BundleEvent copy assignment");

  dup_valid_event = BundleEvent(BundleEvent::Type::BUNDLE_INSTALLED, f);
  US_TEST_CONDITION_REQUIRED(!(dup_valid_event == invalid_event),
                             "Test BundleEvent move");

  US_TEST_END()
}
