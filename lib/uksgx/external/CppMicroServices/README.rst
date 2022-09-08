
.. rubric:: Continuous Integration Status

+-------------+-------------------------+--------------------------+------------------------+
| Branch      | GCC 5.4 and 8.0         | Visual Studio 2015       |                        |
|             +-------------------------+--------------------------+------------------------+
|             | Xcode 7.3 and 9.4       | Visual Studio 2017       |                        |
|             +-------------------------+--------------------------+------------------------+
|             | Clang 3.4 and 6.0       | MinGW-w64                |                        |
+=============+=========================+==========================+========================+
| master      | |Linux Build Status|    | |Windows Build status|   | |Code Coverage Status| |
+-------------+-------------------------+--------------------------+------------------------+
| development | |Linux Build Status     | |Windows Build status    | |Code Coverage Status  |
|             | (development)|          | (development)|           | (development)|         |
+-------------+-------------------------+--------------------------+------------------------+

|Coverity Scan Build Status|


C++ Micro Services
==================

|RTD Build Status (stable)| |RTD Build Status (development)|

`Download <https://github.com/CppMicroServices/CppMicroServices/releases>`_

Introduction
------------

The C++ Micro Services project is a collection of components for building
modular and dynamic service-oriented applications. It is based on
`OSGi <http://osgi.org>`_, but tailored to support native cross-platform solutions.

Proper usage of C++ Micro Services patterns and concepts leads to systems
with one or more of the following properties:

- Re-use of software components
- Loose coupling between service providers and consumers
- Separation of concerns, based on a service-oriented design
- Clean APIs based on service interfaces
- Extensible and reconfigurable systems


Requirements
------------

None, except a recent enough C++ compiler. All third-party library
dependencies are included and mostly used for implementation details.

Supported Platforms
-------------------

The library makes use of C++14 language and library features and compiles
on many different platforms.

Recommended minimum required compiler versions:

- GCC 5.4
- Clang 3.4
- Clang from Xcode 8.0
- Visual Studio 2015

You may use older compilers, but certain functionality may not be
available. Check the warnings printed during configuration of
your build tree. The following are the absolute minimum requirements:

- GCC 5.1
- Clang 3.4
- Clang from Xcode 7.3
- Visual Studio 2015 (MSVC++ 14.0)

Below is a list of tested compiler/OS combinations:

- GCC 5.4 (Ubuntu 14.04) via Travis CI
- GCC 8.0 (Ubuntu 14.04) via Travis CI
- Clang 3.4 (Ubuntu 14.04) via Travis CI
- Clang 6.0 (Ubuntu 14.04) via Travis CI
- Clang Xcode 7.3 (OS X 10.11) via Travis CI
- Clang Xcode 9.4 (OS X 10.13) via Travis CI
- Visual Studio 2015 via Appveyor
- Visual Studio 2017 via Appveyor
- MinGW-w64 via Appveyor

Legal
-----

The C++ Micro Services project was initially developed at the German
Cancer Research Center. Its source code is hosted as a `GitHub Project`_.
See the `COPYRIGHT file`_ in the top-level directory for detailed
copyright information.

This project is licensed under the `Apache License v2.0`_.

Code of Conduct
---------------

CppMicroServices.org welcomes developers with different backgrounds and
a broad range of experience. A diverse and inclusive community will
create more great ideas, provide more unique perspectives, and produce
more outstanding code. Our aim is to make the CppMicroServices community
welcoming to everyone.

To provide clarity of what is expected of our members, CppMicroServices
has adopted the code of conduct defined by
`contributor-covenant.org <http://contributor-covenant.org>`_. This
document is used across many open source communities, and we believe it
articulates our values well.

Please refer to the :any:`Code of Conduct <code-of-conduct>` for further
details.

Quick Start
-----------

Essentially, the C++ Micro Services library provides you with a powerful
dynamic service registry on top of a managed lifecycle. The framework manages,
among other things, logical units of modularity called *bundles* that
are contained in shared or static libraries. Each bundle
within a library has an associated :any:`cppmicroservices::BundleContext`
object, through which the service registry is accessed.

To query the registry for a service object implementing one or more
specific interfaces, the code would look like this:

