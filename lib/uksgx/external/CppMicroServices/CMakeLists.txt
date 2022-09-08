# Extract the current version from the VERSION file
file(STRINGS VERSION _version LIMIT_COUNT 1)

set(US_CMAKE_MINIMUM_REQUIRED_VERSION 3.2)

cmake_minimum_required(VERSION ${US_CMAKE_MINIMUM_REQUIRED_VERSION})

project(CppMicroServices VERSION ${_version})

set(US_GLOBAL_VERSION_SUFFIX ${${PROJECT_NAME}_VERSION_MAJOR})
if (${PROJECT_NAME}_VERSION_MINOR EQUAL 99)
  math(EXPR US_GLOBAL_VERSION_SUFFIX "${US_GLOBAL_VERSION_SUFFIX}+1")
endif()

string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

cmake_policy(VERSION ${US_CMAKE_MINIMUM_REQUIRED_VERSION})

list(APPEND policies_new
  CMP0025 # Compiler id for Apple Clang is now AppleClang (CMake 3.0)
  CMP0054 # Only interpret if() arguments as variables or keywords when unquoted
  )
foreach(policy ${policies_new})
  if(POLICY ${policy})
    cmake_policy(SET ${policy} NEW)
  endif()
endforeach()

#-----------------------------------------------------------------------------
# Check minimum required compiler versions
#------------------------------------------------------------------------------

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED 1)
set(CMAKE_CXX_STANDARD 11)

# Since version 3.1, CMake pulls in implicit link libraries when compiling
# source files in C mode. Because we mix C and C++ code in the test driver
# executable, we end up with a gcc_eh link dependency when using MinGW
# which breaks exception handling in the executable. See also:
#
# https://stackoverflow.com/questions/46688200/mixing-c-and-c-causing-exceptions-to-terminate
#
set(CMAKE_C_IMPLICIT_LINK_LIBRARIES "")

# We make use of the following C++11 language and library features:
#
# Language feature                 GCC   Clang   Xcode   VisualStudio
# -------------------------------------------------------------------
# Rvalue references                4.3   2.9     6       2012
# Variadic templates               4.3   2.9     6       2013*
# Initializer lists                4.4   3.1     6       2013
# Static assertions                4.3   2.9     6       2010
# auto                             4.4   2.9     6       2010
# Lambdas                          4.5   3.1     6       2012
# decltype                         4.3   2.9     6       2012
# Right angle brackets             4.3   2.9     6       2010
# Null pointer constant            4.6   3.0     6       2010
# Explicit conversion operators    4.5   3.0     6       2013
# Defaulted functions              4.4   3.0     6       2013*
# Deleted functions                4.4   2.9     6       2013*
# Range-based for                  4.6   3.0     6       2012
# Atomic operations                4.4   3.1     6       2012
# thread_local (optional)          4.8   3.3     8       2015
#
# Library features used
# ---------------------
# function
# unique_ptr
# shared_ptr
# unordered_map
# unordered_set
# atomic
# mutex
# wait_condition
# exception_ptr

set(US_COMPILER_GNU_MINIMUM_VERSION          4.8         )
set(US_COMPILER_Clang_MINIMUM_VERSION        3.4         )
set(US_COMPILER_AppleClang_MINIMUM_VERSION   7.3         )
set(US_COMPILER_MSVC_MINIMUM_VERSION         19.00.23026 )

set(US_COMPILER_GNU_MINIMUM_VERSION_PRODUCT        "GCC ${US_COMPILER_GNU_MINIMUM_VERSION}")
set(US_COMPILER_Clang_MINIMUM_VERSION_PRODUCT      "Clang ${US_COMPILER_CLANG_MINIMUM_VERSION}")
set(US_COMPILER_AppleClang_MINIMUM_VERSION_PRODUCT "Xcode 7.3 (Clang ${US_COMPILER_AppleClang_MINIMUM_VERSION})")
set(US_COMPILER_MSVC_MINIMUM_VERSION_PRODUCT       "Visual Studio 2015 (MSVC ${US_COMPILER_MSVC_MINIMUM_VERSION})")

