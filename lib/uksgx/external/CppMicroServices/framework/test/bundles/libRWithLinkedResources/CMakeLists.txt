
set(resource_files
  foo.txt
  manifest.json
)

configure_file(resources/foo.txt ${CMAKE_CURRENT_BINARY_DIR}/resources/foo2.txt COPYONLY)

usFunctionCreateTestBundleWithResources(TestBundleRL
  RESOURCES ${resource_files}
  BINARY_RESOURCES foo2.txt
  LINK_RESOURCES
)

