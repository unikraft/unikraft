
#.rst:
#
# .. cmake:command:: usFunctionAddResources
#
# Add resources to a library or executable.
#
# .. code-block:: cmake
#
#    usFunctionAddResources(TARGET target [BUNDLE_NAME bundle_name]
#      [WORKING_DIRECTORY dir] [COMPRESSION_LEVEL level]
#      [FILES res1...] [ZIP_ARCHIVES archive1...])
#
# This CMake function uses an external command line program to generate a ZIP archive
# containing data from external resources such as text files or images or other ZIP
# archives. The created archive file can be appended or linked into the target file
# using the :cmake:command:`usFunctionEmbedResources` function.
#
# Each bundle can call this function to add resources and make them available at
# runtime through the Bundle class. Multiple calls to this function append the
# input files.
#
# In the case of linking static bundles which contain resources to the target bundle,
# adding the static bundle target name to the ``ZIP_ARCHIVES`` list will merge its
# resources into the target bundle.
#
# .. code-block:: cmake
#    :caption: Example
#
#    set(bundle_srcs )
#    usFunctionAddResources(TARGET mylib
#                           BUNDLE_NAME org_me_mylib
#                           FILES config.properties logo.png
#                          )
#
# **One-value keywords**
#    * ``TARGET`` (required): The target to which the resource files are added.
#    * ``BUNDLE_NAME`` (required/optional): The bundle name of the target, as specified in
#      the \c US_BUNDLE_NAME pre-processor definition of that target. This parameter
#      is optional if a target property with the name US_BUNDLE_NAME exists, containing
#      the required bundle name.
#    * ``COMPRESSION_LEVEL`` (optional): The zip compression level (0-9). Defaults to the default zip
#      level. Level 0 disables compression.
#    * ``WORKING_DIRECTORY`` (optional): The root path for all resource files listed after the
#      FILES argument. If no or a relative path is given, it is considered relative to the
#      current CMake source directory.
#
# **Multi-value keywords**
#    * ``FILES`` (optional): A list of resource files (paths to external files in the file system)
#      relative to the current working directory.
#    * ``ZIP_ARCHIVES`` (optional): A list of zip archives (relative to the current working directory
#      or absolute file paths) whose contents is merged into the target bundle. If a list entry
#      is a valid target name and that target is a static library, its absolute file path is
#      used instead.
#
# .. seealso::
#
#    | :cmake:command:`usFunctionEmbedResources`
#    | :any:`concept-resources`
#
function(usFunctionAddResources)

  cmake_parse_arguments(US_RESOURCE "" "TARGET;BUNDLE_NAME;WORKING_DIRECTORY;COMPRESSION_LEVEL" "FILES;ZIP_ARCHIVES" ${ARGN})

  if(NOT US_RESOURCE_TARGET)
    message(SEND_ERROR "TARGET argument not specified.")
  endif()

  if(NOT US_RESOURCE_BUNDLE_NAME)
    get_target_property(US_RESOURCE_BUNDLE_NAME ${US_RESOURCE_TARGET} US_BUNDLE_NAME)
    if(NOT US_RESOURCE_BUNDLE_NAME)
      message(SEND_ERROR "Either the BUNDLE_NAME argument or the US_BUNDLE_NAME target property is required.")
    endif()
  endif()

  if(NOT US_RESOURCE_FILES AND NOT US_RESOURCE_ZIP_ARCHIVES)
    message(WARNING "No resources specified. Skipping resource processing.")
    return()
  endif()

  if(NOT US_RESOURCE_WORKING_DIRECTORY)
    set(US_RESOURCE_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
  if(NOT IS_ABSOLUTE ${US_RESOURCE_WORKING_DIRECTORY})
    set(US_RESOURCE_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${US_RESOURCE_WORKING_DIRECTORY}")
  endif()

  if(US_RESOURCE_COMPRESSION_LEVEL)
    set(cmd_line_args -c ${US_RESOURCE_COMPRESSION_LEVEL})
  endif()

  if(CMAKE_CROSSCOMPILING)
    # Cross-compiled builds need to use the imported host version of usResourceCompiler
    include(${IMPORT_EXECUTABLES})
    set(resource_compiler native-${US_RCC_EXECUTABLE_TARGET})
  else()
    set(resource_compiler ${US_RCC_EXECUTABLE})
    if(TARGET ${US_RCC_EXECUTABLE_TARGET})
      set(resource_compiler ${US_RCC_EXECUTABLE_TARGET})
    elseif(NOT resource_compiler)
      message(FATAL_ERROR "The CppMicroServices resource compiler was not found. Check the US_RCC_EXECUTABLE CMake variable.")
    endif()
  endif()

  set(_cmd_deps )
  foreach(_file ${US_RESOURCE_FILES})
    if(IS_ABSOLUTE ${_file})
      list(APPEND _cmd_deps ${_file})
    else()
      list(APPEND _cmd_deps ${US_RESOURCE_WORKING_DIRECTORY}/${_file})
    endif()
  endforeach()

  set(_zip_args )
  if(US_RESOURCE_ZIP_ARCHIVES)
    foreach(_zip_archive ${US_RESOURCE_ZIP_ARCHIVES})
      if(TARGET ${_zip_archive})
        get_target_property(_is_static_lib ${_zip_archive} TYPE)
        if(_is_static_lib STREQUAL "STATIC_LIBRARY")
          list(APPEND _cmd_deps ${_zip_archive})
          list(APPEND _zip_args $<TARGET_FILE:${_zip_archive}>)
        endif()
      else()
        if(IS_ABSOLUTE ${_zip_archive})
          list(APPEND _cmd_deps ${_zip_archive})
        else()
          list(APPEND _cmd_deps ${US_RESOURCE_WORKING_DIRECTORY}/${_zip_archive})
        endif()
        list(APPEND _zip_args ${_zip_archive})
      endif()
    endforeach()
  endif()

  if(NOT US_RESOURCE_FILES AND NOT _zip_args)
    return()
  endif()

  if(US_RESOURCE_FILES)
    set(_file_args )
    foreach(_file ${US_RESOURCE_FILES})
      list(APPEND _file_args -r)
      list(APPEND _file_args ${_file})
    endforeach()
  endif()
  if(_zip_args)
    set(_us_zip_args )
    foreach(_file ${_zip_args})
      list(APPEND _us_zip_args -z)
      list(APPEND _us_zip_args ${_file})
    endforeach()
  endif()

  if(US_RESOURCE_BUNDLE_NAME)
    set(_bundle_args -n ${US_RESOURCE_BUNDLE_NAME})
  endif()

  get_target_property(_counter ${US_RESOURCE_TARGET} _us_resource_counter)
  if((NOT ${_counter} EQUAL 0) AND NOT _counter)
    set(_counter 0)
  else()
    math(EXPR _counter "${_counter} + 1")
  endif()

  set(_res_zip "${CMAKE_CURRENT_BINARY_DIR}/${US_RESOURCE_TARGET}/res_${_counter}.zip")

  add_custom_command(
    OUTPUT ${_res_zip}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/${US_RESOURCE_TARGET}"
    COMMAND ${resource_compiler} ${cmd_line_args} -o ${_res_zip} ${_bundle_args} ${_file_args} ${_us_zip_args}
    WORKING_DIRECTORY ${US_RESOURCE_WORKING_DIRECTORY}
    DEPENDS ${_cmd_deps} ${resource_compiler}
    COMMENT "Checking resource dependencies for ${US_RESOURCE_TARGET}"
    VERBATIM
  )

  get_target_property(_res_zips ${US_RESOURCE_TARGET} _us_resource_zips)
  if(NOT _res_zips)
    set(_res_zips )
  endif()
  list(APPEND _res_zips ${_res_zip})

  set_target_properties(${US_RESOURCE_TARGET} PROPERTIES
    _us_resource_counter "${_counter}"
    _us_resource_zips "${_res_zips}"
  )

endfunction()
