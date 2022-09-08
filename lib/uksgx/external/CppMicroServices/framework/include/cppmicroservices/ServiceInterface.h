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

#ifndef CPPMICROSERVICES_SERVICEINTERFACE_H
#define CPPMICROSERVICES_SERVICEINTERFACE_H

#include "cppmicroservices/GlobalConfig.h"
#include "cppmicroservices/ServiceException.h"

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <typeinfo>

/**
\defgroup gr_serviceinterface Service Interface

\brief Groups Service Interface related symbols.
*/

/**
 * \ingroup MicroServices
 * \ingroup gr_serviceinterface
 *
 * Returns a unique id for a given type. By default, the
 * demangled name of \c T is returned.
 *
 * This template method may be specialized directly or by
 * using the macro #CPPMICROSERVICES_DECLARE_SERVICE_INTERFACE to return
 * a custom id for each service interface.
 *
 * @tparam T The service interface type.
 * @return A unique id for the service interface type T.
 */
template<class T>
const std::string& us_service_interface_iid();

namespace cppmicroservices {

class ServiceFactory;

/**
 * @ingroup MicroServices
 * @ingroup gr_serviceinterface
 *
 * A map containing interfaces ids and their corresponding service object
 * smart pointers. InterfaceMap instances represent a complete service object
 * which implements one or more service interfaces. For each implemented
 * service interface, there is an entry in the map with the key being
 * the service interface id and the value a smart pointer to the service
 * interface implementation.
 *
 * To create InterfaceMap instances, use the MakeInterfaceMap helper class.
 *
 * @note This is a low-level type and should only rarely be used.
 *
 * @see MakeInterfaceMap
 */
typedef std::map<std::string, std::shared_ptr<void>> InterfaceMap;
typedef std::shared_ptr<InterfaceMap> InterfaceMapPtr;
typedef std::shared_ptr<const InterfaceMap> InterfaceMapConstPtr;

/// \cond
namespace detail {
US_Framework_EXPORT std::string GetDemangledName(
  const std::type_info& typeInfo);

template<class Interfaces, size_t size>
struct InsertInterfaceHelper
{
  static void insert(InterfaceMapPtr& im, const Interfaces& interfaces)
  {
    std::pair<std::string, std::shared_ptr<void>> aPair = std::make_pair(
      std::string(
        us_service_interface_iid<
          typename std::tuple_element<size - 1,
                                      Interfaces>::type::element_type>()),
      std::static_pointer_cast<void>(std::get<size - 1>(interfaces)));
    im->insert(aPair);
    InsertInterfaceHelper<Interfaces, size - 1>::insert(im, interfaces);
  }
};

template<class T>
struct InsertInterfaceHelper<T, 0>
{
  static void insert(InterfaceMapPtr&, const T&) {}
};

template<class Interfaces>
void InsertInterfaceTypes(InterfaceMapPtr& im, const Interfaces& interfaces)
{
  InsertInterfaceHelper<Interfaces, std::tuple_size<Interfaces>::value>::insert(
    im, interfaces);
}

template<template<class...> class List, class... Args>
struct InterfacesTuple
{
  typedef List<std::shared_ptr<Args>...> type;

  template<class Impl>
  static type create(const std::shared_ptr<Impl>& impl)
  {
    return type(std::static_pointer_cast<Args>(impl)...);
  }
};

template<class T, class... List>
struct Contains : std::true_type
{};

template<class T, class Head, class... Rest>
struct Contains<T, Head, Rest...>
  : std::conditional<std::is_same<T, Head>::value,
                     std::true_type,
                     Contains<T, Rest...>>::type
{};

template<class T>
struct Contains<T> : std::false_type
{};
}
/// \endcond
}

/// \cond
template<class T>
const std::string& us_service_interface_iid()
{
  static const std::string name =
    cppmicroservices::detail::GetDemangledName(typeid(T));
  return name;
}

template<>
inline const std::string& us_service_interface_iid<void>()
{
  static const std::string name("");
  return name;
}
/// \endcond

/**
 * \ingroup MicroServices
 * \ingroup gr_macros
 *
 * \brief Declare a service interface id.
 *
 * This macro associates the given identifier \e _service_interface_id (a string literal) to the
 * interface class called _service_interface_type. The Identifier must be unique. For example:
 *
 * \code
 * #include "cppmicroservices/ServiceInterface.h"
 *
 * struct ISomeInterace { ... };
 *
 * CPPMICROSERVICES_DECLARE_SERVICE_INTERFACE(ISomeInterface, "com.mycompany.service.ISomeInterface/1.0")
 * \endcode
 *
 * The usage of this macro is optional and the service interface id which is automatically
 * associated with any type is usually good enough (the demangled type name). However, care must
 * be taken if the default id is compared with a string literal hard-coding a service interface
 * id. E.g. the default id for templated types in the STL may differ between platforms. For
 * user-defined types and templates the ids are typically consistent, but platform specific
 * default template arguments will lead to different ids.
 *
 * This macro is normally used right after the class definition for _service_interface_type,
 * in a header file.
 *
 * If you want to use #CPPMICROSERVICES_DECLARE_SERVICE_INTERFACE with interface classes declared in a
 * namespace then you have to make sure the #CPPMICROSERVICES_DECLARE_SERVICE_INTERFACE macro call is not
 * inside a namespace though. For example:
 *
 * \code
 * #include "cppmicroservices/ServiceInterface.h"
 *
 * namespace Foo
 * {
 *   struct ISomeInterface { ... };
 * }
 *
 * CPPMICROSERVICES_DECLARE_SERVICE_INTERFACE(Foo::ISomeInterface, "com.mycompany.service.ISomeInterface/1.0")
 * \endcode
 *
 * @param _service_interface_type The service interface type.
 * @param _service_interface_id A string literal representing a globally unique identifier.
 */
