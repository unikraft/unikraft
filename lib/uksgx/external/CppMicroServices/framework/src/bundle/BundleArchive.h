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

#ifndef CPPMICROSERVICES_BUNDLEARCHIVE_H
#define CPPMICROSERVICES_BUNDLEARCHIVE_H

#include <memory>
#include <string>
#include <vector>
#include <chrono>

namespace cppmicroservices {

class BundleResource;
class BundleResourceContainer;
struct BundleStorage;

/**
 * Class for managing bundle data.
 */
struct BundleArchive : std::enable_shared_from_this<BundleArchive>
{
  using TimeStamp = std::chrono::steady_clock::time_point;

  struct Data
  {
    long bundleId;
    int64_t lastModified;
    int32_t autostartSetting;
  };

  BundleArchive(const BundleArchive&) = delete;
  BundleArchive& operator=(const BundleArchive&) = delete;

  BundleArchive();

  BundleArchive(
    BundleStorage* storage,
    std::unique_ptr<Data>&& data,
    const std::shared_ptr<BundleResourceContainer>& resourceContainer,
    const std::string& resourcePrefix,
    const std::string& location);

  /**
   * Autostart setting stopped.
   *
   * @see BundleArchive#setAutostartSetting(String)
   */
  static const std::string AUTOSTART_SETTING_STOPPED; // = "stopped";

  /**
   * Autostart setting eager.
   *
   * @see BundleArchive#setAutostartSetting(String)
   */
  static const std::string AUTOSTART_SETTING_EAGER; // = "eager";

  /**
   * Autostart setting declared activation policy.
   *
   * @see BundleArchive#setAutostartSetting(String)
   */
  static const std::string
    AUTOSTART_SETTING_ACTIVATION_POLICY; // = "activation_policy";

  bool IsValid() const;

  /**
   * Remove bundle archive from storage.
   */
  void Purge();

  /**
   * Get bundle identifier for this bundle archive.
   *
   * @return Bundle identifier.
   */
  long GetBundleId() const;

  /**
   * Get bundle location for this bundle archive.
   *
   * @return Bundle location.
   */
  std::string GetBundleLocation() const;

  /**
   * Get resource prefix in the shared resource container for this bundle archive.
   *
   * @return Resource prefix.
   */
  std::string GetResourcePrefix() const;

  /**
   * Get a BundleResource to named entry inside a bundle.
   *
   * @param path Entry to get reference to.
   * @return BundleResource to entry.
   */
  BundleResource GetResource(const std::string& path) const;

  /**
   * Returns a list of all the paths to entries within the bundle matching
   * the pattern.
   *
   * @param path
   * @param filePattern
   * @param recurse
   * @return
   */
  std::vector<BundleResource> FindResources(const std::string& path,
                                            const std::string& filePattern,
                                            bool recurse) const;

  /**
   * Get last modified timestamp.
   */
  TimeStamp GetLastModified() const;

  /**
   * Set stored last modified timestamp.
   */
  void SetLastModified(const TimeStamp& ts);

  /**
   * Get auto-start setting.
   *
   * @return the autostart setting. "-1" if bundle not started.
   */
  int32_t GetAutostartSetting() const;

  /**
   * Set the auto-start setting.
   *
   * @param setting the autostart setting to use.
   */
  void SetAutostartSetting(int32_t setting);

  std::shared_ptr<BundleResourceContainer> GetResourceContainer() const;

private:
  BundleStorage* const storage;
  const std::unique_ptr<Data> data;
  const std::shared_ptr<BundleResourceContainer> resourceContainer;
  const std::string resourcePrefix;
  const std::string location;
};
}

#endif // CPPMICROSERVICES_BUNDLEARCHIVE_H
