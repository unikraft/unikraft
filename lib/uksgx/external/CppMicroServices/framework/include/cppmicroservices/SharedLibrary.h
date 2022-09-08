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

#ifndef CPPMICROSERVICES_SHAREDLIBRARY_H
#define CPPMICROSERVICES_SHAREDLIBRARY_H

#include "cppmicroservices/FrameworkConfig.h"
#include "cppmicroservices/SharedData.h"

#include <string>

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4251)
#endif

namespace cppmicroservices {

class SharedLibraryPrivate;

/**
 * \ingroup MicroServicesUtils
 *
 * The SharedLibrary class loads shared libraries at runtime.
 */
class US_Framework_EXPORT SharedLibrary
{
public:
  SharedLibrary();
  SharedLibrary(const SharedLibrary& other);

  /**
   * Construct a SharedLibrary object using a library search path and
   * a library base name.
   *
   * @param libPath An absolute path containing the shared library
   * @param name The base name of the shared library, without prefix
   *        and suffix.
   */
  SharedLibrary(const std::string& libPath, const std::string& name);

  /**
   * Construct a SharedLibrary object using an absolute file path to
   * the shared library. Using this constructor effectively disables
   * all setters except SetFilePath().
   *
   * @param absoluteFilePath The absolute path to the shared library.
   */
  SharedLibrary(const std::string& absoluteFilePath);

  /**
   * Destroys this object but does not unload the shared library.
   */
  ~SharedLibrary();

  SharedLibrary& operator=(const SharedLibrary& other);

  /**
   * Loads the shared library pointed to by this SharedLibrary object.
   * On POSIX systems dlopen() is called with the RTLD_LAZY and
   * RTLD_LOCAL flags unless the compiler is gcc 4.4.x or older. Then
   * the RTLD_LAZY and RTLD_GLOBAL flags are used to load the shared library
   * to work around RTTI problems across shared library boundaries.
   *
   * @throws std::logic_error If the library is already loaded.
   * @throws std::runtime_error If loading the library failed.
   */
  void Load();

  /**
   * Loads the shared library pointed to by this SharedLibrary object,
   * using the specified flags on POSIX systems.
   *
   * @throws std::logic_error If the library is already loaded.
   * @throws std::runtime_error If loading the library failed.
   */
  void Load(int flags);

  /**
   * Un-loads the shared library pointed to by this SharedLibrary object.
   *
   * @throws std::runtime_error If an error occurred while un-loading the
   *         shared library.
   */
  void Unload();

  /**
   * Sets the base name of the shared library. Does nothing if the shared
   * library is already loaded or the SharedLibrary(const std::string&)
   * constructor was used.
   *
   * @param name The base name of the shared library, without prefix and
   *        suffix.
   */
  void SetName(const std::string& name);

  /**
   * Gets the base name of the shared library.
   * @return The shared libraries base name.
   */
  std::string GetName() const;

  /**
   * Gets the absolute file path for the shared library with base name
   * \c name, using the search path returned by GetLibraryPath().
   *
   * @param name The shared library base name.
   * @return The absolute file path of the shared library.
   */
  std::string GetFilePath(const std::string& name) const;

  /**
   * Sets the absolute file path of this SharedLibrary object.
   * Using this methods with a non-empty \c absoluteFilePath argument
   * effectively disables all other setters.
   *
   * @param absoluteFilePath The new absolute file path of this SharedLibrary
   *        object.
   */
  void SetFilePath(const std::string& absoluteFilePath);

  /**
   * Gets the absolute file path of this SharedLibrary object.
   *
   * @return The absolute file path of the shared library.
   */
  std::string GetFilePath() const;

  /**
   * Sets a new library search path. Does nothing if the shared
   * library is already loaded or the SharedLibrary(const std::string&)
   * constructor was used.
   *
   * @param path The new shared library search path.
   */
  void SetLibraryPath(const std::string& path);

  /**
   * Gets the library search path of this SharedLibrary object.
   *
   * @return The library search path.
   */
  std::string GetLibraryPath() const;

  /**
   * Sets the suffix for shared library names (e.g. lib). Does nothing if the shared
   * library is already loaded or the SharedLibrary(const std::string&)
   * constructor was used.
   *
   * @param suffix The shared library name suffix.
   */
  void SetSuffix(const std::string& suffix);

  /**
   * Gets the file name suffix of this SharedLibrary object.
   *
   * @return The file name suffix of the shared library.
   */
  std::string GetSuffix() const;

  /**
   * Sets the file name prefix for shared library names (e.g. .dll or .so).
   * Does nothing if the shared library is already loaded or the
   * SharedLibrary(const std::string&) constructor was used.
   *
   * @param prefix The shared library name prefix.
   */
  void SetPrefix(const std::string& prefix);

  /**
   * Gets the file name prefix of this SharedLibrary object.
   *
   * @return The file name prefix of the shared library.
   */
  std::string GetPrefix() const;

  /**
   * Gets the internal handle of this SharedLibrary object.
   *
   * @return \c nullptr if the shared library is not loaded, the operating
   * system specific handle otherwise.
   */
  void* GetHandle() const;

  /**
   * Gets the loaded/unloaded stated of this SharedLibrary object.
   *
   * @return \c true if the shared library is loaded, \c false otherwise.
   */
  bool IsLoaded() const;

private:
  ExplicitlySharedDataPointer<SharedLibraryPrivate> d;
};
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

#endif // CPPMICROSERVICES_SHAREDLIBRARY_H
