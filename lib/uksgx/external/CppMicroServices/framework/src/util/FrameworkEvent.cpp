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

#include "cppmicroservices/Bundle.h"

#include "cppmicroservices/util/Error.h"

namespace cppmicroservices {

class FrameworkEventData
{
public:
  FrameworkEventData(FrameworkEvent::Type type,
                     const Bundle& bundle,
                     const std::string& message,
                     const std::exception_ptr exception)
    : type(type)
    , bundle(bundle)
    , message(message)
    , exception(exception)
  {}

  FrameworkEventData(const FrameworkEventData& other)
    : type(other.type)
    , bundle(other.bundle)
    , message(other.message)
    , exception(other.exception)
  {}

  const FrameworkEvent::Type type;
  const Bundle bundle;
  const std::string message;
  const std::exception_ptr exception;
};

FrameworkEvent::FrameworkEvent()
  : d(nullptr)
{}

FrameworkEvent::FrameworkEvent(Type type,
                               const Bundle& bundle,
                               const std::string& message,
                               const std::exception_ptr exception)
  : d(new FrameworkEventData(type, bundle, message, exception))
{}

Bundle FrameworkEvent::GetBundle() const
{
  if (!d)
    return Bundle{};
  return d->bundle;
}

FrameworkEvent::Type FrameworkEvent::GetType() const
{
  if (!d)
    return Type::FRAMEWORK_ERROR;
  return d->type;
}

std::string FrameworkEvent::GetMessage() const
{
  if (!d)
    return std::string();
  return d->message;
}

std::exception_ptr FrameworkEvent::GetThrowable() const
{
  if (!d)
    return nullptr;
  return d->exception;
}

FrameworkEvent::operator bool() const
{
  return d.operator bool();
}

std::ostream& operator<<(std::ostream& os, FrameworkEvent::Type eventType)
{
  switch (eventType) {
    case FrameworkEvent::Type::FRAMEWORK_STARTED:
      return os << "STARTED";
    case FrameworkEvent::Type::FRAMEWORK_ERROR:
      return os << "ERROR";
    case FrameworkEvent::Type::FRAMEWORK_WARNING:
      return os << "WARNING";
    case FrameworkEvent::Type::FRAMEWORK_INFO:
      return os << "INFO";
    case FrameworkEvent::Type::FRAMEWORK_STOPPED:
      return os << "STOPPED";
    case FrameworkEvent::Type::FRAMEWORK_STOPPED_UPDATE:
      return os << "STOPPED_UPDATE";
    case FrameworkEvent::Type::FRAMEWORK_WAIT_TIMEDOUT:
      return os << "WAIT_TIMEDOUT";

    default:
      return os << "Unknown bundle event type ("
                << static_cast<unsigned int>(eventType) << ")";
  }
}

std::ostream& operator<<(std::ostream& os, const FrameworkEvent& evt)
{
  if (!evt)
    return os << "NONE";

  std::string exceptionStr("NONE");
  if (evt.GetThrowable()) {
    exceptionStr = util::GetExceptionStr(evt.GetThrowable());
  }

  os << evt.GetType() << "\n " << evt.GetMessage() << "\n " << evt.GetBundle()
     << "\n Exception: " << exceptionStr;
  return os;
}

bool operator==(const FrameworkEvent& rhs, const FrameworkEvent& lhs)
{
  return (rhs.GetBundle() == lhs.GetBundle() &&
          rhs.GetMessage() == lhs.GetMessage() &&
          rhs.GetThrowable() == lhs.GetThrowable() &&
          rhs.GetType() == lhs.GetType());
}
}
