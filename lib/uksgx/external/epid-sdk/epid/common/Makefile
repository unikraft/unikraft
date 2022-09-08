#!/usr/bin/make -f

#define path variables
IPP_INCLUDE_DIR = ../../ext/ipp/sources/ippcp/src
GTEST_INCLUDE_DIR = ../../ext

LIB_INSTALL_DIR = $(epidinstalldir)/lib/posix-$(ARCH)/
TEST_INSTALL_DIR = $(epidinstalldir)/test/
HEADERS_INSTALL_DIR :=  $(epidinstalldir)/include/epid/common/
HEADERS_11_INSTALL_DIR :=  $(epidinstalldir)/include/epid/common/1.1

API_HEADERS_INSTALL = $(wildcard ./*.h)

API_11_HEADERS_INSTALL = $(wildcard ./1.1/*.h)

COMMON_INCLUDE_DIR = ../../
COMMON_MATH_INCLUDE_DIR = ./math
COMMON_SRC =  $(wildcard ./src/*.c)
COMMON_SRC +=  $(wildcard ./math/src/*.c)
COMMON_SRC +=  $(wildcard ./1.1/src/*.c)
COMMON_OBJ = $(sort $(COMMON_SRC:.c=.o))
COMMON_LIB = ./libcommon.a

COMMON_UTEST_EXE = ./common-utest$(EXE_EXTENSION)
COMMON_UTEST_SRC = $(wildcard ./math/unittests/*.cc)
COMMON_UTEST_SRC += $(wildcard ./unittests/*.cc)
COMMON_UTEST_SRC += $(wildcard ./1.1/unittests/*.cc)
COMMON_UTEST_OBJ = $(sort $(COMMON_UTEST_SRC:.cc=.o))
LIB_COMMON_DIR = .
LIB_COMMON_TESTHELPER_DIR = ../common-testhelper
LIB_IPPCP_DIR = ../../ext/ipp/sources/ippcp/src

#set flags for linker
LDFLAGS += -L$(GTEST_INCLUDE_DIR)/gtest  -L$(LIB_COMMON_TESTHELPER_DIR)  \
	-L$(LIB_COMMON_DIR) \
	-lgtest -lcommon-testhelper -lcommon -lippcp

#target part
$(COMMON_OBJ): %.o: %.c
	$(CC) $(CFLAGS) -I$(COMMON_MATH_INCLUDE_DIR) -I$(COMMON_INCLUDE_DIR) -c $^ -o $@

$(COMMON_LIB): $(COMMON_OBJ)
	$(AR) rc $(COMMON_LIB) $(COMMON_OBJ)
	$(RANLIB) $(COMMON_LIB)

$(COMMON_UTEST_EXE): $(COMMON_UTEST_OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(COMMON_UTEST_OBJ): %.o: %.cc
	$(CXX) -o $@ $(CXXFLAGS) $(GTEST_DEFINES) -I$(COMMON_INCLUDE_DIR) -I$(GTEST_INCLUDE_DIR) -c $^

build: all

all: $(COMMON_LIB)

install:
	mkdir -p '$(LIB_INSTALL_DIR)'
	mkdir -p '$(HEADERS_INSTALL_DIR)'
	mkdir -p '$(HEADERS_11_INSTALL_DIR)'
	cp $(COMMON_LIB) '$(LIB_INSTALL_DIR)'
	cp $(API_HEADERS_INSTALL) '$(HEADERS_INSTALL_DIR)'
	cp $(API_11_HEADERS_INSTALL) '$(HEADERS_11_INSTALL_DIR)'
#install tests if they exist
ifneq (,$(wildcard $(COMMON_UTEST_EXE)))
	mkdir -p '$(TEST_INSTALL_DIR)'
	cp $(COMMON_UTEST_EXE) '$(TEST_INSTALL_DIR)'
endif

utest:  $(COMMON_UTEST_EXE)

run_utest:
	$(COMMON_UTEST_EXE) $(GTEST_FLAGS)

check:  utest run_utest

clean:
	rm -f  $(COMMON_OBJ) \
		$(COMMON_LIB) \
		$(COMMON_UTEST_OBJ) \
		$(COMMON_UTEST_EXE) \
		*.xml
