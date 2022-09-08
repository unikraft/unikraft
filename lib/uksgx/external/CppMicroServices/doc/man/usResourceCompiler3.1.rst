.. _`usResourceCompiler3(1)`:

usResourceCompiler3
===================

See also :any:`cppmicroservices(7)` for a high-level description.

Command-Line Reference
----------------------

The following options are supported by the :program:`usResourceCompiler3` program:

.. program:: usResourceCompiler3

.. option:: --help, -h

   Print usage and exit.

.. option:: --verbose, -V

   Run in verbose mode.
 
.. option:: --bundle-name, -n
 
   The bundle name as specified in the ``US_BUNDLE_NAME`` compile definition.
 
.. option:: --compression-level, -c
 
   Compression level used for zip. Value range is 0 to 9.
   Default value is 6.
 
.. option:: --out-file, -o
 
   Path to output zip file. If the file exists it will be
   overwritten. If this option is not provided, a
   temporary zip fie will be created.
 
.. option:: --res-add, -r
 
   Path to a resource file, relative to the current
   working directory.
 
.. option:: --zip-add, -z
 
   Path to a file containing a zip archive to be merged
   into the output zip file. 
 
.. option:: --manifest-add, -m
 
   Path to the bundle's manifest file. If multiple --manifest-add options
   are specified, all manifest files will be concatenated into one.
 
.. option:: --bundle-file, -b
 
   Path to the bundle binary. The resources zip file will
   be appended to this binary. 

.. note::

   #. Only options :option:`--res-add`, :option:`--zip-add` and :option:`--manifest-add`
      can be specified multiple times.
   #. If option :option:`--manifest-add` or :option:`--res-add` is specified,
      option :option:`--bundle-name` must be provided.
   #. At-least one of :option:`--bundle-file` or :option:`--out-file` options
      must be provided.

.. hint::

   If you are using CMake, consider using the provided
   :any:`usFunctionEmbedResources` CMake macro which handles the invocation
   of the :program:`usResourceCompiler3` executable and sets up the correct file
   dependencies. Otherwise, you also need to make sure that the set of
   static bundles linked into a shared bundle or executable is also in the
   input file list of your :program:`usResourceCompiler3` call for that shared
   bundle or executable.

   Here is a full example creating a bundle and embedding resource data:

   .. literalinclude:: /framework/doc/snippets/uServices-resources-cmake/CMakeLists_example.txt
      :language: cmake

If you are not using CMake, you can run the resource compiler from the
command line yourself.

Example usage
-------------

Construct a zip blob with contents *mybundle/manifest.json*, merge the
contents of zip file *filetomerge.zip* into it and write the resulting blob into
*Example.zip*::

   usResourceCompiler3 --compression-level 9 --verbose --bundle-name mybundle
     --out-file Example.zip --manifest-add manifest.json --zip-add filetomerge.zip


Construct a zip blob with contents *mybundle/manifest.json*, merge the
contents of zip file *archivetomerge.zip* into it and append the resulting zip
blob to *mybundle.dylib*::

   usResourceCompiler3 -V -n mybundle -b mybundle.dylib -m manifest.json
     -z archivetomerge.zip

Append the contents of *archivetoembed.zip* to *mybundle.dll*::

   usResourceCompiler3.exe -b mybundle.dll -z archivetoembed.zip

Construct a zip blob with the contents of manifest_part1.json and auto_generated_manifest.json
concatenated into *mybundle/manifest.json* and embed it into *mybundle.dll*::

   usResourceCompiler3 -n mybundle -b mybundle.dll -m manifest_part1.json
     -m auto_generated_manifest.json

