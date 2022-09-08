# How to Build Intel® Integrated Performance Primitives Cryptography (Intel® IPP Cryptography) <!-- omit in toc -->

- [Software Requirements](#software-requirements)
    - [Common tools](#common-tools)
    - [Linux* OS](#linux-os)
    - [Windows* OS](#windows-os)
    - [macOS*](#macos)
- [Building Intel IPP Cryptography on Linux\* OS](#building-intel-ipp-cryptography-on-linux-os)
- [Building Intel IPP Cryptography on Windows\* OS](#building-intel-ipp-cryptography-on-windows-os)
- [Building Intel IPP Cryptography on macOS\*](#building-intel-ipp-cryptography-on-macos)
- [CMake Build Options](#cmake-build-options)
    - [Common for all operating systems](#common-for-all-operating-systems)
    - [Windows\* OS](#windows-os)
    - [Linux\* OS](#linux-os)
- [CMake Commands FAQ](#cmake-commands-faq)
    - [How to build a 32-bit library?](#how-to-build-a-32-bit-library)
    - [How to build a 64-bit generic library without any CPU-specific optimizations?](#how-to-build-a-64-bit-generic-library-without-any-cpu-specific-optimizations)
    - [How to build two libraries with optimizations for Intel® Advanced Vector Extensions 2 and Intel® Advanced Vector Extensions 512 instruction sets?](#how-to-build-two-libraries-with-optimizations-for-intel-advanced-vector-extensions-2-and-intel-advanced-vector-extensions-512-instruction-sets)
    - [How to build a library to work in a kernel space?](#how-to-build-a-library-to-work-in-a-kernel-space)
- [Incorporating Intel® IPP Cryptography sources into custom build system](#incorporating-intel-ipp-cryptography-sources-into-custom-build-system)


## Software Requirements
### Common tools
- [CMake\*](https://cmake.org/download) 3.15 or higher
- Python 3.8.1
- The Netwide Assembler (NASM) 2.15
- OpenSSL\* 1.1.0 or higher

### Linux* OS
- [Common tools](#common-tools)
- Intel® C++ Compiler 19.1 for Linux\* OS
- GCC 8.3
- GCC 9.1
- GNU binutils 2.32
### Windows* OS
- [Common tools](#common-tools)
- Intel® C++ Compiler 19.1 for Windows\* OS
- Microsoft Visual C++ Compiler\* version 19.16 provided by Microsoft Visual Studio\* 2017 version 15.9
> **NOTE:** Support for this compiler version will be removed from Intel IPP Cryptography starting 2021.4 release. If you use it for building Intel IPP Cryptography library, please plan on migrating to a newer supported version of Microsoft Visual C++ Compiler\*.
- Microsoft Visual C++ Compiler\* version 19.24 provided by Microsoft Visual Studio\* 2019 version 16.4
### macOS*
- [Common tools](#common-tools)
- Intel® C++ Compiler 19.1 for macOS\* 
## Building Intel IPP Cryptography on Linux\* OS

The software was validated on:

- Red Hat\* Enterprise Linux\* 7

To build the Intel IPP Cryptography library on Linux\* OS, complete the following steps: 
1. Clone the source code from GitHub\* as follows:

    ``` bash
    git clone --recursive https://github.com/intel/ipp-crypto
    ```

2. Set the environment for one of the supported C/C++ compilers.

   *Example for Intel® Compiler:*

    ```bash
    source /opt/intel/bin/compilervars.sh intel64
    ```

    For details, refer to the [Intel® C++ Compiler Developer Guide and Reference](https://software.intel.com/en-us/cpp-compiler-developer-guide-and-reference-specifying-the-location-of-compiler-components-with-compilervars).

3. Run CMake\* in the command line.

    *Examples*:

    For Intel® C++ Compiler:

    ``` bash
    CC=icc CXX=icpc cmake CMakeLists.txt -B_build -DARCH=intel64
    ```

    For GCC:

    ``` bash
    CC=gcc CXX=g++ cmake CMakeLists.txt -B_build -DARCH=intel64
    ```

    For the complete list of supported CMake options, refer to the [CMake Build Options](#cmake-build-options) section.

4. Navigate to the build folder specified in the CMake command line and start the build:

    ```bash
    cd _build
    make all
    ```

    You can find the built libraries in the `<build_dir>/.build/<RELEASE|DEBUG>/lib` directory.

## Building Intel IPP Cryptography on Windows\* OS

The software was validated on:

- Windows Server\* 2016

To build the Intel IPP Cryptography library on Windows* OS, complete the following steps:

1. Clone the source code from GitHub\* as follows:

    ``` bash
    git clone --recursive https://github.com/intel/ipp-crypto
    ```

2. Set the environment variables for one of the supported C/C++ compilers.
    For Intel® Compiler instructions, refer to the [Intel® C++ Compiler Developer Guide and Reference](https://software.intel.com/en-us/cpp-compiler-developer-guide-and-reference).
    For MSVC* Compiler, refer to [Use the MSVC toolset from the command line](https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=vs-2017).

3. Run CMake\* in the command line.

    *Examples*:

    For Intel® C++ Compiler and Visual Studio\* 2019:

    ``` bash
    cmake CMakeLists.txt -B_build -G"Visual Studio 16 2019" -T"Intel C++ Compiler 19.1" -Ax64
    ```

    For MSVC\* Compiler and Visual Studio\* 2019:

    ``` bash
    cmake CMakeLists.txt -B_build -G"Visual Studio 16 2019" -Ax64
    ```

    For the complete list of supported CMake options, please refer to the [CMake Build Options](#cmake-build-options) section.

4. Navigate to the build folder, specified in the CMake command line and start build either from Visual Studio\* or in the command line.

    *Build from command line:*

    ```bash
    cmake --build . --parallel 4 --target ALL_BUILD --config Release
    ```

    *Build from Visual Studio\*:*
    Open the Microsoft Visual Studio\* solution `Intel(R) IPP Crypto.sln`, choose project (build target) from the Solution Explorer and run the build.

## Building Intel IPP Cryptography on macOS\*

The software was validated on:

- macOS\* 10.14

To build the Intel IPP Cryptography library on macOS*, complete the following steps:

1. Clone the source code from GitHub\* as follows:

    ``` bash
    git clone --recursive https://github.com/intel/ipp-crypto
    ```

2. Set the environment variables for one of the supported C/C++ compilers.

   *Example for Intel® Compiler:*

    ```bash
    source /opt/intel/bin/compilervars.sh intel64
    ```

    For details, refer to the [Intel® C++ Compiler Developer Guide and Reference](https://software.intel.com/en-us/cpp-compiler-developer-guide-and-reference-specifying-the-location-of-compiler-components-with-compilervars)

3. Run CMake\* in the command line.

    *Examples*:

    For Intel® C++ Compiler:

    ``` bash
    CC=icc CXX=icpc cmake CMakeLists.txt -B_build -DARCH=intel64
    ```

    For the complete list of supported CMake options, refer to the [CMake Build Options](#cmake-build-options) section.

4. Navigate to the build folder specified in the CMake command line and start the build:

    ```bash
    cd _build
    make all
    ```

    You can find the built libraries in the `<build_dir>/.build/<RELEASE|DEBUG>/lib` directory.

## CMake Build Options

### Common for all operating systems

- `-B<build-dir>` - defines the build directory. This is the directory where CMake puts the generated Microsoft Visual Studio\* solution or makefiles.

- `-DARCH=<ia32|intel64>` - on Linux* OS and macOS*, defines the target architecture for the build of the Intel IPP Cryptography library.
    > **NOTE:** On Windows* OS, use `-G`/`-A`  instead. See the description of these options [below](#windows-os-1).

- `-DMERGED_BLD:BOOL=<on|off>` - optional. Defines the configuration of the Intel IPP Cryptography library to build:

  - `-DMERGED_BLD:BOOL=on`: default configuration. It includes the following steps:
    - Build of a dispatched static library with all available optimizations
    - Build of a dispatched dynamic library with all available optimizations
    - Generation of the single-CPU headers (for details, refer to [this](./OVERVIEW.md) section)

  - `-DMERGED_BLD:BOOL=off`: build of one static library per optimization; build of one dynamic library per optimization.

- `-DPLATFORM_LIST="<platform list>"` - optional, works only if `-DMERGED_BLD:BOOL=off` is set. Sets target platforms for the code to be compiled. See the supported platforms list [here](./OVERVIEW.md).

    - Example for Linux\* OS and the IA-32 architecture:
        `-DPLATFORM_LIST="m7;s8;p8;g9;h9"`

    - Example for Linux\* OS and the Intel® 64 architecture:
        `-DPLATFORM_LIST="w7;n8;y8;e9;l9;n0;k0"`

### Windows\* OS

- `-G"<tool-chain-generator>"` - defines the native build system CMake will generate from the input files.
  Refer to CMake [documentation](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html#visual-studio-generators) for the Visual Studio\* generators options.

- `-A<x64|Win32>` - for Visual Studio\* 2019+, defines the target architecture for the build of the Intel IPP Cryptography library.

- `-T<Compiler>` - defines the compiler for building.
  For example, to use Intel® Compiler, specify `-T"Intel C++ Compiler 19.1"`.

> **NOTE:** Refer to CMake [documentation](https://cmake.org/cmake/help/latest/manual/ccmake.1.html) for more information on these options.

### Linux\* OS

- `-DNONPIC_LIB:BOOL=<off|on>` - optional. Defines whether the built library is position-dependent or not.

  - `-DNONPIC_LIB:BOOL=off:` default. Position-independent code.

  - `-DNONPIC_LIB:BOOL=on:` position-dependent code.

## CMake Commands FAQ

### How to build a 32-bit library?

`cmake CMakeLists.txt -B_build -DARCH=ia32`

### How to build a 64-bit generic library without any CPU-specific optimizations?

`cmake CMakeLists.txt -B_build -DARCH=intel64 -DMERGED_BLD:BOOL=off -DPLATFORM_LIST=mx`

### How to build two libraries with optimizations for Intel® Advanced Vector Extensions 2 and Intel® Advanced Vector Extensions 512 instruction sets?

`cmake CMakeLists.txt -B_build -DARCH=intel64 -DMERGED_BLD:BOOL=off -DPLATFORM_LIST="l9;k0"`

### How to build a library to work in a kernel space?

`cmake CMakeLists.txt -B_build -DARCH=intel64 -DNONPIC_LIB:BOOL=on`

### How to specify path to OpenSSL\*
`cmake CMakeLists.txt -B_build -DARCH=intel64 -DOPENSSL_INCLUDE_DIR=/path/to/openssl/include -DOPENSSL_LIBRARIES=/path/to/openssl/lib -DOPENSSL_ROOT_DIR=/path/to/openssl`

## Incorporating Intel® IPP Cryptography sources into custom build system

You can include Intel IPP Cryptography sources into some arbitrary project's CMake build system and build them with it.

Here is the minimal working example:

``` bash
cmake_minimum_required(VERSION 3.14)

project("test_proj")

# `crypto` is the repository root folder of Intel IPP Cryptography
add_subdirectory(crypto)
include_directories(crypto/include)

# 'main.cpp' is some arbitrary project's source file
add_executable("test_proj" main.cpp)
# `ippcp_s` is the target name of static library in the Intel IPP Cryptography build system.
# This static library will be built automatically, when you build your project.
target_link_libraries("test_proj" "ippcp_s")
```

Also you can use CMake module to find the Intel IPP Cryptography library installed on the system. The module location is `examples/FindIPPCrypto.cmake` and here is the example of its usage:

``` bash
find_package(IPPCrypto REQUIRED MODULE)

if (NOT IPPCRYPTO_FOUND)
   message(FATAL_ERROR "No Intel IPP Cryptography library found on the system.")
endif()

# If Intel IPP Cryptography is found, the following variables will be defined:
#     `IPPCRYPTO_LIBRARIES` - static library name
#     `IPPCRYPTO_INCLUDE_DIRS` - path to Intel IPP Cryptography headers
#     `IPPCRYPTO_ROOT_DIR` - library root dir (a folder with 'include' and 'lib' directories)
```
