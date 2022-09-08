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

#ifndef CPPMICROSERVICES_THREADS_H
#define CPPMICROSERVICES_THREADS_H

#include "cppmicroservices/FrameworkConfig.h"

#include <atomic>
#include <memory>
#include <mutex>

namespace cppmicroservices {

namespace detail {

template<class MutexHost>
class WaitCondition;

template<class Mutex = std::mutex>
class MutexLockingStrategy
{
public:
  typedef Mutex MutexType;

  MutexLockingStrategy() {}

  MutexLockingStrategy(const MutexLockingStrategy&)
#ifdef US_ENABLE_THREADING_SUPPORT
    : m_Mtx()
#endif
  {}

  friend class UniqueLock;

  class UniqueLock
  {
  public:
    UniqueLock() {}

    UniqueLock(const UniqueLock&) = delete;
    UniqueLock& operator=(const UniqueLock&) = delete;

#ifdef US_ENABLE_THREADING_SUPPORT

    UniqueLock(UniqueLock&& o)
      : m_Lock(std::move(o.m_Lock))
    {}

    UniqueLock& operator=(UniqueLock&& o)
    {
      m_Lock = std::move(o.m_Lock);
      return *this;
    }

    // Lock object
    explicit UniqueLock(const MutexLockingStrategy& host)
      : m_Lock(host.m_Mtx)
    {}

    // Lock object
    explicit UniqueLock(const MutexLockingStrategy* host)
      : m_Lock(host->m_Mtx)
    {}

    UniqueLock(const MutexLockingStrategy& host, std::defer_lock_t d)
      : m_Lock(host.m_Mtx, d)
    {}

    void Lock() { m_Lock.lock(); }

    void UnLock() { m_Lock.unlock(); }

    template<typename Rep, typename Period>
    bool TryLockFor(const std::chrono::duration<Rep, Period>& duration)
    {
      return m_Lock.try_lock_for(duration);
    }

#else
    UniqueLock(UniqueLock&&) {}
    UniqueLock& operator=(UniqueLock&&) {}
    explicit UniqueLock(const MutexLockingStrategy&) {}
    explicit UniqueLock(const MutexLockingStrategy*) {}
    void Lock() {}
    void UnLock() {}
    template<typename Rep, typename Period>
    bool TryLockFor(const std::chrono::duration<Rep, Period>&)
    {
      return true;
    }
#endif

  private:
    friend class WaitCondition<MutexLockingStrategy>;

#ifdef US_ENABLE_THREADING_SUPPORT
    std::unique_lock<MutexType> m_Lock;
#endif
  };

  /**
   * @brief Lock this object.
   *
   * Call this method to lock this object and obtain a lock object
   * which automatically releases the acquired lock when it goes out
   * of scope. E.g.
   *
   * \code
   * auto lock = object->Lock();
   * \endcode
   *
   * @return A lock object.
   */
  UniqueLock Lock() const { return UniqueLock(this); }

  UniqueLock DeferLock() const { return UniqueLock(this); }

protected:
#ifdef US_ENABLE_THREADING_SUPPORT
  mutable MutexType m_Mtx;
#endif
};

class NoLockingStrategy
{
public:
  typedef void UniqueLock;
};

template<class MutexHost>
class NoWaitCondition
{};

template<class LockingStrategy = MutexLockingStrategy<>,
         template<class MutexHost> class WaitConditionStrategy =
           NoWaitCondition>
class MultiThreaded
  : public LockingStrategy
  , public WaitConditionStrategy<LockingStrategy>
{};

template<class T>
class Atomic : private MultiThreaded<>
{
  T m_t;

public:
  template<class... Args>
  Atomic(Args&&... args)
    : m_t{ std::forward<Args>(args)... }
  {}

  T Load() const { return Lock(), m_t; }

  void Store(const T& t) { Lock(), m_t = t; }

  T Exchange(const T& t)
  {
    auto l = Lock();
    US_UNUSED(l);
    auto o = m_t;
    m_t = t;
    return o;
  }

  bool CompareExchange(T& expected, const T& desired)
  {
    auto l = Lock();
    US_UNUSED(l);
    if (expected == m_t) {
      m_t = desired;
      return true;
    }
    expected = m_t;
    return false;
  }
};

#if !defined(__GNUC__) || __GNUC__ > 4
// The std::atomic_load() et.al. overloads for std::shared_ptr are only available
// in libstdc++ since GCC 5.0. Visual Studio 2013 has it, but the Clang version
// is unknown so far.

// Specialize cppmicroservices::Atomic for std::shared_ptr to use the standard library atomic
// functions:
template<class T>
class Atomic<std::shared_ptr<T>>
{

  std::shared_ptr<T> m_t;

public:
  std::shared_ptr<T> Load() const { return std::atomic_load(&m_t); }

  void Store(const std::shared_ptr<T>& t) { std::atomic_store(&m_t, t); }

  std::shared_ptr<T> Exchange(const std::shared_ptr<T>& t)
  {
    return std::atomic_exchange(&m_t, t);
  }

  bool CompareExchange(std::shared_ptr<T>& expected,
                       const std::shared_ptr<T>& desired)
  {
    return std::atomic_compare_exchange_strong(&m_t, &expected, desired);
  }
};
#endif

} // namespace detail

} // namespace cppmicroservices

#endif // CPPMICROSERVICES_THREADS_H
