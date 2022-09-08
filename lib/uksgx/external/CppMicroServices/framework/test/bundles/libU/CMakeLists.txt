
usFunctionCreateTestBundleWithResources(TestBundleU SOURCES TestBundleU.cpp RESOURCES manifest.json LINK_RESOURCES)

# Codepage for the prompt needs to be changed for command-line generator
if(WIN32 AND CMAKE_GENERATOR MATCHES "(.*)Makefiles(.*)")
  # find the current codepage of DOS Prompt
  execute_process(COMMAND CMD /c chcp OUTPUT_VARIABLE CHCP_OUTPUT)
  separate_arguments(CHCP_OUTPUT)
  list(GET CHCP_OUTPUT -1 OLD_CODEPAGE)

  # set the current codepage for the DOS prompt to support utf-8
  add_custom_command(TARGET TestBundleU
                     PRE_BUILD
                     COMMAND chcp 65001
                     COMMENT "Change console codepage to 65001 to accomodate utf-8")
  
  # reset the codepage for the DOS prompt back to what it was
  add_custom_command(TARGET TestBundleU
                     POST_BUILD
                     COMMAND chcp ${OLD_CODEPAGE}
                     COMMENT "Restore console codepage to ${OLD_CODEPAGE}")
endif()

if(NOT MINGW) # skipping MINGW because unicode characters in file path causes MINGW linker failure 
  # override the library and runtime output directories for the target to include unicode characters in path 
  set(unicode_dir_name くいりのまちとこしくそ) 
  if(DEFINED CMAKE_CONFIGURATION_TYPES)   # necessary for generators such as Visual Studio and Xcode 
    foreach(_type ${CMAKE_CONFIGURATION_TYPES}) 
      string(TOUPPER ${_type} cur_config) 
      set_target_properties(TestBundleU PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${cur_config} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${_type}/${unicode_dir_name}/ ) 
      set_target_properties(TestBundleU PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${cur_config} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${_type}/${unicode_dir_name}/ ) 
    endforeach() 
  endif() 
  set_target_properties(TestBundleU PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${unicode_dir_name}/ ) 
  set_target_properties(TestBundleU PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${unicode_dir_name}/ ) 
endif()