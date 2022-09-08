# Building usage examples of Intel® Integrated Performance Primitives Cryptography (Intel® IPP Cryptography) library

## System requirements

-   CMake 3.15

## Build with library sources

Only merged library (cmake option `-DMERGED_BLD:BOOL=ON`) builds are supported for the examples.

1. Navigate to the library sources root and run cmake to generate a library build procedure with the `-DBUILD_EXAMPLES:BOOL=ON`
   option.

   On Linux\*/macOS\*:

    `cmake CMakeLists.txt -B_build -DARCH=intel64 -DMERGED_BLD:BOOL=ON -DBUILD_EXAMPLES:BOOL=ON`

   On Windows\*:

   `cmake CMakeLists.txt -B_build -G<vs_generator> [-T"Intel® C++ Compiler <version>"] -DBUILD_EXAMPLES:BOOL=ON`

   For the Visual Studio\* generators options, please refer to the CMake help.
   The toolchain switch is optional, specify it if you want to build the library and examples using Intel® C++ Compiler.

   For the list of supported compiler versions or other cmake build options, please refer to the library root README.md file.

2. On Linux\*/macOS\*, build with `make -j8 <target>`. You can use the following targets:

   - To build an invididual example, use targets started with the *example_* string (like *example_aes-256-ctr-encryption*).

   - To build all examples of a single specific category, use target *ippcp_examples_\<category\>* (like *ippcp_examples_aes*).

   - To build all examples, use target *ippcp_examples_all*.

3. On Windows\* OS open generated Visual Studio\* solution in the IDE, select the appropriate project (individual example,
   all examples by category or the whole set of examples) in the *examples* folder of project structure in IDE and run **Build**.

   To run the build from the command line, open "Developer Command Prompt for VS \<version\>" console and run the cmake command:

   `cmake --build _build --target <target> --config Release`,

   where '_build' is the path to CMake build directory.

## Build with pre-built library

1. Navigate to the *examples* folder and run the cmake command below.

   On Linux\*/macOS\*:

   `cmake CMakeLists.txt -B_build`

   On Windows\* OS it is required to specify a generator (`-G` option) and optionally a toolchain (`-T` option)
   to build with Intel® C++ Compiler. Example:

   `cmake CMakeLists.txt -B_build -G<vs_generator> [-T"Intel C++ Compiler <version>"]`

   For the Visual Studio\* generators options, please refer to the CMake help.

2. The build system will scan the system for the Intel IPP Cryptography library.
   If it is found, you’ll see the following message:

   ```
   -- Found Intel IPP Cryptography at: /home/user/intel/ippcp
   -- Configuring done
   ```

   If the library is not found automatically, you can specify the path to the library root folder
   (where the include/ and lib/ directories are located) using the `-DIPPCRYPTO_ROOT_DIR=<path>` option.

3. Run the build process as described in the [Build with library sources](#build-with-library-sources).


# How to add a new example into Intel IPP Cryptography library:

1. Choose a category (a folder), where to put the example, and a filename. Use
   existing folders where applicable.
   The file name should be as follows: "\<category\>-\<key-size\>-\<mode\>-\<other-info\>.cpp"
   E.g.: "rsa-1k-oaep-sha1-type2-decryption.cpp" for the example of RSA category.

2. Write an example keeping its source code formatting consistent with other
   examples as much as possible.  The "aes/aes-256-ctr-encryption.cpp" can be used
   as a reference.

3. Use Doxygen annotations for the file description, global variables and
   macros. The *main()* function shall not use doxygen annotations inside
   (otherwise they disappear in the source code section of an example page in
   the generated documentation).

4. Add the example to the build: open *examples/CMakeLists.txt* file and add the
   new file to the *IPPCP_EXAMPLES* list.

5. Make sure it can be built using Intel IPP Cryptography examples build procedure, and it
   works correctly.

You are ready to submit a pull request!
