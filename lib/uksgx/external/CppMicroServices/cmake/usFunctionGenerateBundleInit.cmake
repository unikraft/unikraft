#.rst:
#
# .. cmake:command:: usFunctionGenerateBundleInit
#
# Generate a source file which handles proper initialization of a bundle.
#
# .. code-block:: cmake
#
#    usFunctionGenerateBundleInit(TARGET target OUT <out_var>)
#
# This CMake function will append the path to a generated source file to the
# out_var variable, which should be compiled into a bundle. The bundles source
# code must be compiled with the ``US_BUNDLE_NAME`` pre-processor definition.
#
# .. code-block:: cmake
#    :caption: Example
#
#    set(bundle_srcs )
#    usFunctionGenerateBundleInit(TARGET mylib OUT bundle_srcs)
#    add_library(mylib ${bundle_srcs})
#    set_property(TARGET ${mylib} APPEND PROPERTY COMPILE_DEFINITIONS US_BUNDLE_NAME=MyBundle)
#
# **One-value keywords**
#    * ``TARGET`` (required): The name of the target for which the source file
#      is generated.
#    * ``OUT`` (required): A list variable to which the path of the generated
#      source file will be appended.
#
function(usFunctionGenerateBundleInit)
  cmake_parse_arguments(_gen "" "TARGET;OUT" "" "" ${ARGN})
  if(NOT _gen_TARGET)
    message(SEND_ERROR "TARGET must not be empty")
  endif()
  if(NOT _gen_OUT)
    message(SEND_ERROR "OUT argument must not be empty")
  endif()

  set(_out "${CMAKE_CURRENT_BINARY_DIR}/${_gen_TARGET}/cppmicroservices_init.cpp")
  configure_file(${US_BUNDLE_INIT_TEMPLATE} ${_out} @ONLY)

  set(${_gen_OUT} ${${_gen_OUT}} ${_out} PARENT_SCOPE)
endfunction()
