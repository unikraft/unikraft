CXX = g++
CXXFLAGS = -g -Wall -Wno-unused -pedantic -fPIC $(US_CXX_FLAGS)
LDFLAGS = -Wl,-rpath="$(CppMicroServices_ROOT)/lib" -Wl,-rpath=.
LDLIBS = "${US_CPPMICROSERVICES_TARGET}"

INCLUDEDIRS = -I"$(CppMicroServices_ROOT)/include/cppmicroservices4"
LIBDIRS = -L"$(CppMicroServices_ROOT)/lib" -L.

RC = "$(CppMicroServices_ROOT)/bin/usResourceCompiler4"

OBJECTS = bundle.o IDictionaryService.o

all : main libbundle.so

main: libbundle.so main.o
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(INCLUDEDIRS) $(LIBDIRS) $(LDLIBS) -lbundle

libbundle.so: $(OBJECTS) resources.zip
	$(CXX) -shared -o $@ $(OBJECTS) $(CXXFLAGS) $(LDFLAGS) $(INCLUDEDIRS) $(LIBDIRS) $(LDLIBS)
	$(RC) -z resources.zip -b $@ 

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -DUS_BUNDLE_NAME=main $(INCLUDEDIRS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -DUS_BUNDLE_NAME=bundle $(INCLUDEDIRS) -c $< -o $@

resources.zip: resources/manifest.json
	$(RC) -m $< -n bundle -o resources.zip 

.PHONY : clean

clean:
	rm -f *.o
