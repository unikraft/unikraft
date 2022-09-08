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

#include "cppmicroservices/Bundle.h"
#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/ServiceException.h"

#include "cppmicroservices/detail/ServiceTrackerPrivate.h"
#include "cppmicroservices/detail/TrackedService.h"

#include <limits>
#include <stdexcept>
#include <string>
#include <chrono>

namespace cppmicroservices {

template<class S, class T>
ServiceTracker<S,T>::~ServiceTracker()
{
  try 
  {
    Close();
  }
  catch (...) {}
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4355)
#endif

template<class S, class T>
ServiceTracker<S,T>::ServiceTracker(const BundleContext& context,
                                    const ServiceReference<S>& reference,
                                    _ServiceTrackerCustomizer* customizer)
  : d(new _ServiceTrackerPrivate(this, context, reference, customizer))
{
}

template<class S, class T>
ServiceTracker<S,T>::ServiceTracker(const BundleContext& context,
                                    const std::string& clazz,
                                    _ServiceTrackerCustomizer* customizer)
  : d(new _ServiceTrackerPrivate(this, context, clazz, customizer))
{
}

template<class S, class T>
ServiceTracker<S,T>::ServiceTracker(const BundleContext& context,
                                    const LDAPFilter& filter,
                                    _ServiceTrackerCustomizer* customizer)
  : d(new _ServiceTrackerPrivate(this, context, filter, customizer))
{
}

template<class S, class T>
ServiceTracker<S,T>::ServiceTracker(const BundleContext& context,
                                    _ServiceTrackerCustomizer* customizer)
  : d(new _ServiceTrackerPrivate(this, context, us_service_interface_iid<S>(), customizer))
{
  std::string clazz = us_service_interface_iid<S>();
  if (clazz.empty()) throw ServiceException("The service interface class has no CPPMICROSERVICES_DECLARE_SERVICE_INTERFACE macro");
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

template<class S, class T>
void ServiceTracker<S,T>::Open()
{
  std::shared_ptr<_TrackedService> t;
  {
    auto l = d->Lock(); US_UNUSED(l);
    if (d->trackedService.Load())
    {
      return;
    }

    DIAG_LOG(*d->context.GetLogSink()) << "ServiceTracker<S,TTT>::Open: " << d->filter;

    t.reset(new _TrackedService(this, d->customizer));
    try
    {
      /* Remove if already exists. No-op if it's an invalid (default) token */
      d->context.RemoveListener(std::move(d->listenerToken));
      d->listenerToken = d->context.AddServiceListener(std::bind(&_TrackedService::ServiceChanged,
                                                                 t.get(), std::placeholders::_1),
                                                       d->listenerFilter);
      std::vector<ServiceReference<S>> references;
      if (!d->trackClass.empty())
      {
        references = d->GetInitialReferences(d->trackClass, std::string());
      }
      else
      {
        if (d->trackReference.GetBundle())
        {
          references.push_back(d->trackReference);
        }
        else
        { /* user supplied filter */
          references = d->GetInitialReferences(std::string(),
                                               (d->listenerFilter.empty()) ? d->filter.ToString() : d->listenerFilter);
        }
      }
      /* set tracked with the initial references */
      t->SetInitial(references);
    }
    catch (const std::invalid_argument& e)
    {
      d->context.RemoveListener(std::move(d->listenerToken));
      throw std::runtime_error(std::string("unexpected std::invalid_argument exception: ")
                               + e.what());
    }
    d->trackedService.Store(t);
  }
  /* Call tracked outside of synchronized region */
  t->TrackInitial(); /* process the initial references */
}

template<class S, class T>
void ServiceTracker<S,T>::Close()
{
  std::vector<ServiceReference<S>> references;
  std::shared_ptr<_TrackedService> outgoing = d->trackedService.Exchange(std::shared_ptr<_TrackedService>());
  if (outgoing == nullptr)
  {
    return;
  }

  DIAG_LOG(*d->context.GetLogSink()) << "ServiceTracker<S,TTT>::close:" << d->filter;
  outgoing->Close();
  references = GetServiceReferences();
  try
  {
    d->context.RemoveListener(std::move(d->listenerToken));
  }
  catch (const std::runtime_error& /*e*/)
  {
    /* In case the context was stopped or invalid. */
  }

  d->Modified(); /* clear the cache */
  outgoing->NotifyAll(); /* wake up any waiters */
  for(auto& ref : references)
  {
    outgoing->Untrack(ref, ServiceEvent());
  }

  if (d->context.GetLogSink()->Enabled())
  {
    if (!d->cachedReference.Load().GetBundle() &&
        d->cachedService.Load() == nullptr)
    {
      DIAG_LOG(*d->context.GetLogSink()) << "ServiceTracker<S,TTT>::close[cached cleared]:"
                    << d->filter;
    }
  }

}

template<class S, class T>
std::shared_ptr<typename ServiceTracker<S,T>::TrackedParmType>
ServiceTracker<S,T>::WaitForService()
{
  return WaitForService(std::chrono::milliseconds::zero());
}

template<class S, class T>
template<class Rep, class Period>
std::shared_ptr<typename ServiceTracker<S,T>::TrackedParmType>
ServiceTracker<S,T>::WaitForService(const std::chrono::duration<Rep, Period>& rel_time)
{
  if (rel_time.count() < 0)
  {
    throw std::invalid_argument("negative timeout");
  }

  auto object = GetService();
  if (object) return object;

  typedef std::chrono::duration<Rep, Period> D;

  auto timeout = rel_time;
  auto endTime = (rel_time == D::zero())
                 ? std::chrono::steady_clock::time_point()
                 : (std::chrono::steady_clock::now() + rel_time);
  do
  {
    auto t = d->Tracked();
    if (!t)
    { /* if ServiceTracker is not open */
      return std::shared_ptr<TrackedParmType>();
    }

    {
      auto l = t->Lock();
      if (t->Size_unlocked() == 0)
      {
        t->WaitFor(l, rel_time,  [&t]{ return t->Size_unlocked() > 0 || t->closed; });
      }
    }
    object = GetService();
    // Adapt the timeout in case we "missed" the object after having
    // been notified within the timeout.
    if (!object && endTime > std::chrono::steady_clock::time_point())
    {
      timeout = std::chrono::duration_cast<D>(endTime - std::chrono::steady_clock::now());
      if (timeout.count() <= 0) break; // timed out
    }
  } while (!object);

  return object;
}

template<class S, class T>
std::vector<ServiceReference<S>>
ServiceTracker<S,T>::GetServiceReferences() const
{
  std::vector<ServiceReference<S>> refs;
  auto t = d->Tracked();
  if (!t)
  { /* if ServiceTracker is not open */
    return refs;
  }
  {
    auto l = t->Lock(); US_UNUSED(l);
    d->GetServiceReferences_unlocked(refs, t.get());
  }
  return refs;
}

template<class S, class T>
ServiceReference<S>
ServiceTracker<S,T>::GetServiceReference() const
{
  ServiceReference<S> reference = d->cachedReference.Load();
  if (reference.GetBundle())
  {
    DIAG_LOG(*d->context.GetLogSink()) << "ServiceTracker<S,TTT>::getServiceReference[cached]:"
                  << d->filter;
    return reference;
  }
  DIAG_LOG(*d->context.GetLogSink()) << "ServiceTracker<S,TTT>::getServiceReference:" << d->filter;
  auto references = GetServiceReferences();
  std::size_t length = references.size();
  if (length == 0)
  { /* if no service is being tracked */
    throw ServiceException("No service is being tracked");
  }
  auto selectedRef = references.begin();
  if (length > 1)
  { /* if more than one service, select highest ranking */
    std::vector<int> rankings(length);
    int count = 0;
    int maxRanking = (std::numeric_limits<int>::min)();
    auto refIter = references.begin();
    for (std::size_t i = 0; i < length; i++)
    {
      Any rankingAny = refIter->GetProperty(Constants::SERVICE_RANKING);
      int ranking = 0;
      if (rankingAny.Type() == typeid(int))
      {
        ranking = any_cast<int>(rankingAny);
      }

      rankings[i] = ranking;
      if (ranking > maxRanking)
      {
        selectedRef = refIter;
        maxRanking = ranking;
        count = 1;
      }
      else
      {
        if (ranking == maxRanking)
        {
          count++;
        }
      }
      ++refIter;
    }
    if (count > 1)
    { /* if still more than one service, select lowest id */
      long int minId = (std::numeric_limits<long int>::max)();
      refIter = references.begin();
      for (std::size_t i = 0; i < length; i++)
      {
        if (rankings[i] == maxRanking)
        {
          Any idAny = refIter->GetProperty(Constants::SERVICE_ID);
          long int id = 0;
          if (idAny.Type() == typeid(long int))
          {
            id = any_cast<long int>(idAny);
          }
          if (id < minId)
          {
            selectedRef = refIter;
            minId = id;
          }
        }
        ++refIter;
      }
    }
  }

  d->cachedReference.Store(*selectedRef);
  return *selectedRef;
}

template<class S, class T>
std::shared_ptr<typename ServiceTracker<S,T>::TrackedParmType>
ServiceTracker<S,T>::GetService(const ServiceReference<S>& reference) const
{
  auto t = d->Tracked();
  if (!t)
  { /* if ServiceTracker is not open */
    return std::shared_ptr<TrackedParmType>();
  }
  return (t->Lock(), t->GetCustomizedObject_unlocked(reference));
}

template<class S, class T>
std::vector<std::shared_ptr<typename ServiceTracker<S,T>::TrackedParmType>> ServiceTracker<S,T>::GetServices() const
{
  std::vector<std::shared_ptr<TrackedParmType>> services;
  auto t = d->Tracked();
  if (!t)
  { /* if ServiceTracker is not open */
    return services;
  }
  {
    auto l = t->Lock(); US_UNUSED(l);
    std::vector<ServiceReference<S>> references;
    d->GetServiceReferences_unlocked(references, t.get());
    for(auto& ref : references)
    {
      services.push_back(t->GetCustomizedObject_unlocked(ref));
    }
  }
  return services;
}

template<class S, class T>
std::shared_ptr<typename ServiceTracker<S,T>::TrackedParmType>
ServiceTracker<S,T>::GetService() const
{
  auto service = d->cachedService.Load();
  if (service)
  {
    DIAG_LOG(*d->context.GetLogSink()) << "ServiceTracker<S,TTT>::getService[cached]:"
                  << d->filter;
    return service;
  }
  DIAG_LOG(*d->context.GetLogSink()) << "ServiceTracker<S,TTT>::getService:" << d->filter;

  try
  {
    auto reference = GetServiceReference();
    if (!reference.GetBundle())
    {
      return std::shared_ptr<TrackedParmType>();
    }
    service = GetService(reference);
    d->cachedService.Store(service);
    return service;
  }
  catch (const ServiceException&)
  {
    return std::shared_ptr<TrackedParmType>();
  }
}

template<class S, class T>
void ServiceTracker<S,T>::Remove(const ServiceReference<S>& reference)
{
  auto t = d->Tracked();
  if (!t)
  { /* if ServiceTracker is not open */
    return;
  }
  t->Untrack(reference, ServiceEvent());
}

template<class S, class T>
int ServiceTracker<S,T>::Size() const
{
  auto t = d->Tracked();
  if (!t)
  { /* if ServiceTracker is not open */
    return 0;
  }
  return (t->Lock(), static_cast<int>(t->Size_unlocked()));
}

template<class S, class T>
int ServiceTracker<S,T>::GetTrackingCount() const
{
  auto t = d->Tracked();
  if (!t)
  { /* if ServiceTracker is not open */
    return -1;
  }
  return (t->Lock(), t->GetTrackingCount());
}

template<class S, class T>
void ServiceTracker<S,T>::GetTracked(TrackingMap& map) const
{
  auto t = d->Tracked();
  if (!t)
  { /* if ServiceTracker is not open */
    return;
  }
  t->Lock(), t->CopyEntries_unlocked(map);
}

template<class S, class T>
bool ServiceTracker<S,T>::IsEmpty() const
{
  auto t = d->Tracked();
  if (!t)
  { /* if ServiceTracker is not open */
    return true;
  }
  return (t->Lock(), t->IsEmpty_unlocked());
}

template<class S, class T>
std::shared_ptr<typename ServiceTracker<S,T>::TrackedParmType>
ServiceTracker<S,T>::AddingService(const ServiceReference<S>& reference)
{
  return TypeTraits::ConvertToTrackedType(d->context.GetService(reference));
}

template<class S, class T>
void ServiceTracker<S,T>::ModifiedService(const ServiceReference<S>& /*reference*/, const std::shared_ptr<TrackedParmType>& /*service*/)
{
  /* do nothing */
}

template<class S, class T>
void ServiceTracker<S,T>::RemovedService(const ServiceReference<S>& /*reference*/, const std::shared_ptr<TrackedParmType>& /*service*/)
{
  /* do nothing */
}

} // namespace cppmicroservices
