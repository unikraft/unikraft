function(usFunctionCompileSnippets snippet_path)

  # get all files called "main.cpp"
  file(GLOB_RECURSE main_cpp_list "${snippet_path}/main.cpp")

  foreach(main_cpp_file ${main_cpp_list})
    # get the directory containing the main.cpp file
    get_filename_component(main_cpp_dir "${main_cpp_file}" PATH)

    set(snippet_src_files )

    # If there exists a "files.cmake" file in the snippet directory,
    # include it and assume it sets the variable "snippet_src_files"
    # to a list of source files for the snippet.
    if(EXISTS "${main_cpp_dir}/files.cmake")
      include("${main_cpp_dir}/files.cmake")
      set(_tmp_src_files ${snippet_src_files})
      set(snippet_src_files )
      foreach(_src_file ${_tmp_src_files})
        if(IS_ABSOLUTE ${_src_file})
          list(APPEND snippet_src_files ${_src_file})
        else()
          list(APPEND snippet_src_files ${main_cpp_dir}/${_src_file})
        endif()
      endforeach()
    else()
      # glob all files in the directory and add them to the snippet src list
      file(GLOB_RECURSE snippet_src_files "${main_cpp_dir}/*")
    endif()

    # Uset the top-level directory name as the executable name
    string(REPLACE "/" ";" main_cpp_dir_tokens "${main_cpp_dir}")
    list(GET main_cpp_dir_tokens -1 snippet_exec_name)
    set(snippet_target_name "Snippet-${snippet_exec_name}")

    add_executable(${snippet_target_name} ${snippet_src_files})
    target_link_libraries(${snippet_target_name} ${PROJECT_TARGET} ${snippet_link_libraries} ${US_LIBRARIES})
    set_property(TARGET ${snippet_target_name} APPEND PROPERTY COMPILE_DEFINITIONS US_BUNDLE_NAME=main)
    set_property(TARGET ${snippet_target_name} PROPERTY US_BUNDLE_NAME main)
    set_target_properties(${snippet_target_name} PROPERTIES
      LABELS Documentation
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/snippets"
      ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/snippets"
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/snippets"
      OUTPUT_NAME ${snippet_exec_name}
    )

  endforeach()

endfunction()