set(US_COMPILER_GNU_MINIMUM_RECOMMENDED_VERSION         5.4   )
set(US_COMPILER_Clang_MINIMUM_RECOMMENDED_VERSION       3.4   )
set(US_COMPILER_AppleClang_MINIMUM_RECOMMENDED_VERSION  8.0   )
set(US_COMPILER_MSVC_MINIMUM_RECOMMENDED_VERSION        19.00 )

set(US_COMPILER_GNU_MINIMUM_RECOMMENDED_VERSION_PRODUCT        "GCC ${US_COMPILER_GNU_MINIMUM_RECOMMENDED_VERSION}")
set(US_COMPILER_Clang_MINIMUM_RECOMMENDED_VERSION_PRODUCT      "Clang ${US_COMPILER_Clang_MINIMUM_RECOMMENDED_VERSION}")
set(US_COMPILER_AppleClang_MINIMUM_RECOMMENDED_VERSION_PRODUCT "Xcode 8.0 (Clang ${US_COMPILER_AppleClang_MINIMUM_RECOMMENDED_VERSION})")
set(US_COMPILER_MSVC_MINIMUM_RECOMMENDED_VERSION_PRODUCT       "Visual Studio 2015 (MSVC ${US_COMPILER_MSVC_MINIMUM_RECOMMENDED_VERSION})")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(US_COMPILER_GNU 1)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(US_COMPILER_CLANG 1)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
  set(US_COMPILER_APPLE_CLANG 1)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(US_COMPILER_MSVC 1)
else()
  message(WARNING "You are using an unsupported compiler! Compilation has only been tested with Clang (Linux or Apple), GCC and MSVC.")
endif()

if (CMAKE_CXX_COMPILER_VERSION AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${US_COMPILER_${CMAKE_CXX_COMPILER_ID}_MINIMUM_VERSION})
  message(FATAL_ERROR "${US_COMPILER_${CMAKE_CXX_COMPILER_ID}_MINIMUM_VERSION_PRODUCT} or higher required (you are using ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}).")
endif()

#-----------------------------------------------------------------------------
# Update CMake module path
#------------------------------------------------------------------------------

set(US_CMAKE_DIR ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_MODULE_PATH
  ${US_CMAKE_DIR}
  ${CMAKE_MODULE_PATH}
  )

#-----------------------------------------------------------------------------
# CMake function(s) and macro(s)
#-----------------------------------------------------------------------------

include(CMakeParseArguments)
include(CMakePackageConfigHelpers)
include(CheckCXXSourceCompiles)
include(usFunctionAddResources)
include(usFunctionEmbedResources)
include(usFunctionGetResourceSource)
include(usFunctionCheckResourceLinking)
include(usFunctionCheckCompilerFlags)
include(usFunctionGenerateBundleInit)
include(usMacroCreateBundle)

if(US_BUILD_TESTING)
  include(usFunctionCompileSnippets)
endif()

#-----------------------------------------------------------------------------
# Init output directories
#-----------------------------------------------------------------------------

