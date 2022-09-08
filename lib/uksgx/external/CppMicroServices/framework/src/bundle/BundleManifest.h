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

#ifndef CPPMICROSERVICES_BUNDLEMANIFEST_H
#define CPPMICROSERVICES_BUNDLEMANIFEST_H

#include "cppmicroservices/Any.h"

#include "cppmicroservices/AnyMap.h"

namespace cppmicroservices {

class BundleManifest
{

public:
  BundleManifest();

  void Parse(std::istream& is);

  AnyMap GetHeaders() const;

  bool Contains(const std::string& key) const;
  Any GetValue(const std::string& key) const;

  Any GetValueDeprecated(const std::string& key) const;
  std::vector<std::string> GetKeysDeprecated() const;

  std::map<std::string, Any> GetPropertiesDeprecated() const;

private:
  std::map<std::string, Any> m_PropertiesDeprecated;
  AnyMap m_Headers;
};
}

#endif // CPPMICROSERVICES_BUNDLEMANIFEST_H
