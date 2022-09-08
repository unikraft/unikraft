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

#include "cppmicroservices/ServiceException.h"

#include <ostream>

namespace cppmicroservices {

ServiceException::~ServiceException() throw() {}

ServiceException::ServiceException(const std::string& msg, const Type& type)
  : std::runtime_error(msg)
  , type(type)
{}

ServiceException::ServiceException(const ServiceException& o)
  : std::runtime_error(o)
  , type(o.type)
{}

ServiceException& ServiceException::operator=(const ServiceException& o)
{
  std::runtime_error::operator=(o);
  this->type = o.type;
  return *this;
}

ServiceException::Type ServiceException::GetType() const
{
  return type;
}
}

using namespace cppmicroservices;

std::ostream& operator<<(std::ostream& os, const ServiceException& exc)
{
  return os << "ServiceException: " << exc.what();
}
