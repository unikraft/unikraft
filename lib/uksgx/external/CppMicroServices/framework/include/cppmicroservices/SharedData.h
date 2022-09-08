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


Modified version of qshareddata.h from Qt 4.7.3 for CppMicroServices.
Original copyright (c) Nokia Corporation. Usage covered by the
GNU Lesser General Public License version 2.1
(http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html) and the Nokia Qt
LGPL Exception version 1.1 (file LGPL_EXCEPTION.txt in Qt 4.7.3 package).

=========================================================================*/

#ifndef CPPMICROSERVICES_SHAREDDATA_H
#define CPPMICROSERVICES_SHAREDDATA_H

#include "cppmicroservices/GlobalConfig.h"

#include <algorithm>
#include <atomic>
#include <utility>

namespace cppmicroservices {

/**
\defgroup gr_shareddata SharedData

\brief Groups SharedData related symbols.
*/

/**
 * \ingroup MicroServicesUtils
 * \ingroup gr_shareddata
 */
class SharedData
{
public:
  mutable std::atomic<int> ref;

  inline SharedData()
    : ref(0)
  {}
  inline SharedData(const SharedData&)
    : ref(0)
  {}

  // using the assignment operator would lead to corruption in the ref-counting
  SharedData& operator=(const SharedData&) = delete;
};

/**
 * \ingroup MicroServicesUtils
 * \ingroup gr_shareddata
 */
template<class T>
class SharedDataPointer
{
public:
  typedef T Type;
  typedef T* pointer;

  inline void Detach()
  {
    if (d && d->ref != 1)
      Detach_helper();
  }
  inline T& operator*()
  {
    Detach();
    return *d;
  }
  inline const T& operator*() const { return *d; }
  inline T* operator->()
  {
    Detach();
    return d;
  }
  inline const T* operator->() const { return d; }
  inline operator T*()
  {
    Detach();
    return d;
  }
  inline operator const T*() const { return d; }
  inline T* Data()
  {
    Detach();
    return d;
  }
  inline const T* Data() const { return d; }
  inline const T* ConstData() const { return d; }

  inline bool operator==(const SharedDataPointer<T>& other) const
  {
    return d == other.d;
  }
  inline bool operator!=(const SharedDataPointer<T>& other) const
  {
    return d != other.d;
  }

  inline SharedDataPointer()
    : d(0)
  {}
  inline ~SharedDataPointer()
  {
    if (d && !--d->ref)
      delete d;
  }

  explicit SharedDataPointer(T* data);
  inline SharedDataPointer(const SharedDataPointer<T>& o)
    : d(o.d)
  {
    if (d)
      ++d->ref;
  }

  inline SharedDataPointer<T>& operator=(const SharedDataPointer<T>& o)
  {
    if (o.d != d) {
      if (o.d)
        ++o.d->ref;
      T* old = d;
      d = o.d;
      if (old && !--old->ref)
        delete old;
    }
    return *this;
  }

  inline SharedDataPointer& operator=(T* o)
  {
    if (o != d) {
      if (o)
        ++o->ref;
      T* old = d;
      d = o;
      if (old && !--old->ref)
        delete old;
    }
    return *this;
  }

  inline bool operator!() const { return !d; }

  inline void Swap(SharedDataPointer& other)
  {
    using std::swap;
    swap(d, other.d);
  }

protected:
  T* Clone();

private:
  void Detach_helper();

  T* d;
};

/**
 * \ingroup MicroServicesUtils
 * \ingroup gr_shareddata
 */
template<class T>
class ExplicitlySharedDataPointer
{
public:
  typedef T Type;
  typedef T* pointer;

  inline T& operator*() const { return *d; }
  inline T* operator->() { return d; }
  inline T* operator->() const { return d; }
  inline T* Data() const { return d; }
  inline const T* ConstData() const { return d; }

  inline void Detach()
  {
    if (d && d->ref != 1)
      Detach_helper();
  }

  inline void Reset()
  {
    if (d && !--d->ref)
      delete d;

    d = nullptr;
  }

  inline operator bool() const { return d != nullptr; }

  inline bool operator==(const ExplicitlySharedDataPointer<T>& other) const
  {
    return d == other.d;
  }
  inline bool operator!=(const ExplicitlySharedDataPointer<T>& other) const
  {
    return d != other.d;
  }
  inline bool operator==(const T* ptr) const { return d == ptr; }
  inline bool operator!=(const T* ptr) const { return d != ptr; }

  inline ExplicitlySharedDataPointer() { d = nullptr; }
  inline ~ExplicitlySharedDataPointer()
  {
    if (d && !--d->ref)
      delete d;
  }

  explicit ExplicitlySharedDataPointer(T* data);
  inline ExplicitlySharedDataPointer(const ExplicitlySharedDataPointer<T>& o)
    : d(o.d)
  {
    if (d)
      ++d->ref;
  }

  template<class X>
  inline ExplicitlySharedDataPointer(const ExplicitlySharedDataPointer<X>& o)
    : d(static_cast<T*>(o.Data()))
  {
    if (d)
      ++d->ref;
  }

  inline ExplicitlySharedDataPointer<T>& operator=(
    const ExplicitlySharedDataPointer<T>& o)
  {
    if (o.d != d) {
      if (o.d)
        ++o.d->ref;
      T* old = d;
      d = o.d;
      if (old && !--old->ref)
        delete old;
    }
    return *this;
  }

  inline ExplicitlySharedDataPointer& operator=(T* o)
  {
    if (o != d) {
      if (o)
        ++o->ref;
      T* old = d;
      d = o;
      if (old && !--old->ref)
        delete old;
    }
    return *this;
  }

  inline bool operator!() const { return !d; }

  inline void Swap(ExplicitlySharedDataPointer& other)
  {
    using std::swap;
    swap(d, other.d);
  }

protected:
  T* Clone();

private:
  void Detach_helper();

  T* d;
};

template<class T>
SharedDataPointer<T>::SharedDataPointer(T* adata)
  : d(adata)
{
  if (d)
    ++d->ref;
}

template<class T>
T* SharedDataPointer<T>::Clone()
{
  return new T(*d);
}

template<class T>
void SharedDataPointer<T>::Detach_helper()
{
  T* x = Clone();
  ++x->ref;
  if (!--d->ref)
    delete d;
  d = x;
}

template<class T>
T* ExplicitlySharedDataPointer<T>::Clone()
{
  return new T(*d);
}

template<class T>
void ExplicitlySharedDataPointer<T>::Detach_helper()
{
  T* x = Clone();
  ++x->ref;
  if (!--d->ref)
    delete d;
  d = x;
}

template<class T>
ExplicitlySharedDataPointer<T>::ExplicitlySharedDataPointer(T* adata)
  : d(adata)
{
  if (d)
    ++d->ref;
}

template<class T>
void swap(cppmicroservices::SharedDataPointer<T>& p1,
          cppmicroservices::SharedDataPointer<T>& p2)
{
  p1.Swap(p2);
}

template<class T>
void swap(cppmicroservices::ExplicitlySharedDataPointer<T>& p1,
          cppmicroservices::ExplicitlySharedDataPointer<T>& p2)
{
  p1.Swap(p2);
}
}

#endif // CPPMICROSERVICES_SHAREDDATA_H
