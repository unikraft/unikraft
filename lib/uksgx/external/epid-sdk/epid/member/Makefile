#!/usr/bin/make -f

#define path variables
IPP_SRC_INCLUDE_DIR = ../../ext/ipp/sources/ippcp/src/
IPP_INCLUDE_DIR = ../../ext/ipp/sources/include/
GTEST_INCLUDE_DIR = ../../ext/

LIB_INSTALL_DIR = $(epidinstalldir)/lib/posix-$(ARCH)/
TEST_INSTALL_DIR = $(epidinstalldir)/test/
HEADERS_INSTALL_DIR :=  $(epidinstalldir)/include/epid/member/

API_HEADERS_INSTALL = $(wildcard ./*.h)

MEMBER_INCLUDE_DIR = ../../
TINY_MEMBER_INCLUDE_DIR = ./tiny/

MEMBER_SRC =  $(wildcard ./src/*.c)
TINY_MEMBER_SRC =  $(wildcard ./tiny/src/*.c)
TINY_MEMBER_MATH_SRC =  $(wildcard ./tiny/math/src/*.c)
TINY_STDLIB_MEMBER_SRC =  $(wildcard ./tiny/stdlib/src/*.c)
MEMBER_TPM2_TSS_UTEST_SRC = $(wildcard ./tpm2/unittests/*.cc)
ifneq ($(TSS_PATH),)
	MEMBER_TPM2_SRC = $(wildcard ./tpm2/ibm_tss/*.c)
	MEMBER_TPM2_UTEST_SRC = $(filter-out $(wildcard ./tpm2/unittests/*-simulator-test.cc), \
		$(MEMBER_TPM2_TSS_UTEST_SRC))
	MEMBER_INCLUDE_DIR += -I$(TSS_PATH)
	CFLAGS += -DTPM_TSS
	CXXFLAGS += -DTPM_TSS
else
	MEMBER_TPM2_SRC = $(wildcard ./tpm2/src/*.c)
	MEMBER_TPM2_UTEST_SRC = $(filter-out $(wildcard ./tpm2/unittests/*-tss-test.cc), \
		$(MEMBER_TPM2_TSS_UTEST_SRC))
endif
MEMBER_SRC += $(MEMBER_TPM2_SRC)
MEMBER_LIB = ./libmember.a
MATH_LIB = ./libmath.a
STDLIB_LIB = ./libtiny_stdlib.a

MEMBER_UTEST_EXE = ./member-utest$(EXE_EXTENSION)
MEMBER_UTEST_SRC = $(wildcard ./unittests/*.cc) \
	./tpm2/unittests/tpm2_wrapper-testhelper.cc
TINY_MEMBER_UTEST_SRC = $(wildcard ./tiny/unittests/*.cc)
TINY_MEMBER_MATH_UTEST_SRC = $(wildcard ./tiny/math/unittests/*.cc)
ifneq ($(TINY),)
	MEMBER_OBJ = $(sort $(TINY_MEMBER_SRC:.c=.o))
	MATH_OBJ = $(sort $(TINY_MEMBER_MATH_SRC:.c=.o))
	STDLIB_OBJ = $(sort $(TINY_STDLIB_MEMBER_SRC:.c=.o))
	MEMBER_UTEST_OBJ = $(sort $(TINY_MEMBER_UTEST_SRC:.cc=.o))
	TINY_MEMBER_MATH_UTEST_OBJ = $(sort $(TINY_MEMBER_MATH_UTEST_SRC:.cc=.o))
	MEMBER_INCLUDE_DIR += -I$(TINY_MEMBER_INCLUDE_DIR)
else
	MEMBER_OBJ = $(sort $(MEMBER_SRC:.c=.o))
	MEMBER_UTEST_OBJ = $(sort $(MEMBER_UTEST_SRC:.cc=.o))
	MEMBER_TPM2_UTEST_OBJ = $(sort $(MEMBER_TPM2_UTEST_SRC:.cc=.o))
endif

LIB_MEMBER_DIR = .
LIB_VERIFIER_DIR = ../verifier
LIB_COMMON_TESTHELPER_DIR = ../common-testhelper
LIB_COMMON_DIR = ../common
LIB_IPPCP_DIR = ../../ext/ipp/sources/ippcp/src

#set flags for linker
LDFLAGS += -L$(GTEST_INCLUDE_DIR)/gtest  -L$(LIB_COMMON_TESTHELPER_DIR)  \
	-L$(LIB_MEMBER_DIR) -L$(LIB_VERIFIER_DIR) -L$(LIB_COMMON_DIR) \
	-lgtest -lcommon-testhelper \
	-lmember -lverifier -lcommon -lippcp

ifneq ($(TSS_PATH),)
	LDFLAGS += -L$(TSS_PATH) -ltss -lcrypto
endif

ifneq ($(TINY),)
	CPPDEFINES := -DNDEBUG
	LDFLAGS +=  -lmath -ltiny_stdlib
	CFLAGS := -Os -fno-builtin -fno-stack-protector -fomit-frame-pointer -fno-asynchronous-unwind-tables \
	-fdata-sections -ffunction-sections -fvisibility=hidden -Werror -Wall -Wextra
	ifneq ($(OS),Windows_NT)
		CFLAGS += -fPIC
	endif
	ifeq ($(findstring gcc,$(CC)),gcc)
		CFLAGS += -Wno-missing-braces
	endif
	ifeq ($(ARCH), x86)
		CFLAGS += -m32
	endif
endif

$(MEMBER_OBJ): %.o: %.c
	$(CC) $(CFLAGS) \
	-I$(MEMBER_INCLUDE_DIR) -c $^ -o $@

$(MEMBER_LIB): $(MEMBER_OBJ)
	$(AR) rc $(MEMBER_LIB) $(MEMBER_OBJ)
	$(RANLIB) $(MEMBER_LIB)

$(MEMBER_UTEST_EXE): $(MEMBER_UTEST_OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(MEMBER_UTEST_OBJ): %.o: %.cc
	$(CXX) -o $@ $(CXXFLAGS) $(GTEST_DEFINES) -I$(MEMBER_INCLUDE_DIR) \
	-I$(GTEST_INCLUDE_DIR) -c $^

ifneq ($(TINY),)
    $(MATH_OBJ): %.o: %.c
		$(CC) $(CFLAGS) -I$(MEMBER_INCLUDE_DIR) -c $^ -o $@

    $(STDLIB_OBJ): %.o: %.c
		$(CC) $(CFLAGS) -I$(MEMBER_INCLUDE_DIR) -c $^ -o $@

    $(MATH_LIB): $(MATH_OBJ)
		$(AR) rc $(MATH_LIB) $(MATH_OBJ)
		$(RANLIB) $(MATH_LIB)

    $(STDLIB_LIB): $(STDLIB_OBJ)
		$(AR) rc $(STDLIB_LIB) $(STDLIB_OBJ)
		$(RANLIB) $(STDLIB_LIB)

    TINY_MEMBER_MATH_UTEST_EXE = ./member.math-utest$(EXE_EXTENSION)
    $(TINY_MEMBER_MATH_UTEST_EXE): $(TINY_MEMBER_MATH_UTEST_OBJ)
		$(CXX) -o $@ $^ $(LDFLAGS)

    $(TINY_MEMBER_MATH_UTEST_OBJ): %.o: %.cc
		$(CXX) -o $@ $(CXXFLAGS) $(GTEST_DEFINES) -I$(MEMBER_INCLUDE_DIR) \
		-I$(GTEST_INCLUDE_DIR) -c $^
else
    MEMBER_TPM2_UTEST_EXE = ./member.tpm2-utest$(EXE_EXTENSION)
    $(MEMBER_TPM2_UTEST_EXE): $(MEMBER_TPM2_UTEST_OBJ)
		$(CXX) -o $@ $^ $(LDFLAGS)

    $(MEMBER_TPM2_UTEST_OBJ): %.o: %.cc
		$(CXX) -o $@ $(CXXFLAGS) $(GTEST_DEFINES) -I$(MEMBER_INCLUDE_DIR) \
		-I$(GTEST_INCLUDE_DIR) -c $^
endif

build: all
LIBS = $(MEMBER_LIB)
ifneq ($(TINY),)
	LIBS += $(STDLIB_LIB) $(MATH_LIB)
endif
all: $(LIBS)
install:
	mkdir -p '$(LIB_INSTALL_DIR)'
	mkdir -p '$(HEADERS_INSTALL_DIR)'
	cp $(MEMBER_LIB) '$(LIB_INSTALL_DIR)'
ifneq ($(TINY),)
	cp $(MATH_LIB) '$(LIB_INSTALL_DIR)'
	cp $(STDLIB_LIB) '$(LIB_INSTALL_DIR)'
endif
	cp $(API_HEADERS_INSTALL) '$(HEADERS_INSTALL_DIR)'
#install tests if they exist
ifneq (,$(wildcard $(MEMBER_UTEST_EXE)))
	mkdir -p '$(TEST_INSTALL_DIR)'
	cp $(MEMBER_UTEST_EXE) '$(TEST_INSTALL_DIR)'
endif
ifneq ($(TINY),)
	mkdir -p '$(TEST_INSTALL_DIR)'
	cp $(TINY_MEMBER_MATH_UTEST_EXE) '$(TEST_INSTALL_DIR)'
else
	mkdir -p '$(TEST_INSTALL_DIR)'
	cp $(MEMBER_TPM2_UTEST_EXE) '$(TEST_INSTALL_DIR)'
endif

utest: $(MEMBER_UTEST_EXE) $(MEMBER_TPM2_UTEST_EXE) $(TINY_MEMBER_MATH_UTEST_EXE)

run_utest:
	$(MEMBER_UTEST_EXE) $(GTEST_FLAGS)
	$(MEMBER_TPM2_UTEST_EXE) $(GTEST_FLAGS)
	$(TINY_MEMBER_MATH_UTEST_EXE) $(GTEST_FLAGS)

check: utest run_utest

clean:
	rm -f $(MEMBER_OBJ) \
		$(MEMBER_LIB) \
		$(MATH_OBJ) \
		$(MATH_LIB) \
		$(STDLIB_OBJ) \
		$(STDLIB_LIB) \
		$(MEMBER_UTEST_OBJ) \
		$(MEMBER_UTEST_EXE) \
		$(MEMBER_TPM2_UTEST_OBJ) \
		$(MEMBER_TPM2_UTEST_EXE) \
		$(TINY_MEMBER_MATH_UTEST_OBJ) \
		$(TINY_MEMBER_MATH_UTEST_EXE) \
		*.xml
