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

#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/detail/Log.h"

#include <iterator>

namespace cppmicroservices {

namespace detail {

template<class S, class TTT, class R>
BundleAbstractTracked<S,TTT,R>::BundleAbstractTracked(BundleContext* bc)
  : closed(false), bc(bc)
{
}

template<class S, class TTT, class R>
BundleAbstractTracked<S,TTT,R>::~BundleAbstractTracked()
{

}

template<class S, class TTT, class R>
void BundleAbstractTracked<S,TTT,R>::SetInitial(const std::vector<S>& initiallist)
{
  std::copy(initiallist.begin(), initiallist.end(), std::back_inserter(initial));

  if (bc->GetLogSink()->Enabled())
  {
    for(typename std::list<S>::const_iterator item = initial.begin();
      item != initial.end(); ++item)
    {
      DIAG_LOG(*bc->GetLogSink()) << "BundleAbstractTracked::setInitial: " << (*item);
    }
  }
}

template<class S, class TTT, class R>
void BundleAbstractTracked<S,TTT,R>::TrackInitial()
{
  while (true)
  {
    S item;
    {
      auto l = this->Lock(); US_UNUSED(l);
      if (closed || (initial.size() == 0))
      {
        /*
         * if there are no more initial items
         */
        return; /* we are done */
      }
      /*
       * move the first item from the initial list to the adding list
       * within this synchronized block.
       */
      item = initial.front();
      initial.pop_front();
      if (tracked[item])
      {
        /* if we are already tracking this item */
        DIAG_LOG(*bc->GetLogSink()) << "BundleAbstractTracked::trackInitial[already tracked]: " << item;
        continue; /* skip this item */
      }
      if (std::find(adding.begin(), adding.end(), item) != adding.end())
      {
        /*
         * if this item is already in the process of being added.
         */
        DIAG_LOG(*bc->GetLogSink()) << "BundleAbstractTracked::trackInitial[already adding]: " << item;
        continue; /* skip this item */
      }
      adding.push_back(item);
    }
    DIAG_LOG(*bc->GetLogSink()) << "BundleAbstractTracked::trackInitial: " << item;
    TrackAdding(item, R());
    /*
     * Begin tracking it. We call trackAdding
     * since we have already put the item in the
     * adding list.
     */
  }
}

template<class S, class TTT, class R>
void BundleAbstractTracked<S,TTT,R>::Close()
{
  closed = true;
}

template<class S, class TTT, class R>
void BundleAbstractTracked<S,TTT,R>::Track(S item, R related)
{
  std::shared_ptr<TrackedParmType> object;
  {
    auto l = this->Lock(); US_UNUSED(l);
    if (closed)
    {
      return;
    }
    object = tracked[item];
    if (!object)
    { /* we are not tracking the item */
      if (std::find(adding.begin(), adding.end(),item) != adding.end())
      {
        /* if this item is already in the process of being added. */
        DIAG_LOG(*bc->GetLogSink()) << "BundleAbstractTracked::track[already adding]: " << item;
        return;
      }
      adding.push_back(item); /* mark this item is being added */
    }
    else
    { /* we are currently tracking this item */
      DIAG_LOG(*bc->GetLogSink()) << "BundleAbstractTracked::track[modified]: " << item;
      Modified(); /* increment modification count */
    }
  }

  if (!object)
  { /* we are not tracking the item */
    TrackAdding(item, related);
  }
  else
  {
    /* Call customizer outside of synchronized region */
    CustomizerModified(item, related, object);
    /*
     * If the customizer throws an unchecked exception, it is safe to
     * let it propagate
     */
  }
}

template<class S, class TTT, class R>
void BundleAbstractTracked<S,TTT,R>::Untrack(S item, R related)
{
  std::shared_ptr<TrackedParmType> object;
  {
    auto l = this->Lock(); US_UNUSED(l);
    std::size_t initialSize = initial.size();
    initial.remove(item);
    if (initialSize != initial.size())
    { /* if this item is already in the list
       * of initial references to process
       */
      DIAG_LOG(*bc->GetLogSink()) << "BundleAbstractTracked::untrack[removed from initial]: " << item;
      return; /* we have removed it from the list and it will not be
               * processed
               */
    }

    std::size_t addingSize = adding.size();
    adding.remove(item);
    if (addingSize != adding.size())
    { /* if the item is in the process of
       * being added
       */
      DIAG_LOG(*bc->GetLogSink()) << "BundleAbstractTracked::untrack[being added]: " << item;
      return; /*
           * in case the item is untracked while in the process of
           * adding
           */
    }
    object = tracked[item];
    /*
     * must remove from tracker before
     * calling customizer callback
     */
    tracked.erase(item);
    if (!object)
    { /* are we actually tracking the item */
      return;
    }
    Modified(); /* increment modification count */
  }
  DIAG_LOG(*bc->GetLogSink()) << "BundleAbstractTracked::untrack[removed]: " << item;
  /* Call customizer outside of synchronized region */
  CustomizerRemoved(item, related, object);
  /*
   * If the customizer throws an unchecked exception, it is safe to let it
   * propagate
   */
}

template<class S, class TTT, class R>
std::size_t BundleAbstractTracked<S,TTT,R>::Size_unlocked() const
{
  return tracked.size();
}

template<class S, class TTT, class R>
bool BundleAbstractTracked<S,TTT,R>::IsEmpty_unlocked() const
{
  return tracked.empty();
}

template<class S, class TTT, class R>
std::shared_ptr<typename BundleAbstractTracked<S,TTT,R>::TrackedParmType>
BundleAbstractTracked<S,TTT,R>::GetCustomizedObject_unlocked(S item) const
{
  typename TrackingMap::const_iterator i = tracked.find(item);
  if (i != tracked.end()) return i->second;
  return std::shared_ptr<TrackedParmType>();
}

template<class S, class TTT, class R>
void BundleAbstractTracked<S,TTT,R>::GetTracked_unlocked(std::vector<S>& items) const
{
  for (auto& i : tracked)
  {
    items.push_back(i.first);
  }
}

template<class S, class TTT, class R>
void BundleAbstractTracked<S,TTT,R>::Modified()
{
  // atomic
  ++trackingCount;
}

template<class S, class TTT, class R>
int BundleAbstractTracked<S,TTT,R>::GetTrackingCount() const
{
  // atomic
  return trackingCount;
}

template<class S, class TTT, class R>
void BundleAbstractTracked<S,TTT,R>::CopyEntries_unlocked(TrackingMap& map) const
{
  map.insert(tracked.begin(), tracked.end());
}

template<class S, class TTT, class R>
bool BundleAbstractTracked<S,TTT,R>::CustomizerAddingFinal(S item, const std::shared_ptr<TrackedParmType>& custom)
{
  auto l = this->Lock(); US_UNUSED(l);
  std::size_t addingSize = adding.size();
  adding.remove(item);
  if (addingSize != adding.size() && !closed)
  {
    /*
     * if the item was not untracked during the customizer
     * callback
     */
    if (custom)
    {
      tracked[item] = custom;
      Modified(); /* increment modification count */
      this->NotifyAll(); /* notify any waiters */
    }
    return false;
  }
  else
  {
    return true;
  }
}

template<class S, class TTT, class R>
void BundleAbstractTracked<S,TTT,R>::TrackAdding(S item, R related)
{
  DIAG_LOG(*bc->GetLogSink()) << "BundleAbstractTracked::trackAdding:" << item;
  std::shared_ptr<TrackedParmType> object;
  bool becameUntracked = false;
  /* Call customizer outside of synchronized region */
  try
  {
    object = CustomizerAdding(item, related);
    becameUntracked = this->CustomizerAddingFinal(item, object);
  }
  catch (...)
  {
    /*
     * If the customizer throws an exception, it will
     * propagate after the cleanup code.
     */
    this->CustomizerAddingFinal(item, object);
    throw;
  }

  /*
   * The item became untracked during the customizer callback.
   */
  if (becameUntracked && object)
  {
    DIAG_LOG(*bc->GetLogSink()) << "BundleAbstractTracked::trackAdding[removed]: " << item;
    /* Call customizer outside of synchronized region */
    CustomizerRemoved(item, related, object);
    /*
     * If the customizer throws an unchecked exception, it is safe to
     * let it propagate
     */
  }
}

} // namespace detail

} // namespace cppmicroservices