set(US_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(US_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(US_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

foreach(_type ARCHIVE LIBRARY RUNTIME)
  if(NOT CMAKE_${_type}_OUTPUT_DIRECTORY)
    set(CMAKE_${_type}_OUTPUT_DIRECTORY ${US_${_type}_OUTPUT_DIRECTORY})
  endif()
endforeach()

#-----------------------------------------------------------------------------
# Set a default build type if none was specified
#-----------------------------------------------------------------------------

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)

  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
               STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

#-----------------------------------------------------------------------------
# CMake options
#-----------------------------------------------------------------------------

function(us_cache_var _var_name _var_default _var_type _var_help)
  set(_advanced 0)
  set(_force)
  foreach(_argn ${ARGN})
    if(_argn STREQUAL ADVANCED)
      set(_advanced 1)
    elseif(_argn STREQUAL FORCE)
      set(_force FORCE)
    endif()
  endforeach()

  set(${_var_name} ${_var_default} CACHE ${_var_type} "${_var_help}" ${_force})
  if(_advanced)
    mark_as_advanced(${_var_name})
  endif()
endfunction()

# Determine the name of the install component for "SDK" artifacts.
# The default is "sdk"
if(NOT DEFINED US_SDK_INSTALL_COMPONENT)
  set(US_SDK_INSTALL_COMPONENT COMPONENT sdk)
elseif(US_SDK_INSTALL_COMPONENT)
  set(US_SDK_INSTALL_COMPONENT COMPONENT ${US_SDK_INSTALL_COMPONENT})
endif()

us_cache_var(CMAKE_DEBUG_POSTFIX d STRING "Executable and library debug name postfix" ADVANCED)

us_cache_var(US_ENABLE_THREADING_SUPPORT ON BOOL "Enable threading support")
us_cache_var(US_ENABLE_TSAN OFF BOOL "Enable tsan (thread sanitizer)" ADVANCED)
us_cache_var(US_ENABLE_COVERAGE OFF BOOL "Enable code coverage" ADVANCED)
us_cache_var(US_BUILD_SHARED_LIBS ON BOOL "Build shared libraries")
us_cache_var(US_BUILD_TESTING OFF BOOL "Build tests")
us_cache_var(US_BUILD_EXAMPLES OFF BOOL "Build example projects")
us_cache_var(US_USE_SYSTEM_GTEST OFF BOOL "Build using an external GTest installation" ADVANCED)

if(NOT US_ENABLE_THREADING_SUPPORT AND US_ENABLE_TSAN)
  message(SEND_WARN "Thread-sanitizer enabled without threading support. Forcing US_ENABLE_TSAN to OFF")
  set(US_ENABLE_TSAN OFF)
endif()

if(WIN32 AND NOT CYGWIN)
  set(default_runtime_install_dir bin/)
  set(default_library_install_dir bin/)
  set(default_archive_install_dir lib/)
  set(default_header_install_dir include/${PROJECT_NAME_LOWER}${US_GLOBAL_VERSION_SUFFIX})
  set(default_auxiliary_install_dir share/${PROJECT_NAME_LOWER}${US_GLOBAL_VERSION_SUFFIX})
  set(default_doc_install_dir ${default_auxiliary_install_dir}/doc)
  set(default_man_install_dir share/man)
else()
  set(default_runtime_install_dir bin/)
  set(default_library_install_dir lib/)
  set(default_archive_install_dir lib/)
  set(default_header_install_dir include/${PROJECT_NAME_LOWER}${US_GLOBAL_VERSION_SUFFIX})
  set(default_auxiliary_install_dir share/${PROJECT_NAME_LOWER}${US_GLOBAL_VERSION_SUFFIX})
  set(default_doc_install_dir ${default_auxiliary_install_dir}/doc)
  set(default_man_install_dir share/man)
endif()

us_cache_var(RUNTIME_INSTALL_DIR ${default_runtime_install_dir} STRING "Relative install location for binaries" ADVANCED)
us_cache_var(LIBRARY_INSTALL_DIR ${default_library_install_dir} STRING "Relative install location for libraries" ADVANCED)
us_cache_var(ARCHIVE_INSTALL_DIR ${default_archive_install_dir} STRING "Relative install location for archives" ADVANCED)
us_cache_var(HEADER_INSTALL_DIR ${default_header_install_dir} STRING "Relative install location for headers" ADVANCED)
us_cache_var(AUXILIARY_INSTALL_DIR ${default_auxiliary_install_dir} STRING "Relative install location for auxiliary files" ADVANCED)
us_cache_var(DOC_INSTALL_DIR ${default_doc_install_dir} STRING "Relative install location for documentation files" ADVANCED)
us_cache_var(MAN_INSTALL_DIR ${default_man_install_dir} STRING "Relative install location for man pages" ADVANCED)
set(AUXILIARY_CMAKE_INSTALL_DIR ${AUXILIARY_INSTALL_DIR}/cmake)

us_cache_var(BUILD_SHARED_LIBS ${US_BUILD_SHARED_LIBS} BOOL "Build shared libraries" ADVANCED)

set(US_BUNDLE_INIT_TEMPLATE "${US_CMAKE_DIR}/BundleInit.cpp" CACHE INTERNAL "The bundle initialization template code")
set(US_RESOURCE_RC_TEMPLATE "${US_CMAKE_DIR}/cppmicroservices_resources.rc.in" CACHE INTERNAL "The Windows RC resource template")
set(US_CMAKE_RESOURCE_DEPENDENCIES_CPP "${US_CMAKE_DIR}/CMakeResourceDependencies.cpp" CACHE INTERNAL "The dummy resource dependencies code")
if(APPLE)
  # Avoid RPATH warning in cmakegui
  set(CMAKE_MACOSX_RPATH ON)
endif()

# Cross-compiled builds need access to the host version of usResourceCompiler.
# ImportExecutables.cmake is generated by the host build and contains the cmake target
# for the host built usResourceCompiler. ImportExecutables.cmake can be included by the
# cross-compiled build to access the host built usResourceCompiler.
#
set(IMPORT_EXECUTABLES "${CMAKE_BINARY_DIR}/ImportExecutables.cmake" CACHE FILEPATH "A file containing the CMake target definition for the host build version of usResourceCompiler.")

#-----------------------------------------------------------------------------
# Testing configuration
#-----------------------------------------------------------------------------

function(us_add_tests test_driver)
  foreach(test ${ARGN})
    add_test(NAME ${test} COMMAND ${test_driver} ${test}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_property(TEST ${test} PROPERTY LABELS regular)
  if(US_MEMCHECK_COMMAND)
    add_test(NAME memcheck_${test} COMMAND ${US_MEMCHECK_COMMAND} --error-exitcode=1 ${US_RUNTIME_OUTPUT_DIRECTORY}/${test_driver} ${test}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_property(TEST memcheck_${test} PROPERTY LABELS valgrind memcheck)
    if(US_ENABLE_HELGRIND)
      # By default, these tests are not active until the concurrent bundle
      # start / stop mechanism has been properly implemented (GitHub Issue #25)
      add_test(NAME helgrind_${test} COMMAND ${US_MEMCHECK_COMMAND} --tool=helgrind --error-exitcode=1 ${US_RUNTIME_OUTPUT_DIRECTORY}/${test_driver} ${test}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
      set_property(TEST helgrind_${test} PROPERTY LABELS valgrind helgrind)
    endif()
  endif()
  if(US_COVERAGE_COMMAND)
    file(TO_NATIVE_PATH ${PROJECT_SOURCE_DIR} PROJECT_SOURCE_NATIVE_PATH)
    file(TO_NATIVE_PATH ${US_RUNTIME_OUTPUT_DIRECTORY} US_RUNTIME_OUTPUT_NATIVE_DIRECTORY)
    # skip the unicode test bundle because opencppcoverage tool fails if a module is located in a path with unicode characters
    add_test(NAME coverage_${test} COMMAND ${US_COVERAGE_COMMAND} --verbose --cover_children --sources=${PROJECT_SOURCE_NATIVE_PATH} --export_type=binary:${test}.cov --continue_after_cpp_exception --excluded_modules *TestBundleU.dll -- $<TARGET_FILE:${test_driver}> ${test}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_property(TEST coverage_${test} PROPERTY LABELS opencppcoverage)
  endif()
  endforeach()
endfunction()

# Valgrind rc file
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/valgrindrc
  ${CMAKE_BINARY_DIR}/.valgrindrc
  @ONLY
  )

if(US_BUILD_TESTING)
  enable_testing()
  find_program(US_MEMCHECK_COMMAND valgrind)
  mark_as_advanced(US_MEMCHECK_COMMAND)
  if(WIN32 AND US_ENABLE_COVERAGE)
    find_program(US_COVERAGE_COMMAND NAMES OpenCppCoverage OpenCppCoverage.exe DOC "path to the code coverage tool" HINTS "C:/Program Files/OpenCppCoverage" "C:/Program Files (x86)/OpenCppCoverage")
    mark_as_advanced(US_COVERAGE_COMMAND)
  endif()

  if(US_USE_SYSTEM_GTEST)
    find_package(GTest REQUIRED)
  else()
    # This keeps GTest CMake variables hidden from users unless they explicitly want to view/modify them.

    # Workaround an apparent bug in GCC 4.6 with C++11 variadic templates. Don't build gmock for any platform
    # for now (we don't use it yet anyway). See https://groups.google.com/forum/#!topic/googlemock/XvPDUENM5JM
    # for details.
    us_cache_var(BUILD_GMOCK OFF BOOL "Build GMock" ADVANCED FORCE)
    us_cache_var(BUILD_GTEST ON BOOL "Build GTest" ADVANCED FORCE)

    us_cache_var(gtest_build_samples OFF BOOL "Build GTest samples" ADVANCED)
    us_cache_var(gtest_build_tests OFF BOOL "Build GTest tests" ADVANCED)
    us_cache_var(gtest_disable_pthreads OFF BOOL "Disable the use of pthreads" ADVANCED)

    # Prevent overriding the parent project's compiler/linker
    # settings on Windows
    us_cache_var(gtest_force_shared_crt ON BOOL "" ADVANCED FORCE)

    us_cache_var(gtest_hide_internal_symbols OFF BOOL "" ADVANCED)

    # pthreads on MinGW is not supported, turn it off. Refer to
    # https://github.com/google/googletest/pull/721/files for details.
    if(MINGW)
     set(gtest_disable_pthreads ON CACHE BOOL "" FORCE)
    endif()

    set(GTEST_BOTH_LIBRARIES gtest gtest_main)

    # Prevent GTest from installing libraries into our install dir.
    set(GTEST_NO_INSTALL 1)

    # This project also sets BUILD_SHARED_LIBS. We want to guarantee that GTest is always built as a
    # static library even if CppMicroServices is a shared library.
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/googletest)
    set(BUILD_SHARED_LIBS ${US_BUILD_SHARED_LIBS} CACHE BOOL "" FORCE)
  endif()
endif()


#-----------------------------------------------------------------------------
# US C/CXX Flags
#-----------------------------------------------------------------------------

set(US_HAVE_VISIBILITY_ATTRIBUTE 0)
set(US_CXX11_FLAGS )
set(US_CXX_TSAN_FLAGS )
set(US_TSAN_LINK_FLAGS )
set(US_CXX_COVERAGE_FLAGS )

if(MSVC)
  set(disabled_warnings
    4251 # 'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'
  )
  foreach(wd ${disabled_warnings})
    set(US_CXX_FLAGS "/wd${wd} ${US_CXX_FLAGS}")
  endforeach()
  set(US_CXX_FLAGS "/MP /WX ${US_CXX_FLAGS}")
else()

  if(US_COMPILER_APPLE_CLANG)
    set(US_CXX_FLAGS "${US_CXX_FLAGS} -stdlib=libc++")
  endif()

  # If not cross-compiling, turn on Stack Smashing Protection.
  # This is done in cases where libssp.so may not be available in the
  # cross build toolchain. The dependency with libssp.so was originally
  # discovered building with the Android ARM gcc toolchain. It is
  # being applied to any cross compilation as a precaution.
  if(NOT CMAKE_CROSSCOMPILING)
    usFunctionCheckCompilerFlags(-fstack-protector-all US_CXX_FLAGS)
  endif()

  foreach(_cxxflag  -Werror -Wall -Wextra -Wpointer-arith -Winvalid-pch -Wcast-align
                    -Wwrite-strings -Woverloaded-virtual -Wnon-virtual-dtor -Wold-style-cast
                    -Wstrict-null-sentinel -Wsign-promo -fdiagnostics-show-option -Wno-deprecated-copy -Wno-deprecated-declarations)
    usFunctionCheckCompilerFlags(${_cxxflag} US_CXX_FLAGS)
  endforeach()

  if(US_COMPILER_CLANG OR US_COMPILER_APPLE_CLANG)
    # The check does not work reliably for gcc
    usFunctionCheckCompilerFlags(-Wno-mismatched-tags US_CXX_FLAGS)
  endif()

  # Issue #56: Workaround a clang compiler issue (https://llvm.org/bugs/show_bug.cgi?id=26749).
  # The typeinfo symbols do not have reliable symbol visibility. Instead of exporting only
  # symbols and associated typeinfo symbols which have explicit "default" visibility,
  # export all typeinfo symbols by default and maintain hidden symbol visibility for all other
  # symbols (unless explicitly overriden). See
  # https://developer.apple.com/library/mac/technotes/tn2185/_index.html#//apple_ref/doc/uid/DTS10004200-CH1-SUBSECTION2
  # to understand what -fvisibility-ms-compat does.
  if(APPLE AND (US_COMPILER_CLANG OR US_COMPILER_APPLE_CLANG))
    usFunctionCheckCompilerFlags("-fvisibility-ms-compat" _have_visibility)
  else()
    usFunctionCheckCompilerFlags("-fvisibility=hidden -fvisibility-inlines-hidden" _have_visibility)
  endif()

  if(_have_visibility)
    set(US_HAVE_VISIBILITY_ATTRIBUTE 1)
    set(US_CXX_FLAGS "${US_CXX_FLAGS} ${_have_visibility}")
  endif()

  usFunctionCheckCompilerFlags("-O1 -D_FORTIFY_SOURCE=2" _fortify_source_flag)
  if(_fortify_source_flag)
    set(US_CXX_FLAGS_RELEASE "${US_CXX_FLAGS_RELEASE} -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2")
  endif()

  set(_orig_req_libs ${CMAKE_REQUIRED_LIBRARIES})
  set(CMAKE_REQUIRED_LIBRARIES "-fsanitize=thread")
  usFunctionCheckCompilerFlags("-O1 -fsanitize=thread" US_CXX_TSAN_FLAGS)
  set(CMAKE_REQUIRED_LIBRARIES "${_orig_req_libs}")
  if(NOT US_CXX_TSAN_FLAGS AND US_ENABLE_TSAN)
    message(WARNING "TSAN requested, but compiler does not recognize it")
  endif()

  set(CMAKE_REQUIRED_LIBRARIES "-fprofile-arcs -ftest-coverage")
  usFunctionCheckCompilerFlags("-fprofile-arcs -ftest-coverage" US_CXX_COVERAGE_FLAGS)
  set(CMAKE_REQUIRED_LIBRARIES "${_orig_req_libs}")
  if(NOT US_CXX_COVERAGE_FLAGS AND US_ENABLE_COVERAGE)
    message(WARNING "Code coverage requested, but compiler does not recognize it")
  endif()
endif()

if(MINGW)
  # suppress warnings about auto imported symbols
  set(US_CXX_FLAGS "-Wl,--enable-auto-import ${US_CXX_FLAGS}")
  # we need to define a Windows version (Windows Vista)
  set(US_CXX_FLAGS "-D_WIN32_WINNT=0x0600 -DNTDDI_VERSION=0x06000000 ${US_CXX_FLAGS}")
endif()

if(US_ENABLE_THREADING_SUPPORT)
  set(THREADS_PREFER_PTHREAD_FLAG 1)
  find_package(Threads REQUIRED)

  if(US_ENABLE_TSAN AND US_CXX_TSAN_FLAGS)
    set(US_CXX_FLAGS "${US_CXX_FLAGS} ${US_CXX_TSAN_FLAGS}")
  endif()
endif()

if(US_ENABLE_COVERAGE AND US_CXX_COVERAGE_FLAGS)
  # US_CXX_COVERAGE_FLAGS is used for both the compiler and the linker
  set(US_CXX_FLAGS "${US_CXX_FLAGS} ${US_CXX_COVERAGE_FLAGS}")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${US_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${US_CXX_FLAGS_RELEASE}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${US_C_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${US_C_FLAGS_RELEASE}")

#-----------------------------------------------------------------------------
# US Link Flags
#-----------------------------------------------------------------------------

set(US_LINK_FLAGS )
if(CMAKE_COMPILER_IS_GNUCXX AND NOT APPLE)
  foreach(_linkflag -Wl,--no-undefined)
    set(_add_flag)
    usFunctionCheckCompilerFlags("${_linkflag}" _add_flag)
    if(_add_flag)
      set(US_LINK_FLAGS "${US_LINK_FLAGS} ${_linkflag}")
    endif()
  endforeach()
endif()

if(NOT MSVC)
  if (US_ENABLE_TSAN AND US_CXX_TSAN_FLAGS)
    set(US_TSAN_LINK_FLAGS "-fsanitize=thread")
  endif()
endif()

if(US_ENABLE_THREADING_SUPPORT AND US_TSAN_LINK_FLAGS)
  set(US_LINK_FLAGS "${US_LINK_FLAGS} ${US_TSAN_LINK_FLAGS}")
endif()

if(US_ENABLE_COVERAGE AND US_CXX_COVERAGE_FLAGS)
  # US_CXX_COVERAGE_FLAGS is used for both the compiler and the linker
  set(US_LINK_FLAGS "${US_LINK_FLAGS} ${US_CXX_COVERAGE_FLAGS}")
endif()

usFunctionCheckResourceLinking()

#-----------------------------------------------------------------------------
# US Header Checks
#-----------------------------------------------------------------------------

include(CheckIncludeFileCXX)

CHECK_INCLUDE_FILE_CXX(cxxabi.h US_HAVE_CXXABI_H)

#-----------------------------------------------------------------------------
# C++ language support
#-----------------------------------------------------------------------------

set(disabled_func )

file(GLOB try_compile_srcs ${CMAKE_CURRENT_SOURCE_DIR}/cmake/try_compile/*.cpp)
foreach(try_compile_src ${try_compile_srcs})
  get_filename_component(try_compile_var ${try_compile_src} NAME_WE)
  string(TOUPPER ${try_compile_var} try_compile_var)
  try_compile(
    ${try_compile_var}
    ${CMAKE_CURRENT_BINARY_DIR}/try_compile/${try_compile_var}
    SOURCES ${try_compile_src}
    CMAKE_FLAGS -DCMAKE_CXX_EXTENSIONS:BOOL=OFF -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=ON -DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}
    # The line below only works on CMake 3.8 and higher
    # CXX_STANDARD ${CMAKE_CXX_STANDARD}
  )
endforeach()

if(MINGW)
  #
  # The MinGW winpthread implementation does not properly destruct
  # non-POD thread-local objects, leading to segmentation faults on
  # thread exit. See
  #
  # https://github.com/Alexpux/MINGW-packages/issues/2519
  # https://sourceforge.net/p/mingw-w64/bugs/527/
  #
  set(US_HAVE_THREAD_LOCAL 0)
  set(disabled_func "${disabled_func}MinGW winpthread: ")
endif()

if(NOT US_HAVE_THREAD_LOCAL AND US_ENABLE_THREADING_SUPPORT)
  set(disabled_func "${disabled_func}thread_local support unavailable. Recursive service factory calls will not be detected. See also https://github.com/CppMicroServices/CppMicroServices/issues/213\n")
endif()

#-----------------------------------------------------------------------------
# C++ library support
#-----------------------------------------------------------------------------

# c++11 regular expressions are used in the unit tests.
# Currently the default C++ library for gcc < 4.9 doesn't
# fully support c++11 regex.
# Generalize the detection of regex and make its availability
# conditional of whether the compiler's C++ library supports it
# or not.
# This can be removed once the minimum supported gcc version is 4.9
if(NOT US_HAVE_REGEX AND US_BUILD_TESTING)
  set(disabled_func "${disabled_func}regular expression support unavailable. Some tests will be disabled.\n")
endif()

# Print a warning summary
if (disabled_func)
  message(WARNING "${US_COMPILER_${CMAKE_CXX_COMPILER_ID}_MINIMUM_RECOMMENDED_VERSION_PRODUCT} or higher recommended (you are using ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}). The following functionality will not be available:\n${disabled_func}")
endif()

#-----------------------------------------------------------------------------
# System Information
#-----------------------------------------------------------------------------

include(TestBigEndian)

test_big_endian(US_BIG_ENDIAN)

if(NOT US_BIG_ENDIAN)
  set(US_LITTLE_ENDIAN 1)
endif()

#-----------------------------------------------------------------------------
# Source directory
#-----------------------------------------------------------------------------

# Configure CppMicroServicesConfig.cmake for the build tree.
# The file is used in sub-directories.

set(PACKAGE_CONFIG_RUNTIME_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(PACKAGE_CONFIG_CMAKE_DIR ${US_CMAKE_DIR})

set(US_RCC_EXECUTABLE_TARGET usResourceCompiler CACHE INTERNAL "The target name of the usResourceCompiler executable.")
set(US_RCC_EXECUTABLE_OUTPUT_NAME ${US_RCC_EXECUTABLE_TARGET}${US_GLOBAL_VERSION_SUFFIX})

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}Config.cmake.in
  ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  @ONLY
  )

set(global_config_h_file "${PROJECT_BINARY_DIR}/include/${PROJECT_NAME_LOWER}/GlobalConfig.h")
configure_file(${US_CMAKE_DIR}/GlobalConfig.h.in ${global_config_h_file})

# Version information
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}ConfigVersion.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  @ONLY
  )

add_subdirectory(tools/rc)

export(TARGETS ${US_RCC_EXECUTABLE_TARGET}
       FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake)

add_subdirectory(util)
add_subdirectory(framework)

#-----------------------------------------------------------------------------
# Documentation
#-----------------------------------------------------------------------------

add_subdirectory(doc)

#-----------------------------------------------------------------------------
# Installation
#-----------------------------------------------------------------------------

if(NOT US_NO_INSTALL)

install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        DESTINATION ${AUXILIARY_CMAKE_INSTALL_DIR})

  set(_install_cmake_scripts
    ${US_BUNDLE_INIT_TEMPLATE}
    ${US_CMAKE_RESOURCE_DEPENDENCIES_CPP}
    ${US_CMAKE_DIR}/usFunctionGenerateBundleInit.cmake
    ${US_CMAKE_DIR}/usFunctionAddResources.cmake
    ${US_CMAKE_DIR}/usFunctionEmbedResources.cmake
    ${US_CMAKE_DIR}/usFunctionGetResourceSource.cmake
    ${US_CMAKE_DIR}/usFunctionCheckResourceLinking.cmake
    ${US_CMAKE_DIR}/usFunctionCheckCompilerFlags.cmake
    )

  install(FILES ${_install_cmake_scripts}
          DESTINATION ${AUXILIARY_CMAKE_INSTALL_DIR})

  install(FILES ${global_config_h_file}
          DESTINATION ${HEADER_INSTALL_DIR}/${PROJECT_NAME_LOWER})

  # Configure CppMicroServicesConfig.cmake for the install tree
  set(CONFIG_INCLUDE_DIR ${HEADER_INSTALL_DIR})
  set(CONFIG_RUNTIME_DIR ${RUNTIME_INSTALL_DIR})
  set(CONFIG_CMAKE_DIR ${AUXILIARY_CMAKE_INSTALL_DIR})
  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}Config.cmake.in
    ${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${AUXILIARY_CMAKE_INSTALL_DIR}
    PATH_VARS CONFIG_INCLUDE_DIR CONFIG_RUNTIME_DIR CONFIG_CMAKE_DIR
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECT_NAME}Config.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
          DESTINATION ${AUXILIARY_CMAKE_INSTALL_DIR}
          ${US_SDK_INSTALL_COMPONENT}
    )

endif()
