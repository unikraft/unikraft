set(snippet_src_files
  main.cpp
)

set(base_dir uServices-staticbundles)
add_library(MyStaticBundle STATIC ${base_dir}/MyStaticBundle.cpp)
set_property(TARGET MyStaticBundle APPEND PROPERTY COMPILE_DEFINITIONS US_BUNDLE_NAME=MyStaticBundle)
set_property(TARGET MyStaticBundle PROPERTY US_BUNDLE_NAME MyStaticBundle)

target_link_libraries(MyStaticBundle PRIVATE CppMicroServices)

set(snippet_link_libraries
  MyStaticBundle
)
