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

#ifndef CPPMICROSERVICES_FRAMEWORKPRIVATE_H
#define CPPMICROSERVICES_FRAMEWORKPRIVATE_H

#include "cppmicroservices/FrameworkEvent.h"

#include "BundlePrivate.h"
#include "CoreBundleContext.h"

#include <map>
#include <string>

namespace cppmicroservices {

/**
 * The implementation of the Framework class.
 *
 * This class exists to hide and decouple the implementation of the
 * Framework class from client code.
 */
class FrameworkPrivate : public BundlePrivate
{
public:
  FrameworkPrivate(CoreBundleContext* fwCtx);

  void Init();

  void DoInit();

  void InitSystemBundle();

  void UninitSystemBundle();

  FrameworkEvent WaitForStop(const std::chrono::milliseconds& timeout);

  void Shutdown(bool restart);

  virtual void Start(uint32_t);
  virtual void Stop(uint32_t);

  virtual void Uninstall();
  virtual std::string GetLocation() const;

  virtual AnyMap GetHeaders() const;

  /**
   * Stop this FrameworkContext, suspending all started contexts. This method
   * suspends all started contexts so that they can be automatically restarted
   * when this FrameworkContext is next launched.
   *
   * <p>
   * If the framework is not started, this method does nothing. If the framework
   * is started, this method will:
   * <ol>
   * <li>Set the state of the FrameworkContext to <i>inactive</i>.</li>
   * <li>Stop all started bundles as described in the {@link Bundle#Stop(int)}
   * method except that the persistent state of the bundle will continue to be
   * started. Reports any exceptions that occur during stopping using
   * <code>FrameworkErrorEvents</code>.</li>
   * <li>Disable event handling.</li>
   * </ol>
   * </p>
   *
   */
  void Shutdown0(bool restart, bool wasActive);

  /**
   * Tell system bundle shutdown finished.
   */
  void ShutdownDone_unlocked(bool restart);

  /**
   *  Stop and unresolve all bundles.
   */
  void StopAllBundles();

  /**
   * The event to return to callers waiting in Framework.waitForStop() when the
   * framework has been stopped.
   */
  struct FrameworkEventInternal
  {
    bool valid;
    FrameworkEvent::Type type;
    std::string msg;
    std::exception_ptr excPtr;
  } stopEvent;

  /**
   * Shutting down is done.
   */
  void SystemShuttingdownDone_unlocked(const FrameworkEventInternal& fe);

  /**
   * The thread that performs shutdown of this framework instance.
   */
  std::thread shutdownThread;
};
}

#endif // CPPMICROSERVICES_FRAMEWORKPRIVATE_H
