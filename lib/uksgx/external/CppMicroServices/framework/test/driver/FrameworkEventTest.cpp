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

#include "cppmicroservices/FrameworkEvent.h"
#include "cppmicroservices/Framework.h"
#include "cppmicroservices/FrameworkFactory.h"

#include "TestUtils.h"
#include "TestingConfig.h"
#include "TestingMacros.h"

#include <iostream>
#include <typeinfo>

using namespace cppmicroservices;

namespace {

std::string GetMessageFromStdExceptionPtr(const std::exception_ptr ptr)
{
  if (ptr) {
    try {
      std::rethrow_exception(ptr);
    } catch (const std::exception& e) {
      return e.what();
    }
  }
  return std::string();
}

} // end anonymous namespace

int FrameworkEventTest(int /*argc*/, char* /*argv*/ [])
{
  US_TEST_BEGIN("FrameworkEventTest");

  // The OSGi spec assigns specific values to event types for future extensibility.
  // Ensure we don't deviate from those assigned values.
  US_TEST_CONDITION_REQUIRED(FrameworkEvent::Type::FRAMEWORK_STARTED ==
                               static_cast<FrameworkEvent::Type>(1),
                             "Test assigned event type values");
  US_TEST_CONDITION_REQUIRED(FrameworkEvent::Type::FRAMEWORK_ERROR ==
                               static_cast<FrameworkEvent::Type>(2),
                             "Test assigned event type values");
  US_TEST_CONDITION_REQUIRED(FrameworkEvent::Type::FRAMEWORK_INFO ==
                               static_cast<FrameworkEvent::Type>(32),
                             "Test assigned event type values");
  US_TEST_CONDITION_REQUIRED(FrameworkEvent::Type::FRAMEWORK_WARNING ==
                               static_cast<FrameworkEvent::Type>(16),
                             "Test assigned event type values");
  US_TEST_CONDITION_REQUIRED(FrameworkEvent::Type::FRAMEWORK_STOPPED ==
                               static_cast<FrameworkEvent::Type>(64),
                             "Test assigned event type values");
  US_TEST_CONDITION_REQUIRED(FrameworkEvent::Type::FRAMEWORK_STOPPED_UPDATE ==
                               static_cast<FrameworkEvent::Type>(128),
                             "Test assigned event type values");
  US_TEST_CONDITION_REQUIRED(FrameworkEvent::Type::FRAMEWORK_WAIT_TIMEDOUT ==
                               static_cast<FrameworkEvent::Type>(512),
                             "Test assigned event type values");

  // @todo mock the framework. We only need a Bundle object to construct a FrameworkEvent object.
  auto const f = FrameworkFactory().NewFramework();

  std::string default_exception_message(GetMessageFromStdExceptionPtr(nullptr));

  FrameworkEvent invalid_event;
  US_TEST_CONDITION_REQUIRED((!invalid_event),
                             "Test for invalid FrameworkEvent construction.");
  US_TEST_CONDITION_REQUIRED(invalid_event.GetType() ==
                               FrameworkEvent::Type::FRAMEWORK_ERROR,
                             "invalid event GetType()");
  US_TEST_CONDITION_REQUIRED(!invalid_event.GetBundle(),
                             "invalid event GetBundle()");
  US_TEST_CONDITION_REQUIRED(
    (GetMessageFromStdExceptionPtr(invalid_event.GetThrowable()) ==
     default_exception_message),
    "invalid event GetThrowable()");
  std::cout << invalid_event << std::endl;

  FrameworkEvent error_event(
    FrameworkEvent::Type::FRAMEWORK_ERROR,
    f,
    "test framework error event",
    std::make_exception_ptr(std::runtime_error("test exception")));
  US_TEST_CONDITION_REQUIRED((!error_event) == false,
                             "FrameworkEvent construction - error type");
  US_TEST_CONDITION_REQUIRED(error_event.GetType() ==
                               FrameworkEvent::Type::FRAMEWORK_ERROR,
                             "error event GetType()");
  US_TEST_CONDITION_REQUIRED(error_event.GetBundle() == f,
                             "error event GetBundle()");
  US_TEST_CONDITION_REQUIRED(
    GetMessageFromStdExceptionPtr(error_event.GetThrowable()) ==
      std::string("test exception"),
    "error event GetThrowable");
  std::cout << error_event << std::endl;

  bool exception_caught = false;
  try {
    std::rethrow_exception(error_event.GetThrowable());
  } catch (const std::exception& ex) {
    exception_caught = true;

    US_TEST_CONDITION_REQUIRED(
      ex.what() == std::string("test exception"),
      "Test FrameworkEvent::Type::FRAMEWORK_ERROR exception");
    US_TEST_CONDITION_REQUIRED(
      std::string(typeid(std::runtime_error).name()) == typeid(ex).name(),
      std::string("Test that the correct exception type was thrown: ") +
        typeid(std::runtime_error).name() + " == " + typeid(ex).name());
  }
  US_TEST_CONDITION_REQUIRED(exception_caught,
                             "Test throw/catch a FrameworkEvent exception");

  FrameworkEvent info_event(
    FrameworkEvent::Type::FRAMEWORK_INFO, f, "test info framework event");
  US_TEST_CONDITION_REQUIRED((!info_event) == false,
                             "FrameworkEvent construction - info type");
  US_TEST_CONDITION_REQUIRED(info_event.GetType() ==
                               FrameworkEvent::Type::FRAMEWORK_INFO,
                             "info event GetType()");
  US_TEST_CONDITION_REQUIRED(info_event.GetBundle() == f,
                             "info event GetBundle()");
  US_TEST_CONDITION_REQUIRED(
    GetMessageFromStdExceptionPtr(info_event.GetThrowable()) ==
      default_exception_message,
    "info event GetThrowable()");
  std::cout << info_event << std::endl;

  FrameworkEvent warn_event(
    FrameworkEvent::Type::FRAMEWORK_WARNING, f, "test warning framework event");
  US_TEST_CONDITION_REQUIRED((!warn_event) == false,
                             "FrameworkEvent construction - warning type");
  US_TEST_CONDITION_REQUIRED(warn_event.GetType() ==
                               FrameworkEvent::Type::FRAMEWORK_WARNING,
                             "warning event GetType()");
  US_TEST_CONDITION_REQUIRED(warn_event.GetBundle() == f,
                             "warning event GetBundle()");
  US_TEST_CONDITION_REQUIRED(
    GetMessageFromStdExceptionPtr(warn_event.GetThrowable()) ==
      default_exception_message,
    "wanring event GetThrowable()");
  std::cout << warn_event << std::endl;

  FrameworkEvent unknown_event(
    static_cast<FrameworkEvent::Type>(127), f, "test unknown framework event");
  US_TEST_CONDITION_REQUIRED((!unknown_event) == false,
                             "FrameworkEvent construction - unknown type");
  US_TEST_CONDITION_REQUIRED(unknown_event.GetType() ==
                               static_cast<FrameworkEvent::Type>(127),
                             "unknown event GetType()");
  US_TEST_CONDITION_REQUIRED(unknown_event.GetBundle() == f,
                             "unknown event GetBundle()");
  US_TEST_CONDITION_REQUIRED(
    GetMessageFromStdExceptionPtr(unknown_event.GetThrowable()) ==
      default_exception_message,
    "unknown event GetThrowable()");
  std::cout << unknown_event << std::endl;

  // copy test
  FrameworkEvent dup_error_event(error_event);
  US_TEST_CONDITION_REQUIRED(dup_error_event == error_event, "Test copy ctor");

  // copy assignment test
  dup_error_event = invalid_event;
  US_TEST_CONDITION_REQUIRED(!(dup_error_event == error_event),
                             "Test copy assignment");

  dup_error_event =
    FrameworkEvent(FrameworkEvent::Type::FRAMEWORK_STARTED, f, "");
  US_TEST_CONDITION_REQUIRED(!(dup_error_event == error_event), "Test move");

  US_TEST_END()
}
