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

#ifndef CPPMICROSERVICES_BUNDLETHREAD_H
#define CPPMICROSERVICES_BUNDLETHREAD_H

#include "cppmicroservices/BundleEvent.h"
#include "cppmicroservices/detail/Threads.h"
#include "cppmicroservices/detail/WaitCondition.h"

#include "BundleEventInternal.h"

#include <atomic>
#include <chrono>
#include <string>

#ifdef US_ENABLE_THREADING_SUPPORT
#  include <future>
#  include <thread>
#endif

namespace cppmicroservices {

class BundlePrivate;
class CoreBundleContext;

class BundleThread : public std::enable_shared_from_this<BundleThread>
{
  const static int OP_IDLE;
  const static int OP_BUNDLE_EVENT;
  const static int OP_START;
  const static int OP_STOP;

  detail::Atomic<BundleEventInternal> be;

#ifdef US_ENABLE_THREADING_SUPPORT
  const static std::chrono::milliseconds KEEP_ALIVE;

  std::chrono::milliseconds startStopTimeout;

  struct Op
    : detail::MultiThreaded<detail::MutexLockingStrategy<>,
                            detail::WaitCondition>
  {
    Op()
      : operation(OP_IDLE)
    {}

    std::atomic<BundlePrivate*> bundle;
    std::atomic<int> operation;
    std::promise<bool> pr;
  } op;

  std::atomic<bool> doRun;

  struct : detail::MultiThreaded<>
  {
    std::thread v;
  } th;
#endif

public:
  typedef detail::MultiThreaded<>::UniqueLock UniqueLock;

  BundleThread(CoreBundleContext* ctx);
  ~BundleThread();

  void Quit();

  void Run(CoreBundleContext* fwCtx);

  void Join();

  /**
   * Note! Must be called while holding packages lock.
   */
  void BundleChanged(const BundleEventInternal& be, UniqueLock& resolveLock);

  /**
   * Note! Must be called while holding packages lock.
   */
  std::exception_ptr CallStart0(BundlePrivate* b, UniqueLock& resolveLock);

  /**
   * Note! Must be called while holding packages lock.
   */
  std::exception_ptr CallStop1(BundlePrivate* b, UniqueLock& resolveLock);

  /**
   * Note! Must be called while holding packages lock.
   */
  std::exception_ptr StartAndWait(BundlePrivate* b,
                                  int op,
                                  UniqueLock& resolveLock);

  bool IsExecutingBundleChanged() const;

  bool operator==(const std::thread::id& id) const;
};
}

#endif // CPPMICROSERVICES_BUNDLETHREAD_H
