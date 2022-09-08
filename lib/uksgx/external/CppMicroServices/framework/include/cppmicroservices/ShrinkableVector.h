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

#ifndef CPPMICROSERVICES_SHRINKABLEVECTOR_H
#define CPPMICROSERVICES_SHRINKABLEVECTOR_H

#include "cppmicroservices/GlobalConfig.h"

#include <vector>

namespace cppmicroservices {

/**
 * \ingroup MicroServicesUtils
 *
 * A std::vector style container allowing query and removal
 * operations only.
 */
template<class E>
class ShrinkableVector
{
private:
  static std::vector<E> emptyVector;

public:
  typedef std::vector<E> container_type;
  typedef typename container_type::iterator iterator;
  typedef typename container_type::const_iterator const_iterator;
  typedef typename container_type::size_type size_type;
  typedef typename container_type::reference reference;
  typedef typename container_type::const_reference const_reference;
  typedef typename container_type::value_type value_type;

  ShrinkableVector()
    : container(emptyVector)
  {}

  iterator begin() { return container.begin(); }

  const_iterator begin() const { return container.begin(); }

  iterator end() { return container.end(); }

  const_iterator end() const { return container.end(); }

  reference front() { return container.front(); }

  const_reference front() const { return container.front(); }

  reference back() { return container.back(); }

  const_reference back() const { return container.back(); }

  iterator erase(iterator pos) { return container.erase(pos); }

  iterator erase(iterator first, iterator last)
  {
    return container.erase(first, last);
  }

  void pop_back() { container.pop_back(); }

  bool empty() const { return container.empty(); }

  void clear() { container.clear(); }

  size_type size() const { return container.size(); }

  reference at(size_type pos) { return container.at(pos); }

  const_reference at(size_type pos) const { return container.at(pos); }

  /**
   * \rststar
   * .. deprecated:: 3.1.0
   *
   *    This function exists only to maintain backwards compatibility
   *    and will be removed in the next major release.
   *    Use :any:`at(size_type pos) <cppmicroservices::ShrinkableVector::at>`
   *    instead.
   * \endrststar
   */
  US_DEPRECATED const_reference operator[](size_type i) const
  {
    return container[i];
  }

  /**
   * \rststar
   * .. deprecated:: 3.1.0
   *
   *    This function exists only to maintain backwards compatibility
   *    and will be removed in the next major release.
   *    Use :any:`at(size_type pos) <cppmicroservices::ShrinkableVector::at>`
   *    instead.
   * \endrststar
   */
  US_DEPRECATED reference operator[](size_type i) { return container[i]; }

private:
  friend class BundleHooks;
  friend class ServiceHooks;

  ShrinkableVector(container_type& container)
    : container(container)
  {}

  container_type& container;
};

template<class E>
std::vector<E> ShrinkableVector<E>::emptyVector;
}

#endif // CPPMICROSERVICES_SHRINKABLEVECTOR_H
