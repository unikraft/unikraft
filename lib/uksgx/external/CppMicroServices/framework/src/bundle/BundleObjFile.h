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

#ifndef CPPMICROSERVICES_MODULEOBJFILE_P_H
#define CPPMICROSERVICES_MODULEOBJFILE_P_H

#include "cppmicroservices/GlobalConfig.h"

#include <string>
#include <vector>

namespace cppmicroservices {

struct InvalidObjFileException : public std::exception
{
  ~InvalidObjFileException() throw() {}
  InvalidObjFileException(const std::string& what, int errorNumber = 0);

  virtual const char* what() const throw();

  std::string m_What;
};

class BundleObjFile
{
public:
  virtual ~BundleObjFile() {}

  virtual std::vector<std::string> GetDependencies() const = 0;
  virtual std::string GetLibName() const = 0;
  virtual std::string GetBundleName() const = 0;

protected:
  static bool ExtractBundleName(const std::string& name, std::string& out);
};
}

#endif // CPPMICROSERVICES_MODULEOBJFILE_P_H
