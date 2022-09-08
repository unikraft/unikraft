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

#ifndef CPPMICROSERVICES_LISTENERTOKEN_H
#define CPPMICROSERVICES_LISTENERTOKEN_H

#include "cppmicroservices/FrameworkExport.h"

#include <cstdint>

namespace cppmicroservices {

typedef std::uint64_t ListenerTokenId;

/**
   * \brief The token returned when a listener is registered with the framework.
   *
   * The token object is a move-only type and enables clients to remove the
   * listeners from the framework.
   */
class US_Framework_EXPORT ListenerToken
{
public:
  /**
     * Constructs a default, invalid %ListenerToken object.
     * As this is not associated with any valid listener, a RemoveListener
     * call taking a default ListenerToken object will do nothing.
     */
  ListenerToken();

  ListenerToken(const ListenerToken&) = delete;

  ListenerToken& operator=(const ListenerToken&) = delete;

  ListenerToken(ListenerToken&& other);

  ListenerToken& operator=(ListenerToken&& other);

  /**
     * Tests this %ListenerToken object for validity.
     *
     * Invalid \c ListenerToken objects are created by the default constructor.
     * Also, a \c ListenerToken object can become invalid if it is moved to another
     * ListenerToken object.
     *
     * @return \c true if this %ListenerToken object is valid, false otherwise.
     */
  explicit operator bool() const;

private:
  // The only (internal) client which can initialize this class with a ListenerTokenId.
  friend class ServiceListeners;

  explicit ListenerToken(ListenerTokenId _tokenId);

  ListenerTokenId Id() const;

  ListenerTokenId tokenId;
};
}

#endif // CPPMICROSERVICES_LISTENERTOKEN_H
