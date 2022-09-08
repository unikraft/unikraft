.. _concept-bundle-properties:

Bundle Properties
=================

A C++ Micro Services Bundle provides meta-data in the form of so-called
*properties* about itself. Properties are key - value pairs where the
key is of type ``std::string`` and the value of type ``Any``. The
following properties are always set by the C++ Micro Services library
and cannot be altered by the bundle author:

-  ``bundle.id`` - The unique id of the bundle (type ``long``)
-  ``bundle.location`` - The full path to the bundle's shared library on
   the file system (type ``std::string``)

Bundle authors must always add the following property to their bundle's
``manifest.json`` file:

-  ``bundle.symbolic_name`` - The human readable name of the bundle (type
   ``std::string``)

C++ Micro Services will not install any bundle which doesn't contain a
valid 'bundle.symbolic_name' property in its ``manifest.json`` file.

Bundle authors can add custom properties by providing a
``manifest.json`` file, embedded as a top-level resource into the bundle
(see :any:`concept-resources`). The root value of the
JSON file must be a JSON object. An example ``manifest.json`` file would
be:

.. code:: json

    {
      "bundle.symbolic_name" : "my bundle",
      "bundle.version" : "1.0.2",
      "bundle.description" : "This bundle provides an awesome service",
      "authors" : [ "John Doe", "Douglas Reynolds", "Daniel Cannady" ],
      "rating" : 5
    }

    
All JSON member names of the root object will be available as property
keys in the bundle containing the ``manifest.json`` file. The C++ Micro
Services library specifies the following standard keys for re-use in
``manifest.json`` files:

-  ``bundle.version`` - The version of the bundle (type
   ``std::string``). The version string must be a valid version
   identifier, as specified in the BundleVersion class.
-  ``bundle.vendor`` - The vendor name of the bundle (type
   ``std::string``)
-  ``bundle.description`` - A description for the bundle (type
   ``std::string``)

.. note::

   Some of the properties mentioned above may also be
   accessed via dedicated methods in the Bundle class, e.g.
   :any:`GetSymbolicName() <cppmicroservices::Bundle::GetSymbolicName>` or
   :any:`GetVersion() <cppmicroservices::Bundle::GetVersion>`.

.. attention::

   Despite JSON being a case-sensitive format, C++ Micro Services stores bundle properties
   as case-insensitive to accommodate LDAP queries using :any:`LDAPFilter <cppmicroservices::LDAPFilter>`
   and :any:`LDAPProp <cppmicroservices::LDAPProp>`.
   Either keep the JSON case-insensitive or standardize on a convention to ensure queries
   return the correct results.


When parsing the ``manifest.json`` file, the JSON types are mapped to
C++ types and stored in instances of the :any:`Any class <cppmicroservices::Any>`.
The mapping is as follows:

+----------+-----------------+
| JSON     | C++ (Any)       |
+==========+=================+
| object   | std::map        |
+----------+-----------------+
| array    | std::vector     |
+----------+-----------------+
| string   | std::string     |
+----------+-----------------+
| number   | int or double   |
+----------+-----------------+
| true     | bool            |
+----------+-----------------+
| false    | bool            |
+----------+-----------------+
| null     | Any()           |
+----------+-----------------+