.. code:: cpp

    #include "cppmicroservices/BundleContext.h"
    #include "SomeInterface.h"

    using namespace cppmicroservices;

    void UseService(BundleContext context)
    {
      auto serviceRef = context.GetServiceReference<SomeInterface>();
      if (serviceRef)
      {
        auto service = context.GetService(serviceRef);
        if (service) { /* do something */ }
      }
    }

Registering a service object against a certain interface looks like
this:

.. code:: cpp

    #include "cppmicroservices/BundleContext.h"
    #include "SomeInterface.h"

    using namespace cppmicroservices;

    void RegisterSomeService(BundleContext context, const std::shared_ptr<SomeInterface>& service)
    {
      context.RegisterService<SomeInterface>(service);
    }

The OSGi service model additionally allows to annotate services with
properties and using these properties during service look-ups. It also
allows to track the life-cycle of service objects. Please see the
`Documentation <http://docs.cppmicroservices.org>`_
for more examples and tutorials and the API reference. There is also a
blog post about `OSGi Lite for C++ <http://blog.cppmicroservices.org/2012/04/15/osgi-lite-for-c++>`_.

Git Branch Conventions
----------------------

The Git repository contains two eternal branches,
`master <https://github.com/CppMicroServices/CppMicroServices/tree/master/>`_
and
`development <https://github.com/CppMicroServices/CppMicroServices/tree/development/>`_.
The master branch contains production quality code and its HEAD points
to the latest released version. The development branch is the default
branch and contains the current state of development. Pull requests by
default target the development branch. See the :ref:`CONTRIBUTING <contributing>`
file for details about the contribution process.


.. _COPYRIGHT file: https://github.com/CppMicroServices/CppMicroServices/blob/development/COPYRIGHT
.. _GitHub Project: https://github.com/CppMicroServices/CppMicroServices
.. _Apache License v2.0: http://www.apache.org/licenses/LICENSE-2.0

.. |Linux Build Status| image:: https://img.shields.io/travis/CppMicroServices/CppMicroServices/master.svg?style=flat-square&label=Linux%20%26%20OS%20X
   :target: http://travis-ci.org/CppMicroServices/CppMicroServices
.. |Windows Build status| image:: https://img.shields.io/appveyor/ci/cppmicroservices/cppmicroservices/master.svg?style=flat-square&label=Windows
   :target: https://ci.appveyor.com/project/cppmicroservices/cppmicroservices/branch/master
.. |Linux Build Status (development)| image:: https://img.shields.io/travis/CppMicroServices/CppMicroServices/development.svg?style=flat-square&label=Linux%20%26%20OS%20X
   :target: https://travis-ci.org/CppMicroServices/CppMicroServices
.. |Windows Build status (development)| image:: https://img.shields.io/appveyor/ci/cppmicroservices/cppmicroservices/development.svg?style=flat-square&label=Windows
   :target: https://ci.appveyor.com/project/cppmicroservices/cppmicroservices/branch/development
.. |Coverity Scan Build Status| image:: https://img.shields.io/coverity/scan/1329.svg?style=flat-square
   :target: https://scan.coverity.com/projects/1329
.. |RTD Build Status (stable)| image:: https://readthedocs.org/projects/cppmicroservices/badge/?version=stable&style=flat-square
   :target: http://docs.cppmicroservices.org/en/stable/?badge=stable
   :alt: Documentation Status (stable)
.. |RTD Build Status (development)| image:: https://readthedocs.org/projects/cppmicroservices/badge/?version=latest&style=flat-square
   :target: http://docs.cppmicroservices.org/en/latest/?badge=development
   :alt: Documentation Status (development)
.. |Code Coverage Status| image:: https://img.shields.io/codecov/c/github/CppMicroServices/CppMicroServices/master.svg?style=flat-square
   :target: https://codecov.io/gh/cppmicroservices/CppMicroServices/branch/master
.. |Code Coverage Status (development)| image:: https://img.shields.io/codecov/c/github/CppMicroServices/CppMicroServices/development.svg?style=flat-square
   :target: https://codecov.io/gh/cppmicroservices/CppMicroServices/branch/development
