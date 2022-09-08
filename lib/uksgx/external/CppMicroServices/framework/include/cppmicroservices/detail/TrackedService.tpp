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

namespace cppmicroservices {

namespace detail {

template<class S, class TTT>
TrackedService<S,TTT>::TrackedService(ServiceTracker<S,T>* serviceTracker,
                  ServiceTrackerCustomizer<S,T>* customizer)
  : Superclass(&serviceTracker->d->context), serviceTracker(serviceTracker), customizer(customizer)
{

}

template<class S, class TTT>
void TrackedService<S,TTT>::ServiceChanged(const ServiceEvent& event)
{
  /*
   * Check if we had a delayed call (which could happen when we
   * close).
   */
  if (this->closed)
  {
    return;
  }

  ServiceReference<S> reference = event.GetServiceReference<S>();

  DIAG_LOG(*serviceTracker->d->context.GetLogSink()) << "TrackedService::ServiceChanged["
                                                    << event.GetType() << "]: " << reference;
  if (!reference)
  {
    return;
  }

  switch (event.GetType())
  {
  case ServiceEvent::SERVICE_REGISTERED :
  case ServiceEvent::SERVICE_MODIFIED :
    {
      if (!serviceTracker->d->listenerFilter.empty())
      { // service listener added with filter
        this->Track(reference, event);
        /*
       * If the customizer throws an unchecked exception, it
       * is safe to let it propagate
       */
      }
      else
      { // service listener added without filter
        if (serviceTracker->d->filter.Match(reference))
        {
          this->Track(reference, event);
          /*
         * If the customizer throws an unchecked exception,
         * it is safe to let it propagate
         */
        }
        else
        {
          this->Untrack(reference, event);
          /*
         * If the customizer throws an unchecked exception,
         * it is safe to let it propagate
         */
        }
      }
      break;
    }
  case ServiceEvent::SERVICE_MODIFIED_ENDMATCH :
  case ServiceEvent::SERVICE_UNREGISTERING :
    this->Untrack(reference, event);
    /*
     * If the customizer throws an unchecked exception, it is
     * safe to let it propagate
     */
    break;
  }
}

template<class S, class TTT>
void TrackedService<S,TTT>::Modified()
{
  Superclass::Modified(); /* increment the modification count */
  serviceTracker->d->Modified();
}

template<class S, class TTT>
std::shared_ptr<typename TrackedService<S,TTT>::TrackedParmType>
TrackedService<S,TTT>::CustomizerAdding(ServiceReference<S> item,
                                        const ServiceEvent& /*related*/)
{
  return customizer->AddingService(item);
}

template<class S, class TTT>
void TrackedService<S,TTT>::CustomizerModified(ServiceReference<S> item,
                                               const ServiceEvent& /*related*/,
                                               const std::shared_ptr<TrackedParmType>& object)
{
  customizer->ModifiedService(item, object);
}

template<class S, class TTT>
void TrackedService<S,TTT>::CustomizerRemoved(ServiceReference<S> item,
                                              const ServiceEvent& /*related*/,
                                              const std::shared_ptr<TrackedParmType>& object)
{
  customizer->RemovedService(item, object);
}

} // namespace detail

} // namespace cppmicroservices
