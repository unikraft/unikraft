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

#ifndef CPPMICROSERVICES_BUNDLERESOURCESTREAM_H
#define CPPMICROSERVICES_BUNDLERESOURCESTREAM_H

#include "cppmicroservices/detail/BundleResourceBuffer.h"

#include <fstream>

namespace cppmicroservices {

class BundleResource;

/**
 * \ingroup MicroServices
 *
 * An input stream class for BundleResource objects.
 *
 * This class provides access to the resource data embedded in a bundle's
 * shared library via a STL input stream interface.
 *
 * \see BundleResource for an example how to use this class.
 */
class US_Framework_EXPORT BundleResourceStream
  : private detail::BundleResourceBuffer
  , public std::istream
{

public:
  BundleResourceStream(const BundleResourceStream&) = delete;
  BundleResourceStream& operator=(const BundleResourceStream&) = delete;

  /**
   * Construct a %BundleResourceStream object.
   *
   * @param resource The BundleResource object for which an input stream
   * should be constructed.
   * @param mode The open mode of the stream. If \c std::ios_base::binary
   * is used, the resource data will be treated as binary data, otherwise
   * the data is interpreted as text data and the usual platform specific
   * end-of-line translations take place.
   */
  BundleResourceStream(const BundleResource& resource,
                       std::ios_base::openmode mode = std::ios_base::in);
};
}

#endif // CPPMICROSERVICES_BUNDLERESOURCESTREAM_H
