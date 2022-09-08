# The library name for the bundle
set(_lib_name dictionaryservice)

# A list of source code files
set(_srcs
  Activator.cpp
  IDictionaryService.cpp
)

# Add a special source file to the _srcs variable that
# will enable dependency checking on bundle resources.
usFunctionGetResourceSource(TARGET Tutorial-${_lib_name} OUT _srcs)

# Generate bundle initialization code
usFunctionGenerateBundleInit(TARGET Tutorial-${_lib_name} OUT _srcs)

# Create the library
add_library(Tutorial-${_lib_name} ${_srcs})

# Add the US_BUNDLE_NAME target property
set_property(TARGET Tutorial-${_lib_name} APPEND PROPERTY US_BUNDLE_NAME ${_lib_name})

# Add the required compile definition
set_property(TARGET Tutorial-${_lib_name} APPEND PROPERTY COMPILE_DEFINITIONS US_BUNDLE_NAME=${_lib_name})

# Embed the manifest file
usFunctionEmbedResources(TARGET Tutorial-${_lib_name} WORKING_DIRECTORY resources FILES manifest.json)

# Link the CppMicroServices library
target_link_libraries(Tutorial-${_lib_name} ${CppMicroServices_LIBRARIES})

# [doc-end]

set_target_properties(Tutorial-${_lib_name} PROPERTIES
  LABELS Tutorial
  OUTPUT_NAME ${_lib_name}
  DEBUG_POSTFIX ""
)

# The above CMake code achieves the same as the CreateTutorial macro,
# but it is decorated with comments and will show up verbatim in the
# documentation.

#CreateTutorial(dictionaryservice ${_srcs})
