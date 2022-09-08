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

#include "BundleObjFile.h"

#include <cstring>

namespace cppmicroservices {

InvalidObjFileException::InvalidObjFileException(const std::string& what,
                                                 int errorNumber)
  : m_What(what)
{
  if (errorNumber) {
    m_What += std::string(": ") + strerror(errorNumber);
  }
}

const char* InvalidObjFileException::what() const throw()
{
  return m_What.c_str();
}

bool BundleObjFile::ExtractBundleName(const std::string& name, std::string& out)
{
  static const std::string bundleSignature = "_us_import_bundle_initializer_";
  if (name.size() > bundleSignature.size() &&
      name.compare(0, bundleSignature.size(), bundleSignature) == 0) {
    out = name.substr(bundleSignature.size());
    return true;
  }
  return false;
}
}
