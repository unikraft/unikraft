
#.rst:
#
# .. cmake:command:: usFunctionEmbedResources
#
# Embed resources in a library or executable.
#
# .. code-block:: cmake
#
#    usFunctionEmbedResources(TARGET target [BUNDLE_NAME bundle_name] [APPEND | LINK]
#      [WORKING_DIRECTORY dir] [COMPRESSION_LEVEL level]
#      [FILES res1...] [ZIP_ARCHIVES archive1...])
#
# This CMake function uses an external command line program to generate a ZIP archive
# containing data from external resources such as text files or images or other ZIP
# archives. External resources can be added to a bundle using the
# :cmake:command:`usFunctionAddResources` function or directly using this function
# using similar parameters. The created archive file is appended or embedded as a
# binary blob to the target file.
#
# .. note::
#
#    To set-up correct file dependencies from your bundle target to your resource
#    files, you have to add a special source file to the source list of the target.
#    The source file name can be retrieved by using :cmake:command:`usFunctionGetResourceSource`.
#    This ensures that changed resource files will automatically be re-added to the
#    bundle.
#
# There are two differend modes for including resources: *APPEND* and *LINK*. In *APPEND* mode,
# the generated zip file is appended at the end of the target file. In *LINK* mode, the
# zip file is compiled / linked into the target using platform specific techniques. *LINK*
# mode is necessary if certain tools make additional assumptions about the object layout
# of the target file (e.g. codesign on MacOS). *LINK* mode may result in slower bundle
# initialization and bigger object files. The default mode is *LINK* mode on MacOS and
# *APPEND* mode on all other platforms.
#
# .. code-block:: cmake
#    :caption: Example
#
#    usFunctionEmbedResources(TARGET mylib
#                             BUNDLE_NAME org_me_mylib
#                             FILES config.properties logo.png
#                            )
#
# **One-value keywords**
#    * ``TARGET`` (required): The target to which the resource files are added.
#    * ``BUNDLE_NAME`` (required/optional): The bundle name of the target, as specified in
#      the ``US_BUNDLE_NAME`` pre-processor definition of that target. This parameter
#      is optional if a target property with the name ``US_BUNDLE_NAME`` exists, containing
#      the required bundle name.
#
# **Options**
#    * ``APPEND``: Append the resources zip file to the target file.
#    * ``LINK``: Link (embed) the resources zip file if possible.
#
# For the ``WORKING_DIRECTORY``, ``COMPRESSION_LEVEL``, ``FILES``, ``ZIP_ARCHIVES`` parameters see the
# documentation of the usFunctionAddResources macro which is called with these parameters if set.
#
# .. seealso::
#
#    | :cmake:command:`usFunctionAddResources`
#    | :cmake:command:`usFunctionGetResourceSource`
#    | :any:`concept-resources`
#
function(usFunctionEmbedResources)

  cmake_parse_arguments(US_RESOURCE "APPEND;LINK" "TARGET;BUNDLE_NAME;WORKING_DIRECTORY;COMPRESSION_LEVEL" "FILES;ZIP_ARCHIVES" ${ARGN})

  if(NOT US_RESOURCE_TARGET)
    message(SEND_ERROR "TARGET argument not specified.")
  endif()

  if(US_RESOURCE_FILES OR US_RESOURCE_ZIP_ARCHIVES)
    usFunctionAddResources(TARGET ${US_RESOURCE_TARGET}
      BUNDLE_NAME ${US_RESOURCE_BUNDLE_NAME}
      WORKING_DIRECTORY ${US_RESOURCE_WORKING_DIRECTORY}
      COMPRESSION_LEVEL ${US_RESOURCE_COMPRESSION_LEVEL}
      FILES ${US_RESOURCE_FILES}
      ZIP_ARCHIVES ${US_RESOURCE_ZIP_ARCHIVES}
    )
  endif()

  get_target_property(_res_zips ${US_RESOURCE_TARGET} _us_resource_zips)
  if(NOT _res_zips)
    return()
  endif()

  if(US_RESOURCE_APPEND AND US_RESOURCE_LINK)
    message(WARNING "Both APPEND and LINK options specified. Falling back to default behaviour.")
    set(US_RESOURCE_APPEND 0)
    set(US_RESOURCE_LINK 0)
  endif()

  if(US_RESOURCE_LINK AND NOT US_RESOURCE_LINKING_AVAILABLE)
    message(WARNING "Resource linking not available. Falling back to APPEND mode.")
    set(US_RESOURCE_LINK 0)
    set(US_RESOURCE_APPEND 1)
  endif()

  # Set default resource mode
  if(NOT US_RESOURCE_APPEND AND NOT US_RESOURCE_LINK)
    if(US_DEFAULT_RESOURCE_MODE STREQUAL "LINK")
      set(US_RESOURCE_LINK 1)
    else()
      set(US_RESOURCE_APPEND 1)
    endif()
  endif()

  set(_mode )
  if(US_RESOURCE_LINK)
    set(_mode LINK)
  elseif(US_RESOURCE_APPEND)
    set(_mode APPEND)
  endif()
  usFunctionGetResourceSource(TARGET ${US_RESOURCE_TARGET} OUT _source_output ${_mode})

  if(NOT US_RESOURCE_WORKING_DIRECTORY)
    set(US_RESOURCE_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
  if(NOT IS_ABSOLUTE ${US_RESOURCE_WORKING_DIRECTORY})
    set(US_RESOURCE_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${US_RESOURCE_WORKING_DIRECTORY}")
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

  set(_zip_archive )
  get_target_property(_counter ${US_RESOURCE_TARGET} _us_resource_counter)
  if(_counter EQUAL 0)
    set(_zip_archive ${_res_zips})
  else()
    set(_zip_archive ${CMAKE_CURRENT_BINARY_DIR}/${US_RESOURCE_TARGET}/res.zip)
    if(_res_zips)
      set(_zip_args )
      foreach(_file ${_res_zips})
        list(APPEND _zip_args -z)
        list(APPEND _zip_args ${_file})
      endforeach()
    endif()
    add_custom_command(
      OUTPUT ${_zip_archive}
      COMMAND ${resource_compiler} -o ${_zip_archive} -n dummy ${_zip_args}
      DEPENDS ${_res_zips} ${resource_compiler}
      COMMENT "Creating resources zip file for ${US_RESOURCE_TARGET}"
      VERBATIM
     )
  endif()
  get_filename_component(_zip_archive_name ${_zip_archive} NAME)
  get_filename_component(_zip_archive_path ${_zip_archive} PATH)

  if(US_RESOURCE_LINK)
    if(APPLE)
      # This resolves an issue where passing in CMAKE_CXX_FLAGS using
      # -D CMAKE_CXX_FLAGS:STRING="..." results in a quoted string being passed to
      # the command below which results in the command being:
      #  >> clang "..." -c ... -o stub.o
      # when it should really be:
      #  >> clang ... -c ... -o stub.o
      # (pardon the elipsis for abbreviation)
      separate_arguments(_us_resource_cxx_flags UNIX_COMMAND ${CMAKE_CXX_FLAGS})

      # section name is "us_resources" because max length for section names in Mach-O format is 16 characters.
      add_custom_command(
        OUTPUT ${_source_output}
        COMMAND ${CMAKE_CXX_COMPILER} ${_us_resource_cxx_flags} -c ${US_CMAKE_RESOURCE_DEPENDENCIES_CPP} -o stub.o
        COMMAND ${CMAKE_LINKER} -r -sectcreate __TEXT us_resources ${_zip_archive_name} stub.o -o ${_source_output}
        DEPENDS ${_zip_archive}
        WORKING_DIRECTORY ${_zip_archive_path}
        COMMENT "Linking resources zip file for ${US_RESOURCE_TARGET}"
        VERBATIM
       )
      set_source_files_properties(${_source_output} PROPERTIES EXTERNAL_OBJECT 1 GENERATED 1)
    elseif(WIN32 AND CMAKE_RC_COMPILER)
      set(US_RESOURCE_ARCHIVE ${_zip_archive})
      # If the file generated in "Configure" step is specified as the OUTPUT of add_custom_command,
      # "Clean" target will delete the file and subsequent build will fail. To avoid failures in
      # "ReBuild", generate the resource file in the "Configure" step and copy it in "Build" step
      configure_file(${US_RESOURCE_RC_TEMPLATE} ${_source_output}_autogen)
      add_custom_command(
        OUTPUT ${_source_output}
        COMMAND ${CMAKE_COMMAND} -E copy ${_source_output}_autogen ${_source_output}
        DEPENDS ${_zip_archive}
        WORKING_DIRECTORY ${_zip_archive_path}
        COMMENT "Linking resources zip file for ${US_RESOURCE_TARGET}"
        VERBATIM
       )
       set_source_files_properties(${_source_output} PROPERTIES GENERATED 1)
    elseif(UNIX)
      add_custom_command(
        OUTPUT ${_source_output}
        COMMAND ${CMAKE_LINKER} -r -b binary -o ${_source_output} ${_zip_archive_name}
        COMMAND objcopy --rename-section .data=.rodata,alloc,load,readonly,data,contents ${_source_output} ${_source_output}
        DEPENDS ${_zip_archive}
        WORKING_DIRECTORY ${_zip_archive_path}
        COMMENT "Linking resources zip file for ${US_RESOURCE_TARGET}"
        VERBATIM
       )
      set_source_files_properties(${_source_output} PROPERTIES EXTERNAL_OBJECT 1 GENERATED 1)
    else()
      message(WARNING "Internal error: Resource linking not available. Falling back to APPEND mode.")
      set(US_RESOURCE_LINK 0)
      set(US_RESOURCE_APPEND 1)
    endif()
  endif()

  if(US_RESOURCE_APPEND)
    # This command depends on the given resource files and creates a source
    # file which must be added to the source list of the related target.
    # This way, the following command is executed if the resources change
    # and it just touches the created source file to force a (actually unnecessary)
    # re-linking and hence the execution of POST_BUILD commands.
    add_custom_command(
      OUTPUT ${_source_output}
      COMMAND ${CMAKE_COMMAND} -E copy ${US_CMAKE_RESOURCE_DEPENDENCIES_CPP} ${_source_output}
      DEPENDS ${_zip_archive}
      COMMENT "Checking resource dependencies for ${US_RESOURCE_TARGET}"
      VERBATIM
     )

    add_custom_command(
      TARGET ${US_RESOURCE_TARGET}
      POST_BUILD
      COMMAND objcopy --add-section .note.sgx.aesm_resource=${_zip_archive} $<TARGET_FILE:${US_RESOURCE_TARGET}>
      WORKING_DIRECTORY ${US_RESOURCE_WORKING_DIRECTORY}
      COMMENT "Appending zipped resources to ${US_RESOURCE_TARGET}"
      VERBATIM
    )
  endif()

endfunction()
