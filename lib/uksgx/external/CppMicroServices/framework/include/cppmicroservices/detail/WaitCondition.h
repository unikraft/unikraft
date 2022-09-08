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

#ifndef CPPMICROSERVICES_WAITCONDITION_H
#define CPPMICROSERVICES_WAITCONDITION_H

#include "cppmicroservices/GlobalConfig.h"

#include <chrono>
#include <condition_variable>

namespace cppmicroservices {

namespace detail {

template<class MutexHost>
class WaitCondition
{
public:
  void Wait(typename MutexHost::UniqueLock& lock)
  {
#ifdef US_ENABLE_THREADING_SUPPORT
    m_CondVar.wait(lock.m_Lock);
#else
    US_UNUSED(lock);
#endif
  }

  template<class Predicate>
  void Wait(typename MutexHost::UniqueLock& lock, Predicate pred)
  {
#ifdef US_ENABLE_THREADING_SUPPORT
    m_CondVar.wait(lock.m_Lock, pred);
#else
    US_UNUSED(lock);
    US_UNUSED(pred);
#endif
  }

  template<class Rep, class Period>
  std::cv_status WaitFor(typename MutexHost::UniqueLock& lock,
                         const std::chrono::duration<Rep, Period>& rel_time)
  {
#ifdef US_ENABLE_THREADING_SUPPORT
    if (rel_time == std::chrono::duration<Rep, Period>::zero()) {
      Wait(lock);
      return std::cv_status::no_timeout;
    } else {
      return m_CondVar.wait_for(lock.m_Lock, rel_time);
    }
#else
    US_UNUSED(lock);
    US_UNUSED(rel_time);
    return std::cv_status::no_timeout;
#endif
  }

  template<class Rep, class Period, class Predicate>
  bool WaitFor(typename MutexHost::UniqueLock& lock,
               const std::chrono::duration<Rep, Period>& rel_time,
               Predicate pred)
  {
#ifdef US_ENABLE_THREADING_SUPPORT
    if (rel_time == std::chrono::duration<Rep, Period>::zero()) {
      Wait(lock, pred);
      return true;
    } else {
      return m_CondVar.wait_for(lock.m_Lock, rel_time, pred);
    }
#else
    US_UNUSED(lock);
    US_UNUSED(rel_time);
    US_UNUSED(pred);
    return pred();
#endif
  }

  /** Notify that the condition is true and release one waiting thread */
  void Notify()
  {
#ifdef US_ENABLE_THREADING_SUPPORT
    m_CondVar.notify_one();
#endif
  }

  /** Notify that the condition is true and release all waiting threads */
  void NotifyAll()
  {
#ifdef US_ENABLE_THREADING_SUPPORT
    m_CondVar.notify_all();
#endif
  }

private:
#ifdef US_ENABLE_THREADING_SUPPORT
  std::condition_variable m_CondVar;
#endif
};

} // namespace detail

} // namespace cppmicroservices

#endif // CPPMICROSERVICES_WAITCONDITION_H