#define CPPMICROSERVICES_DECLARE_SERVICE_INTERFACE(_service_interface_type,    \
                                                   _service_interface_id)      \
  template<>                                                                   \
  inline const std::string&                                                    \
  us_service_interface_iid<_service_interface_type>()                          \
  {                                                                            \
    static const std::string name(_service_interface_id);                      \
    return name;                                                               \
  }

namespace cppmicroservices {

/**
 * @ingroup MicroServices
 * @ingroup gr_serviceinterface
 *
 * Helper class for constructing InterfaceMap instances based
 * on service implementations or service factories.
 *
 * Example usage:
 * \code
 * std::shared_ptr<MyService> service; // implementes I1 and I2
 * InterfaceMap im = MakeInterfaceMap<I1,I2>(service);
 * \endcode
 *
 * @see InterfaceMap
 */
template<class... Interfaces>
class MakeInterfaceMap
{

public:
  /**
   * Constructor taking a service implementation pointer.
   *
   * @param impl A service implementation pointer, which must
   *        be castable to a all specified service interfaces.
   */
  template<class Impl>
  MakeInterfaceMap(const std::shared_ptr<Impl>& impl)
    : m_interfaces(
        detail::InterfacesTuple<std::tuple, Interfaces...>::create(impl))
  {}

  /**
   * Constructor taking a service factory.
   *
   * @param factory A service factory.
   */
  MakeInterfaceMap(const std::shared_ptr<ServiceFactory>& factory)
    : m_factory(factory)
  {
    if (!m_factory) {
      throw ServiceException(
        "The service factory argument must not be nullptr.");
    }
  }

  operator InterfaceMapPtr() { return getInterfaceMap(); }

  // overload for the const version of the map
  operator InterfaceMapConstPtr()
  {
    return InterfaceMapConstPtr(getInterfaceMap());
  }

private:
  InterfaceMapPtr getInterfaceMap()
  {
    InterfaceMapPtr sim = std::make_shared<InterfaceMap>();
    detail::InsertInterfaceTypes(sim, m_interfaces);

    if (m_factory) {
      sim->insert(
        std::make_pair(std::string("org.cppmicroservices.factory"), m_factory));
    }

    return sim;
  }

  std::shared_ptr<ServiceFactory> m_factory;

  typename detail::InterfacesTuple<std::tuple, Interfaces...>::type
    m_interfaces;
};

/**
 * @ingroup MicroServices
 * @ingroup gr_serviceinterface
 *
 * Extract a service interface pointer from a given InterfaceMap instance.
 *
 * @param map a InterfaceMap instance.
 * @return A shared pointer object of type \c Interface. The returned object is
 *         empty if the map does not contain an entry for the given type
 *
 * @see MakeInterfaceMap
 */
template<class Interface>
std::shared_ptr<Interface> ExtractInterface(const InterfaceMapConstPtr& map)
{
  InterfaceMap::const_iterator iter =
    map->find(us_service_interface_iid<Interface>());
  if (iter != map->end()) {
    return std::static_pointer_cast<Interface>(iter->second);
  }
  return nullptr;
}

/**
 * @ingroup MicroServices
 * @ingroup gr_serviceinterface
 *
 * Extract a service interface pointer from a given InterfaceMap instance.
 *
 * @param map a InterfaceMap instance.
 * @param interfaceId The interface id string.
 * @return The service interface pointer for the service interface id or
 *         \c nullptr if \c map does not contain an entry for the given type.
 *
 * @see ExtractInterface(const InterfaceMapConstPtr&)
 */
inline std::shared_ptr<void> ExtractInterface(const InterfaceMapConstPtr& map,
                                              const std::string& interfaceId)
{
  if (!map) {
    return nullptr;
  }

  if (interfaceId.empty() && map && !map->empty()) {
    return map->begin()->second;
  }

  auto iter = map->find(interfaceId);
  if (iter != map->end()) {
    return iter->second;
  }
  return nullptr;
}

///@{
/**
 * @ingroup MicroServices
 * @ingroup gr_serviceinterface
 *
 * Cast the argument to a shared pointer of type \c ServiceFactory. Useful when
 * calling \c BundleContext::RegisterService with a service factory, for example:
 *
 * \code
 * std::shared_ptr<MyServiceFactory> factory = std::make_shared<MyServiceFactory>();
 * context->RegisterService<ISomeInterface>(ToFactory(factory));
 * \endcode
 *
 * @param factory The service factory shared_ptr object
 * @return A \c shared_ptr object of type \c ServiceFactory
 *
 * @see BundleContext::RegisterService(ServiceFactory* factory, const ServiceProperties& properties)
 */
template<class T>
std::shared_ptr<ServiceFactory> ToFactory(const std::shared_ptr<T>& factory)
{
  return std::static_pointer_cast<ServiceFactory>(factory);
}

///@}
}

#endif // CPPMICROSERVICES_SERVICEINTERFACE_H
