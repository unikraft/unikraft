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

#ifndef IDICTIONARYSERVICE_H
#define IDICTIONARYSERVICE_H

//! [service]
#include "cppmicroservices/ServiceInterface.h"

#include <string>

#ifdef US_BUILD_SHARED_LIBS
#  ifdef Tutorial_dictionaryservice_EXPORTS
#    define DICTIONARYSERVICE_EXPORT US_ABI_EXPORT
#  else
#    define DICTIONARYSERVICE_EXPORT US_ABI_IMPORT
#  endif
#else
#  define DICTIONARYSERVICE_EXPORT US_ABI_EXPORT
#endif

/**
 * A simple service interface that defines a dictionary service.
 * A dictionary service simply verifies the existence of a word.
 **/
struct DICTIONARYSERVICE_EXPORT IDictionaryService
{
  // Out-of-line virtual desctructor for proper dynamic cast
  // support with older versions of gcc.
  virtual ~IDictionaryService();

  /**
   * Check for the existence of a word.
   * @param word the word to be checked.
   * @return true if the word is in the dictionary,
   *         false otherwise.
   **/
  virtual bool CheckWord(const std::string& word) = 0;
};
//! [service]

#endif // DICTIONARYSERVICE_H
