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

#ifndef CPPMICROSERVICES_UTILS_H
#define CPPMICROSERVICES_UTILS_H

#include "cppmicroservices/FrameworkExport.h"

#include <exception>
#include <string>

namespace cppmicroservices {

//-------------------------------------------------------------------
// File type checking
//-------------------------------------------------------------------

bool IsSharedLibrary(const std::string& location);

bool IsBundleFile(const std::string& location);

/**
 * Return true if the bundle's zip file only contains a 
 * manifest file, false otherwise.
 *
 * @param location The bundle file path
 *
 * @return true if the bundle's zip file only contains a 
 *         manifest file, false otherwise.
 * @throw std::runtime_error if the bundle location is empty or
 *        the bundle manifest cannot be read.
 */
bool OnlyContainsManifest(const std::string& location);

//-------------------------------------------------------------------
// Framework storage
//-------------------------------------------------------------------

class CoreBundleContext;

extern const std::string FWDIR_DEFAULT;

std::string GetFrameworkDir(CoreBundleContext* ctx);

/**
* Optionally create and get the persistent storage path.
*
* @param ctx Pointer to the CoreBundleContext object.
* @param leafDir The name of the leaf directory in the persistent storage path.
* @param create Specify if the directory needs to be created if it doesn't already exist.
*
* @return A directory path or an empty string if no storage is available.
*
* @throw std::runtime_error if the storage directory is inaccessible
*        or if there exists a file named @c leafDir in that directory
*        or if the directory cannot be created when @c create is @c true.
*/
std::string GetPersistentStoragePath(CoreBundleContext* ctx,
                                     const std::string& leafDir,
                                     bool create = true);

//-------------------------------------------------------------------
// Generic utility functions
//-------------------------------------------------------------------

void TerminateForDebug(const std::exception_ptr ex);

namespace detail {
US_Framework_EXPORT std::string GetDemangledName(
  const std::type_info& typeInfo);
}

} // namespace cppmicroservices

#endif // CPPMICROSERVICES_UTILS_H
