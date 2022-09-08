#-----------------------------------------------------------------------------
# Build the tutorial and example code
#-----------------------------------------------------------------------------

if(US_BUILD_EXAMPLES AND NOT ${PROJECT_NAME}_SKIP_EXAMPLES AND
   EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/CMakeLists.txt)
  set(CppMicroServices_DIR ${CppMicroServices_BINARY_DIR})
  add_subdirectory(src)
endif()

#-----------------------------------------------------------------------------
# Build the documentation itself
#-----------------------------------------------------------------------------

us_cache_var(US_BUILD_DOC_MAN OFF BOOL "Build man pages")
us_cache_var(US_BUILD_DOC_HTML OFF BOOL "Build html documentation")

find_package(Doxygen)

find_program(SPHINX_EXECUTABLE
  NAMES sphinx-build
  DOC "Sphinx Documentation Builder (sphinx-doc.org)"
  )
us_cache_var(SPHINX_FLAGS "" STRING "Flags to pass to sphinx-build" ADVANCED)
separate_arguments(sphinx_flags UNIX_COMMAND "${SPHINX_FLAGS}")

if(NOT US_BUILD_DOC_MAN AND NOT US_BUILD_DOC_HTML)
  return()
elseif(NOT SPHINX_EXECUTABLE)
  message(FATAL_ERROR "SPHINX_EXECUTABLE (sphinx-build) is not found!")
elseif(NOT Doxygen_FOUND)
  message(FATAL_ERROR "Doxygen is not found!")
endif()

set(doc_formats "")
if(US_BUILD_DOC_HTML)
  list(APPEND doc_formats html)
endif()
if(US_BUILD_DOC_MAN)
  list(APPEND doc_formats man)
endif()

set(doc_format_outputs "")
set(doc_format_last "")
foreach(format ${doc_formats})
  set(doc_format_output "doc_format_${format}")
  set(doc_format_log "build-${format}.log")
  add_custom_command(
    OUTPUT ${doc_format_output}
    COMMAND ${SPHINX_EXECUTABLE}
            -c ${CppMicroServices_SOURCE_DIR}
            -d ${CppMicroServices_BINARY_DIR}/doctrees
            -b ${format}
            ${sphinx_flags}
            ${CppMicroServices_SOURCE_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}/${format}
            > ${doc_format_log} # log stdout, pass stderr
    DEPENDS ${doc_format_last}
    WORKING_DIRECTORY ${CppMicroServices_SOURCE_DIR}
    COMMENT "sphinx-build ${format}: see doc/${doc_format_log}"
    VERBATIM
    )
  set_property(SOURCE ${doc_format_output} PROPERTY SYMBOLIC 1)
  list(APPEND doc_format_outputs ${doc_format_output})
  set(doc_format_last ${doc_format_output})
endforeach()

add_custom_target(doc ALL DEPENDS ${doc_format_outputs})

if(US_BUILD_DOC_MAN AND NOT US_NO_INSTALL)
  file(GLOB man_rst RELATIVE ${CppMicroServices_SOURCE_DIR}/doc/man
    ${CppMicroServices_SOURCE_DIR}/doc/man/*.[1-9].rst)
  foreach(m ${man_rst})
    if("x${m}" MATCHES "^x(.+)\\.([1-9])\\.rst$")
      set(name "${CMAKE_MATCH_1}")
      set(sec "${CMAKE_MATCH_2}")
      install(FILES ${CMAKE_CURRENT_BINARY_DIR}/man/${name}.${sec}
              DESTINATION ${MAN_INSTALL_DIR}/man${sec}
              COMPONENT doc)
    endif()
  endforeach()
endif()

if(US_BUILD_DOC_HTML AND NOT US_NO_INSTALL)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
          DESTINATION ${DOC_INSTALL_DIR}
          COMPONENT doc
         )
endif()
