
set(resource_files
  icons/compressable.bmp
  icons/cppmicroservices.png
  icons/readme.txt
  foo.txt
  special_chars.dummy.txt
  test.xml
  manifest.json
)

configure_file(resources/foo.txt ${CMAKE_CURRENT_BINARY_DIR}/resources/foo2.txt COPYONLY)

usFunctionCreateTestBundleWithResources(TestBundleR
  RESOURCES ${resource_files}
  BINARY_RESOURCES foo2.txt
)
