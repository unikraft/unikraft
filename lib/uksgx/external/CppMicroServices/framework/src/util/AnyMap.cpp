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

#include "cppmicroservices/AnyMap.h"

#include <stdexcept>

namespace cppmicroservices {

namespace detail {

std::size_t any_map_cihash::operator()(const std::string& key) const
{
  std::size_t h = 0;
  std::for_each(key.begin(), key.end(), [&h](char c) { h += tolower(c); });
  return h;
}

bool any_map_ciequal::operator()(const std::string& l,
                                 const std::string& r) const
{
  return l.size() == r.size() &&
         std::equal(l.begin(), l.end(), r.begin(), [](char a, char b) {
           return tolower(a) == tolower(b);
         });
}

const Any& AtCompoundKey(const std::vector<Any>& v,
                         const AnyMap::key_type& key);

const Any& AtCompoundKey(const AnyMap& m, const AnyMap::key_type& key)
{
  auto pos = key.find(".");
  if (pos != AnyMap::key_type::npos) {
    auto head = key.substr(0, pos);
    auto tail = key.substr(pos + 1);

    auto& h = m.at(head);
    if (h.Type() == typeid(AnyMap)) {
      return AtCompoundKey(ref_any_cast<AnyMap>(h), tail);
    } else if (h.Type() == typeid(std::vector<Any>)) {
      return AtCompoundKey(ref_any_cast<std::vector<Any>>(h), tail);
    }
    throw std::invalid_argument("Unsupported Any type at '" + head +
                                "' for dotted get");
  } else {
    return m.at(key);
  }
}

const Any& AtCompoundKey(const std::vector<Any>& v, const AnyMap::key_type& key)
{
  auto pos = key.find(".");
  if (pos != AnyMap::key_type::npos) {
    auto head = key.substr(0, pos);
    auto tail = key.substr(pos + 1);

    const int index = std::stoi(head);
    auto& h = v.at(index < 0 ? v.size() + index : index);

    if (h.Type() == typeid(AnyMap)) {
      return AtCompoundKey(ref_any_cast<AnyMap>(h), tail);
    } else if (h.Type() == typeid(std::vector<Any>)) {
      return AtCompoundKey(ref_any_cast<std::vector<Any>>(h), tail);
    }
    throw std::invalid_argument("Unsupported Any type at '" + head +
                                "' for dotted get");
  } else {
    const int index = std::stoi(key);
    return v.at(index < 0 ? v.size() + index : index);
  }
}
}

// ----------------------------------------------------------------
// ------------------  any_map::const_iterator  -------------------

any_map::const_iter::const_iter() {}

any_map::const_iter::const_iter(const any_map::const_iter& it)
  : iterator_base(it.type)
{
  switch (type) {
    case ORDERED:
      this->it.o = new ociter(it.o_it());
      break;
    case UNORDERED:
      this->it.uo = new uociter(it.uo_it());
      break;
    case UNORDERED_CI:
      this->it.uoci = new uocciiter(it.uoci_it());
      break;
    case NONE:
      break;
    default:
      throw std::logic_error("invalid iterator type");
  }
}

any_map::const_iter::const_iter(const any_map::iterator& it)
  : iterator_base(it.type)
{
  switch (type) {
    case ORDERED:
      this->it.o = new ociter(it.o_it());
      break;
    case UNORDERED:
      this->it.uo = new uociter(it.uo_it());
      break;
    case UNORDERED_CI:
      this->it.uoci = new uocciiter(it.uoci_it());
      break;
    case NONE:
      break;
    default:
      throw std::logic_error("invalid iterator type");
  }
}

any_map::const_iter::~const_iter()
{
  switch (type) {
    case ORDERED:
      delete it.o;
      break;
    case UNORDERED:
      delete it.uo;
      break;
    case UNORDERED_CI:
      delete it.uoci;
      break;
    case NONE:
      break;
  }
}

any_map::const_iter::const_iter(ociter&& it)
  : iterator_base(ORDERED)
{
  this->it.o = new ociter(std::move(it));
}

any_map::const_iter::const_iter(uociter&& it, iter_type type)
  : iterator_base(type)
{
  switch (type) {
    case UNORDERED:
      this->it.uo = new uociter(std::move(it));
      break;
    case UNORDERED_CI:
      this->it.uoci = new uocciiter(std::move(it));
      break;
    default:
      throw std::logic_error("type for unordered_map iterator not supported");
  }
}

any_map::const_iter::reference any_map::const_iter::operator*() const
{
  switch (type) {
    case ORDERED:
      return *o_it();
    case UNORDERED:
      return *uo_it();
    case UNORDERED_CI:
      return *uoci_it();
    case NONE:
      throw std::logic_error("cannot dereference an invalid iterator");
    default:
      throw std::logic_error("invalid iterator type");
  }
}

any_map::const_iter::pointer any_map::const_iter::operator->() const
{
  switch (type) {
    case ORDERED:
      return o_it().operator->();
    case UNORDERED:
      return uo_it().operator->();
    case UNORDERED_CI:
      return uoci_it().operator->();
    case NONE:
      throw std::logic_error("cannot dereference an invalid iterator");
    default:
      throw std::logic_error("invalid iterator type");
  }
}

any_map::const_iter::iterator& any_map::const_iter::operator++()
{
  switch (type) {
    case ORDERED:
      ++o_it();
      break;
    case UNORDERED:
      ++uo_it();
      break;
    case UNORDERED_CI:
      ++uoci_it();
      break;
    case NONE:
      throw std::logic_error("cannot increment an invalid iterator");
    default:
      throw std::logic_error("invalid iterator type");
  }

  return *this;
}

any_map::const_iter::iterator any_map::const_iter::operator++(int)
{
  iterator tmp = *this;
  switch (type) {
    case ORDERED:
      o_it()++;
      break;
    case UNORDERED:
      uo_it()++;
      break;
    case UNORDERED_CI:
      uoci_it()++;
      break;
    case NONE:
      throw std::logic_error("cannot increment an invalid iterator");
    default:
      throw std::logic_error("invalid iterator type");
  }
  return tmp;
}

bool any_map::const_iter::operator==(const iterator& x) const
{
  switch (type) {
    case ORDERED:
      return o_it() == x.o_it();
    case UNORDERED:
      return uo_it() == x.uo_it();
    case UNORDERED_CI:
      return uoci_it() == x.uoci_it();
    case NONE:
      return x.type == NONE;
    default:
      throw std::logic_error("invalid iterator type");
  }
}

bool any_map::const_iter::operator!=(const iterator& x) const
{
  return !this->operator==(x);
}

any_map::const_iter::ociter const& any_map::const_iter::o_it() const
{
  return *it.o;
}

any_map::const_iter::ociter& any_map::const_iter::o_it()
{
  return *it.o;
}

any_map::const_iter::uociter const& any_map::const_iter::uo_it() const
{
  return *it.uo;
}

any_map::const_iter::uociter& any_map::const_iter::uo_it()
{
  return *it.uo;
}

any_map::const_iter::uocciiter const& any_map::const_iter::uoci_it() const
{
  return *it.uoci;
}

any_map::const_iter::uocciiter& any_map::const_iter::uoci_it()
{
  return *it.uoci;
}

// ----------------------------------------------------------------
// ---------------------  AnyMap::iterator  -----------------------

any_map::iter::iter() {}

any_map::iter::iter(const iter& it)
  : iterator_base(it.type)
{
  switch (type) {
    case ORDERED:
      this->it.o = new oiter(it.o_it());
      break;
    case UNORDERED:
      this->it.uo = new uoiter(it.uo_it());
      break;
    case UNORDERED_CI:
      this->it.uoci = new uociiter(it.uoci_it());
      break;
    case NONE:
      break;
    default:
      throw std::logic_error("invalid iterator type");
  }
}

any_map::iter::~iter()
{
  switch (type) {
    case ORDERED:
      delete it.o;
      break;
    case UNORDERED:
      delete it.uo;
      break;
    case UNORDERED_CI:
      delete it.uoci;
      break;
    case NONE:
      break;
  }
}

any_map::iter::iter(oiter&& it)
  : iterator_base(ORDERED)
{
  this->it.o = new oiter(std::move(it));
}

any_map::iter::iter(uoiter&& it, iter_type type)
  : iterator_base(type)
{
  switch (type) {
    case UNORDERED:
      this->it.uo = new uoiter(std::move(it));
      break;
    case UNORDERED_CI:
      this->it.uoci = new uociiter(std::move(it));
      break;
    default:
      throw std::logic_error("type for unordered_map iterator not supported");
  }
}

any_map::iter::reference any_map::iter::operator*() const
{
  switch (type) {
    case ORDERED:
      return *o_it();
    case UNORDERED:
      return *uo_it();
    case UNORDERED_CI:
      return *uoci_it();
    case NONE:
      throw std::logic_error("cannot dereference an invalid iterator");
    default:
      throw std::logic_error("invalid iterator type");
  }
}

any_map::iter::pointer any_map::iter::operator->() const
{
  switch (type) {
    case ORDERED:
      return o_it().operator->();
    case UNORDERED:
      return uo_it().operator->();
    case UNORDERED_CI:
      return uoci_it().operator->();
    case NONE:
      throw std::logic_error("cannot dereference an invalid iterator");
    default:
      throw std::logic_error("invalid iterator type");
  }
}

any_map::iter::iterator& any_map::iter::operator++()
{
  switch (type) {
    case ORDERED:
      ++o_it();
      break;
    case UNORDERED:
      ++uo_it();
      break;
    case UNORDERED_CI:
      ++uoci_it();
      break;
    case NONE:
      throw std::logic_error("cannot increment an invalid iterator");
    default:
      throw std::logic_error("invalid iterator type");
  }

  return *this;
}

any_map::iter::iterator any_map::iter::operator++(int)
{
  iterator tmp = *this;
  switch (type) {
    case ORDERED:
      o_it()++;
      break;
    case UNORDERED:
      uo_it()++;
      break;
    case UNORDERED_CI:
      uoci_it()++;
      break;
    case NONE:
      throw std::logic_error("cannot increment an invalid iterator");
    default:
      throw std::logic_error("invalid iterator type");
  }
  return tmp;
}

bool any_map::iter::operator==(const iterator& x) const
{
  switch (type) {
    case ORDERED:
      return o_it() == x.o_it();
    case UNORDERED:
      return uo_it() == x.uo_it();
    case UNORDERED_CI:
      return uoci_it() == x.uoci_it();
    case NONE:
      return x.type == NONE;
    default:
      throw std::logic_error("invalid iterator type");
  }
}

bool any_map::iter::operator!=(const iterator& x) const
{
  return !this->operator==(x);
}

any_map::iter::oiter const& any_map::iter::o_it() const
{
  return *it.o;
}

any_map::iter::oiter& any_map::iter::o_it()
{
  return *it.o;
}

any_map::iter::uoiter const& any_map::iter::uo_it() const
{
  return *it.uo;
}

any_map::iter::uoiter& any_map::iter::uo_it()
{
  return *it.uo;
}

any_map::iter::uociiter const& any_map::iter::uoci_it() const
{
  return *it.uoci;
}

any_map::iter::uociiter& any_map::iter::uoci_it()
{
  return *it.uoci;
}

// ----------------------------------------------------------
// ------------------------  any_map  -----------------------

any_map::any_map(map_type type)
  : type(type)
{
  switch (type) {
    case map_type::ORDERED_MAP:
      map.o = new ordered_any_map();
      break;
    case map_type::UNORDERED_MAP:
      map.uo = new unordered_any_map();
      break;
    case map_type::UNORDERED_MAP_CASEINSENSITIVE_KEYS:
      map.uoci = new unordered_any_cimap();
      break;
    default:
      throw std::logic_error("invalid map type");
  }
}

any_map::any_map(const ordered_any_map& m)
  : type(map_type::ORDERED_MAP)
{
  map.o = new ordered_any_map(m);
}

any_map::any_map(const unordered_any_map& m)
  : type(map_type::UNORDERED_MAP)
{
  map.uo = new unordered_any_map(m);
}

any_map::any_map(const unordered_any_cimap& m)
  : type(map_type::UNORDERED_MAP_CASEINSENSITIVE_KEYS)
{
  map.uoci = new unordered_any_cimap(m);
}

any_map::any_map(const any_map& m)
  : type(m.type)
{
  switch (type) {
    case map_type::ORDERED_MAP:
      map.o = new ordered_any_map(m.o_m());
      break;
    case map_type::UNORDERED_MAP:
      map.uo = new unordered_any_map(m.uo_m());
      break;
    case map_type::UNORDERED_MAP_CASEINSENSITIVE_KEYS:
      map.uoci = new unordered_any_cimap(m.uoci_m());
      break;
    default:
      throw std::logic_error("invalid map type");
  }
}

any_map& any_map::operator=(const any_map& m)
{
  if (this == &m)
    return *this;

  switch (type) {
    case map_type::ORDERED_MAP:
      delete map.o;
      break;
    case map_type::UNORDERED_MAP:
      delete map.uo;
      break;
    case map_type::UNORDERED_MAP_CASEINSENSITIVE_KEYS:
      delete map.uoci;
      break;
  }

  switch (m.type) {
    case map_type::ORDERED_MAP:
      map.o = new ordered_any_map(m.o_m());
      break;
    case map_type::UNORDERED_MAP:
      map.uo = new unordered_any_map(m.uo_m());
      break;
    case map_type::UNORDERED_MAP_CASEINSENSITIVE_KEYS:
      map.uoci = new unordered_any_cimap(m.uoci_m());
      break;
  }

  return *this;
}

any_map::~any_map()
{
  switch (type) {
    case map_type::ORDERED_MAP:
      delete map.o;
      break;
    case map_type::UNORDERED_MAP:
      delete map.uo;
      break;
    case map_type::UNORDERED_MAP_CASEINSENSITIVE_KEYS:
      delete map.uoci;
      break;
  }
}

any_map::iterator any_map::begin()
{
  switch (type) {
    case map_type::ORDERED_MAP:
      return { o_m().begin() };
    case map_type::UNORDERED_MAP:
      return { uo_m().begin(), iter::UNORDERED };
    case map_type::UNORDERED_MAP_CASEINSENSITIVE_KEYS:
      return { uoci_m().begin(), iter::UNORDERED_CI };
    default:
      throw std::logic_error("invalid map type");
  }
}

any_map::const_iter any_map::begin() const
{
  switch (type) {
    case map_type::ORDERED_MAP:
      return { o_m().begin() };
    case map_type::UNORDERED_MAP:
      return { uo_m().begin(), const_iterator::UNORDERED };
    case map_type::UNORDERED_MAP_CASEINSENSITIVE_KEYS:
      return { uoci_m().begin(), const_iterator::UNORDERED_CI };
    default:
      throw std::logic_error("invalid map type");
  }
}

any_map::const_iterator any_map::cbegin() const
{
  return begin();
}

any_map::iterator any_map::end()
{
  switch (type) {
    case map_type::ORDERED_MAP:
      return { o_m().end() };
    case map_type::UNORDERED_MAP:
      return { uo_m().end(), iterator::UNORDERED };
    case map_type::UNORDERED_MAP_CASEINSENSITIVE_KEYS:
      return { uoci_m().end(), iterator::UNORDERED_CI };
    default:
      throw std::logic_error("invalid map type");
  }
}

any_map::const_iterator any_map::end() const
{
  switch (type) {
    case map_type::ORDERED_MAP:
      return { o_m().end() };
    case map_type::UNORDERED_MAP:
      return { uo_m().end(), const_iterator::UNORDERED };
    case map_type::UNORDERED_MAP_CASEINSENSITIVE_KEYS:
      return { uoci_m().end(), const_iterator::UNORDERED_CI };
    default:
      throw std::logic_error("invalid map type");
  }
}

any_map::const_iterator any_map::cend() const
{
  return end();
}

bool any_map::empty() const
{
  switch (type) {
    case map_type::ORDERED_MAP:
      return o_m().empty();
    case map_type::UNORDERED_MAP:
      return uo_m().empty();
    case map_type::UNORDERED_MAP_CASEINSENSITIVE_KEYS:
      return uoci_m().empty();
    default:
      throw std::logic_error("invalid map type");
  }
}

any_map::size_type any_map::size() const
{
  switch (type) {
    case map_type::ORDERED_MAP:
      return o_m().size();
    case map_type::UNORDERED_MAP:
      return uo_m().size();
    case map_type::UNORDERED_MAP_CASEINSENSITIVE_KEYS:
      return uoci_m().size();
    default:
      throw std::logic_error("invalid map type");
  }
}

any_map::size_type any_map::count(const any_map::key_type& key) const
{
  switch (type) {
    case map_type::ORDERED_MAP:
      return o_m().count(key);
    case map_type::UNORDERED_MAP:
      return uo_m().count(key);
    case map_type::UNORDERED_MAP_CASEINSENSITIVE_KEYS:
      return uoci_m().count(key);
    default:
      throw std::logic_error("invalid map type");
  }
}

void any_map::clear()
{
  switch (type) {
    case map_type::ORDERED_MAP:
      return o_m().clear();
    case map_type::UNORDERED_MAP:
      return uo_m().clear();
    case map_type::UNORDERED_MAP_CASEINSENSITIVE_KEYS:
      return uoci_m().clear();
    default:
      throw std::logic_error("invalid map type");
  }
}

any_map::mapped_type& any_map::at(const key_type& key)
{
  switch (type) {
    case map_type::ORDERED_MAP:
      return o_m().at(key);
    case map_type::UNORDERED_MAP:
      return uo_m().at(key);
    case map_type::UNORDERED_MAP_CASEINSENSITIVE_KEYS:
      return uoci_m().at(key);
    default:
      throw std::logic_error("invalid map type");
  }
}

const any_map::mapped_type& any_map::at(const any_map::key_type& key) const
{
  switch (type) {
    case map_type::ORDERED_MAP:
      return o_m().at(key);
    case map_type::UNORDERED_MAP:
      return uo_m().at(key);
    case map_type::UNORDERED_MAP_CASEINSENSITIVE_KEYS:
      return uoci_m().at(key);
    default:
      throw std::logic_error("invalid map type");
  }
}

any_map::mapped_type& any_map::operator[](const any_map::key_type& key)
{
  switch (type) {
    case map_type::ORDERED_MAP:
      return o_m()[key];
    case map_type::UNORDERED_MAP:
      return uo_m()[key];
    case map_type::UNORDERED_MAP_CASEINSENSITIVE_KEYS:
      return uoci_m()[key];
    default:
      throw std::logic_error("invalid map type");
  }
}

any_map::mapped_type& any_map::operator[](any_map::key_type&& key)
{
  switch (type) {
    case map_type::ORDERED_MAP:
      return o_m()[std::move(key)];
    case map_type::UNORDERED_MAP:
      return uo_m()[std::move(key)];
    case map_type::UNORDERED_MAP_CASEINSENSITIVE_KEYS:
      return uoci_m()[std::move(key)];
    default:
      throw std::logic_error("invalid map type");
  }
}

std::pair<any_map::iterator, bool> any_map::insert(const value_type& value)
{
  switch (type) {
    case map_type::ORDERED_MAP: {
      auto p = o_m().insert(value);
      return { iterator(std::move(p.first)), p.second };
    }
    case map_type::UNORDERED_MAP: {
      auto p = uo_m().insert(value);
      return { iterator(std::move(p.first), iterator::UNORDERED), p.second };
    }
    case map_type::UNORDERED_MAP_CASEINSENSITIVE_KEYS: {
      auto p = uoci_m().insert(value);
      return { iterator(std::move(p.first), iterator::UNORDERED_CI), p.second };
    }
    default:
      throw std::logic_error("invalid map type");
  }
}

any_map::const_iterator any_map::find(const key_type& key) const
{
  switch (type) {
    case map_type::ORDERED_MAP:
      return { o_m().find(key) };
    case map_type::UNORDERED_MAP:
      return { uo_m().find(key), const_iterator::UNORDERED };
    case map_type::UNORDERED_MAP_CASEINSENSITIVE_KEYS:
      return { uoci_m().find(key), const_iterator::UNORDERED_CI };
    default:
      throw std::logic_error("invalid map type");
  }
}

any_map::ordered_any_map const& any_map::o_m() const
{
  return *map.o;
}

any_map::ordered_any_map& any_map::o_m()
{
  return *map.o;
}

any_map::unordered_any_map const& any_map::uo_m() const
{
  return *map.uo;
}

any_map::unordered_any_map& any_map::uo_m()
{
  return *map.uo;
}

any_map::unordered_any_cimap const& any_map::uoci_m() const
{
  return *map.uoci;
}

any_map::unordered_any_cimap& any_map::uoci_m()
{
  return *map.uoci;
}

// ----------------------------------------------------------
// ------------------------  AnyMap  ------------------------

AnyMap::AnyMap(map_type type)
  : any_map(type)
{}

AnyMap::AnyMap(const ordered_any_map& m)
  : any_map(m)
{}

AnyMap::AnyMap(const unordered_any_map& m)
  : any_map(m)
{}

AnyMap::AnyMap(const unordered_any_cimap& m)
  : any_map(m)
{}

AnyMap::map_type AnyMap::GetType() const
{
  return type;
}

AnyMap::mapped_type& AnyMap::AtCompoundKey(const key_type& key)
{
  return const_cast<mapped_type&>(
    static_cast<const AnyMap*>(this)->AtCompoundKey(key));
}

const AnyMap::mapped_type& AnyMap::AtCompoundKey(const key_type& key) const
{
  return detail::AtCompoundKey(*this, key);
}

template<>
std::ostream& any_value_to_string(std::ostream& os, const AnyMap& m)
{
  os << "{";
  typedef any_map::const_iterator Iterator;
  Iterator i1 = m.begin();
  const Iterator begin = i1;
  const Iterator end = m.end();
  for (; i1 != end; ++i1) {
    if (i1 == begin)
      os << i1->first << " : " << i1->second.ToString();
    else
      os << ", " << i1->first << " : " << i1->second.ToString();
  }
  os << "}";
  return os;
}

template<>
std::ostream& any_value_to_json(std::ostream& os, const AnyMap& m)
{
  os << "{";
  typedef any_map::const_iterator Iterator;
  Iterator i1 = m.begin();
  const Iterator begin = i1;
  const Iterator end = m.end();
  for (; i1 != end; ++i1) {
    if (i1 == begin)
      os << "\"" << i1->first << "\" : " << i1->second.ToJSON();
    else
      os << ", "
         << "\"" << i1->first << "\" : " << i1->second.ToJSON();
  }
  os << "}";
  return os;
}
}
