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

#ifndef CPPMICROSERVICES_BUNDLEABSTRACTTRACKED_H
#define CPPMICROSERVICES_BUNDLEABSTRACTTRACKED_H

#include "cppmicroservices/Any.h"
#include "cppmicroservices/detail/Threads.h"
#include "cppmicroservices/detail/WaitCondition.h"

#include <atomic>
#include <vector>

namespace cppmicroservices {

namespace detail {

/**
 * This class is not intended to be used directly. It is exported to support
 * the CppMicroServices bundle system.
 *
 * Abstract class to track items. If a Tracker is reused (closed then reopened),
 * then a new BundleAbstractTracked object is used. This class acts as a map of tracked
 * item -> customized object. Subclasses of this class will act as the listener
 * object for the tracker. This class is used to synchronize access to the
 * tracked items. This is not a public class. It is only for use by the
 * implementation of the Tracker class.
 *
 * @tparam S The tracked item. It is the key.
 * @tparam TTT Type traits with TTT::TrackedType representing the value type mapped to the tracked item.
 * @tparam R The reason the tracked item is  being tracked or untracked.
 * @remarks This class is thread safe.
 */
template<class S, class TTT, class R>
class BundleAbstractTracked
  : public MultiThreaded<MutexLockingStrategy<>, WaitCondition>
{

public:
  typedef typename TTT::TrackedType T;
  typedef typename TTT::TrackedParmType TrackedParmType;

  typedef std::map<S, std::shared_ptr<TrackedParmType>> TrackingMap;

  /**
   * BundleAbstractTracked constructor.
   */
  BundleAbstractTracked(BundleContext* bc);

  virtual ~BundleAbstractTracked();

  /**
   * Set initial list of items into tracker before events begin to be
   * received.
   *
   * This method must be called from Tracker's open method while synchronized
   * on this object in the same synchronized block as the add listener call.
   *
   * @param list The initial list of items to be tracked. <code>null</code>
   *        entries in the list are ignored.
   * @GuardedBy this
   */
  void SetInitial(const std::vector<S>& list);

  /**
   * Track the initial list of items. This is called after events can begin to
   * be received.
   *
   * This method must be called from Tracker's open method while not
   * synchronized on this object after the add listener call.
   *
   */
  void TrackInitial();

  /**
   * Called by the owning Tracker object when it is closed.
   */
  void Close();

  /**
   * Begin to track an item.
   *
   * @param item S to be tracked.
   * @param related Action related object.
   */
  void Track(S item, R related);

  /**
   * Discontinue tracking the item.
   *
   * @param item S to be untracked.
   * @param related Action related object.
   */
  void Untrack(S item, R related);

  /**
   * Returns the number of tracked items.
   *
   * @return The number of tracked items.
   *
   * @GuardedBy this
   */
  std::size_t Size_unlocked() const;

  /**
   * Returns if the tracker is empty.
   *
   * @return Whether the tracker is empty.
   *
   * @GuardedBy this
   */
  bool IsEmpty_unlocked() const;

  /**
   * Return the customized object for the specified item
   *
   * @param item The item to lookup in the map
   * @return The customized object for the specified item.
   *
   * @GuardedBy this
   */
  std::shared_ptr<TrackedParmType> GetCustomizedObject_unlocked(S item) const;

  /**
   * Return the list of tracked items.
   *
   * @return The tracked items.
   * @GuardedBy this
   */
  void GetTracked_unlocked(std::vector<S>& items) const;

  /**
   * Increment the modification count. If this method is overridden, the
   * overriding method MUST call this method to increment the tracking count.
   *
   * @GuardedBy this
   */
  virtual void Modified();

  /**
   * Returns the tracking count for this <code>ServiceTracker</code> object.
   *
   * The tracking count is initialized to 0 when this object is opened. Every
   * time an item is added, modified or removed from this object the tracking
   * count is incremented.
   *
   * @GuardedBy this
   * @return The tracking count for this object.
   */
  int GetTrackingCount() const;

  /**
   * Copy the tracked items and associated values into the specified map.
   *
   * @param map The map into which to copy the tracked items and associated
   *        values. This map must not be a user provided map so that user code
   *        is not executed while synchronized on this.
   * @return The specified map.
   * @GuardedBy this
   */
  void CopyEntries_unlocked(TrackingMap& map) const;

  /**
   * Call the specific customizer adding method. This method must not be
   * called while synchronized on this object.
   *
   * @param item S to be tracked.
   * @param related Action related object.
   * @return Customized object for the tracked item or <code>null</code> if
   *         the item is not to be tracked.
   */
  virtual std::shared_ptr<TrackedParmType> CustomizerAdding(
    S item,
    const R& related) = 0;

  /**
   * Call the specific customizer modified method. This method must not be
   * called while synchronized on this object.
   *
   * @param item Tracked item.
   * @param related Action related object.
   * @param object Customized object for the tracked item.
   */
  virtual void CustomizerModified(
    S item,
    const R& related,
    const std::shared_ptr<TrackedParmType>& object) = 0;

  /**
   * Call the specific customizer removed method. This method must not be
   * called while synchronized on this object.
   *
   * @param item Tracked item.
   * @param related Action related object.
   * @param object Customized object for the tracked item.
   */
  virtual void CustomizerRemoved(
    S item,
    const R& related,
    const std::shared_ptr<TrackedParmType>& object) = 0;

  /**
   * List of items in the process of being added. This is used to deal with
   * nesting of events. Since events may be synchronously delivered, events
   * can be nested. For example, when processing the adding of a service and
   * the customizer causes the service to be unregistered, notification to the
   * nested call to untrack that the service was unregistered can be made to
   * the track method.
   *
   * Since the std::vector implementation is not synchronized, all access to
   * this list must be protected by the same synchronized object for
   * thread-safety.
   *
   * @GuardedBy this
   */
  std::list<S> adding;

  /**
   * true if the tracked object is closed.
   */
  std::atomic<bool> closed;

  /**
   * Initial list of items for the tracker. This is used to correctly process
   * the initial items which could be modified before they are tracked. This
   * is necessary since the initial set of tracked items are not "announced"
   * by events and therefore the event which makes the item untracked could be
   * delivered before we track the item.
   *
   * An item must not be in both the initial and adding lists at the same
   * time. An item must be moved from the initial list to the adding list
   * "atomically" before we begin tracking it.
   *
   * Since the LinkedList implementation is not synchronized, all access to
   * this list must be protected by the same synchronized object for
   * thread-safety.
   *
   * @GuardedBy this
   */
  std::list<S> initial;

  /**
   * Common logic to add an item to the tracker used by track and
   * trackInitial. The specified item must have been placed in the adding list
   * before calling this method.
   *
   * @param item S to be tracked.
   * @param related Action related object.
   */
  void TrackAdding(S item, R related);

private:
  typedef BundleAbstractTracked<S, TTT, R> Self;

  /**
   * Map of tracked items to customized objects.
   *
   * @GuardedBy this
   */
  TrackingMap tracked;

  /**
   * Modification count. This field is initialized to zero and incremented by
   * modified.
   */
  std::atomic<int> trackingCount;

  BundleContext* const bc;

  bool CustomizerAddingFinal(S item,
                             const std::shared_ptr<TrackedParmType>& custom);
};

} // namespace detail

} // namespace cppmicroservices

#include "cppmicroservices/detail/BundleAbstractTracked.tpp"

#endif // CPPMICROSERVICES_BUNDLEABSTRACTTRACKED_H
