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

#ifndef CPPMICROSERVICES_SERVICEEXCEPTION_H
#define CPPMICROSERVICES_SERVICEEXCEPTION_H

#include "cppmicroservices/FrameworkConfig.h"

#include <stdexcept>

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4275)
#endif

namespace cppmicroservices {

/**
\defgroup gr_serviceexception BundleEvent

\brief Groups ServiceException class related symbols.
*/

/**
 * \ingroup MicroServices
 * \ingroup gr_serviceexception
 *
 * A service exception used to indicate that a service problem occurred.
 *
 * <p>
 * A <code>ServiceException</code> object is created by the framework or
 * to denote an exception condition in the service. An enum
 * type is used to identify the exception type for future extendability.
 *
 * <p>
 * This exception conforms to the general purpose exception chaining mechanism.
 */
class US_Framework_EXPORT ServiceException : public std::runtime_error
{
public:
  enum Type
  {
    /**
     * No exception type is unspecified.
     */
    UNSPECIFIED = 0,
    /**
     * The service has been unregistered.
     */
    UNREGISTERED = 1,
    /**
     * The service factory produced an invalid service object.
     */
    FACTORY_ERROR = 2,
    /**
     * The service factory threw an exception.
     */
    FACTORY_EXCEPTION = 3,
    /**
     * An error occurred invoking a remote service.
     */
    REMOTE = 5,
    /**
     * The service factory resulted in a recursive call to itself for the
     * requesting bundle.
     */
    FACTORY_RECURSION = 6
  };

  /**
   * Creates a <code>ServiceException</code> with the specified message,
   * type and exception cause.
   *
   * @param msg The associated message.
   * @param type The type for this exception.
   */
  ServiceException(const std::string& msg, const Type& type = UNSPECIFIED);

  ServiceException(const ServiceException& o);
  ServiceException& operator=(const ServiceException& o);

  ~ServiceException() throw();

  /**
   * Returns the type for this exception or <code>UNSPECIFIED</code> if the
   * type was unspecified or unknown.
   *
   * @return The type of this exception.
   */
  Type GetType() const;

private:
  /**
   * Type of service exception.
   */
  Type type;
};
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

/**
 * \ingroup MicroServices
 * \ingroup gr_serviceexception
 *
 * Writes a string representation of \c exc to the stream \c os.
 */
US_Framework_EXPORT std::ostream& operator<<(
  std::ostream& os,
  const cppmicroservices::ServiceException& exc);

#endif // CPPMICROSERVICES_SERVICEEXCEPTION_H
