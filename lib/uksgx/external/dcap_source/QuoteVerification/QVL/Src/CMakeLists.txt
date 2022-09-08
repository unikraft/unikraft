# Copyright (c) 2017, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
# OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
# OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
# EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
cmake_minimum_required(VERSION 3.12)

set(ATTESTATION_PARSERS_ROOT_DIR "") # Optional path to Attestation Parsers library
set(ATTESTATION_LIBRARY_ROOT_DIR "") # Optional path to Attestation library
option(BUILD_ATTESTATION_APP "Build AttestationApp" ON)
option(BUILD_TESTS "Build tests for all components" ON)
option(BUILD_DOCS "Build doxygen based documentation" OFF)
option(BUILD_ENCLAVE "Build test sgx enclave and sample app that uses it" OFF)

######### QVL Enclave related settings #################################################################################

if(BUILD_ENCLAVE)
	message("Enclave build is on. Checking required components...")
	set(SGX_SDK "/opt/intel/sgxsdk" CACHE INTERNAL "SGX SDK location path")
	set(SGX_OPENSSL "/opt/intel/intel-sgx-ssl/Linux/package" CACHE INTERNAL "SGX OpenSLL location path")
	set(SGX_MODE "HW" CACHE INTERNAL "SGX mode: HW or SIM")
	if (NOT EXISTS ${SGX_SDK})
		message("SGX SDK was not found in ${SGX_SDK}")
	endif()
	if(NOT EXISTS ${SGX_OPENSSL})
		message("SGX SSL was not found in ${SGX_OPENSSL}")
	endif()
	if(NOT EXISTS ${SGX_SDK} OR NOT EXISTS ${SGX_OPENSSL})
		message(FATAL_ERROR "SGX SDK and SGX SSL are required for enclave build.")
	endif()
	message("All required components found. SGX mode: ${SGX_MODE}")
	if(${SGX_MODE} STREQUAL "HW")
		set(TRTS_LIBRARY_NAME     sgx_trts)
		set(TSERVICE_LIBRARY_NAME sgx_tservice)
		set(URTS_LIBRARY_NAME	  sgx_urts)
		set(USERVICE_LIBRARY_NAME sgx_uae_service)
	else()
		set(TRTS_LIBRARY_NAME     sgx_trts_sim)
		set(TSERVICE_LIBRARY_NAME sgx_tservice_sim)
		set(URTS_LIBRARY_NAME	  sgx_urts_sim)
		set(USERVICE_LIBRARY_NAME sgx_uae_service_sim)
	endif()
	set(CMAKE_CXX_COMPILER g++)
	set(CMAKE_C_COMPILER gcc)
endif()

########################################################################################################################

# Use AttestationParsers from provided path if build is disabled
if(NOT ${ATTESTATION_PARSERS_ROOT_DIR} STREQUAL "")
	message(STATUS "Using external attestation parsers library at: ${ATTESTATION_PARSERS_ROOT_DIR}")

	add_library(AttestationParsers INTERFACE)
	target_include_directories(AttestationParsers SYSTEM INTERFACE "${ATTESTATION_PARSERS_ROOT_DIR}/include/")
	target_link_libraries(AttestationParsers INTERFACE ${ATTESTATION_PARSERS_ROOT_DIR}/lib/libAttestationParsers.so)

	add_library(AttestationParsersStatic INTERFACE)
	target_include_directories(AttestationParsersStatic SYSTEM INTERFACE "${ATTESTATION_PARSERS_ROOT_DIR}/include/")
	target_link_libraries(AttestationParsersStatic INTERFACE ${ATTESTATION_PARSERS_ROOT_DIR}/lib/libAttestationParsersStatic.a)
endif()

# Use AttestationLibrary from provided path if build is disabled
if(NOT ${ATTESTATION_LIBRARY_ROOT_DIR} STREQUAL "")
	message(STATUS "Using external attestation library at: ${ATTESTATION_LIBRARY_ROOT_DIR}")

	add_library(AttestationLibrary INTERFACE)
	target_include_directories(AttestationLibrary SYSTEM INTERFACE "${ATTESTATION_LIBRARY_ROOT_DIR}/include/")
	target_link_libraries(AttestationLibrary INTERFACE ${ATTESTATION_LIBRARY_ROOT_DIR}/lib/libQuoteVerification.so)

	add_library(AttestationLibraryStatic INTERFACE)
	target_include_directories(AttestationLibraryStatic SYSTEM INTERFACE "${ATTESTATION_LIBRARY_ROOT_DIR}/include/")
	target_link_libraries(AttestationLibraryStatic INTERFACE ${ATTESTATION_LIBRARY_ROOT_DIR}/lib/libQuoteVerificationStatic.a)
endif()

set(QVL_BUILD_DIR "${CMAKE_SOURCE_DIR}/Build/${CMAKE_BUILD_TYPE}")
set(QVL_DIST_DIR "${QVL_BUILD_DIR}/dist")

set(CMAKE_INSTALL_PREFIX ${QVL_DIST_DIR} CACHE PATH "default install path" FORCE )

if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
	if(UNIX)
		if (NOT DEFINED BULLSEYE_PATH)
			set(BULLSEYE_PATH "/opt/bullseye" CACHE PATH "" FORCE) #default Bullseye install path
		endif ()
		set(BULLSEYE_BIN "${BULLSEYE_PATH}/bin" CACHE PATH "bullseye binaries path" FORCE)

		get_filename_component(COMPILER_NAME ${CMAKE_CXX_COMPILER} NAME)

		find_program(BULLSEYE_CC
				NAMES ${COMPILER_NAME}
				PATHS ${BULLSEYE_BIN}
				NO_DEFAULT_PATH)

		find_program(BULLSEYE_COV_ENABLE
				NAMES "cov01"
				PATHS ${BULLSEYE_BIN}
				NO_DEFAULT_PATH)

		find_program(BULLSEYE_COV_SELECT
				NAMES "covselect"
				PATHS ${BULLSEYE_BIN}
				NO_DEFAULT_PATH)

		find_program(BULLSEYE_COV_HTML
				NAMES "covhtml"
				PATHS ${BULLSEYE_BIN}
				NO_DEFAULT_PATH)

		find_program(BULLSEYE_COV_XML
				NAMES "covxml"
				PATHS ${BULLSEYE_BIN}
				NO_DEFAULT_PATH)

		include(FindPackageHandleStandardArgs)
		find_package_handle_standard_args(BULLSEYE DEFAULT_MSG BULLSEYE_CC BULLSEYE_COV_ENABLE BULLSEYE_COV_SELECT BULLSEYE_COV_HTML BULLSEYE_COV_XML)

		set(CMAKE_CXX_COMPILER "${BULLSEYE_CC}" CACHE STRING "" FORCE) #must be done before 'project()'
	else()
		message(FATAL_ERROR "Coverage build is only supported on Linux")
	endif()
endif ()

#required to run CMake on windows
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

#hunter has to be imported before 'project()'
set(HUNTER_STATUS_DEBUG OFF) #set to 'ON' to see debug hunter logs
#set(HUNTER_ROOT ${CMAKE_SOURCE_DIR}/ThirdParty/hunter) # in case of Hunter build problems
#set(HUNTER_BINARY_DIR ${CMAKE_SOURCE_DIR}/ThirdParty/hunter) # in case of Hunter build problems

option(HUNTER_NO_TOOLCHAIN_ID_RECALCULATION "No Toolchain-ID recalculation" ON)
option(HUNTER_KEEP_PACKAGE_SOURCES "Keep third party sources" ON)

include("${CMAKE_CURRENT_LIST_DIR}/ThirdParty/hunter/HunterGate.cmake")
HunterGate(
		URL "https://github.com/cpp-pm/hunter/archive/v0.23.316.tar.gz"
		SHA1 "167a33c4cc398504826a1a3014ef66f9ccc35db3"
		FILEPATH "${CMAKE_SOURCE_DIR}/ThirdParty/hunter/config.cmake"
)

project(SgxEcdsaAttestation)

set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_THREAD_LIBS_INIT "-lpthread") # Could NOT find Threads (missing: Threads_FOUND) related workarounds
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE) # adds -fpic/-fpie/-pie

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.0")
			message(FATAL_ERROR "Clang 5.0 or above required.")
		endif()
		set(IGNORED_WARNINGS
				"-Wno-gnu-statement-expression" #warns about __FILE__, __LINE__, etc. compiler macros
				"-Wno-c++98-compat"             #family of warnings indicating this code may cause errors if compiled with C++98 compiler
				"-Wno-c++98-compat-pedantic"    #same as above
				"-Wno-used-but-marked-unused"   #detects when a variable is marked as unsued (preprocessor macro or other) and warns if it is used despite that [note: triggers warnings in UT]
				"-Wno-zero-as-null-pointer-constant" #zero as null pointer constant, mainly for openssl which widely uses 0 for null
				"-Wno-undefined-var-template"     #visible after upgrading to C++17, does not seem to be a big problem but, TODO investigate on how a proper fix should look like
				"-Wno-undefined-func-template"    #visible after upgrading to C++17, does not seem to be a big problem but, TODO investigate on how a proper fix should look like
				)
		set(CMAKE_CXX_FLAGS_CLANG
				"-fcolor-diagnostics"           #clang coloured output
				"-fwrapv"                       #Treat signed integer overflow as two’s complement
				"-Wno-unused-command-line-argument"
				)
        # implicit-conversion: Checks for suspicious behavior of implicit conversions. Enables implicit-unsigned-integer-truncation, implicit-signed-integer-truncation, and implicit-integer-sign-change.
		# nullability: Enables nullability-arg, nullability-assign, and nullability-return. While violating nullability does not have undefined behavior, it is often unintentional, so UBSan offers to catch it.
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE_CLANG "-fsanitize=undefined,safe-stack,implicit-conversion,nullability")
	endif()
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.0")
			message(FATAL_ERROR "GCC 5.0 or above required.")
		endif()
        set(CMAKE_CXX_FLAGS_GCC
                "-fno-strict-overflow"            # tells the compiler NOT to assume that signed overflow does not occur.
                "-fno-delete-null-pointer-checks" # tells the compiler NOT to assume that null pointer deference does not exist.
                "-fwrapv"                         # tells the compiler that # signed overflow always wraps.
                )
	endif()
	set(CMAKE_C_FLAGS
			"-fpic"                         #Position Independent Execution (PIE)
			"-fstack-protector-strong"      #stack-based buffer overrun detection
			"-Werror"                       #make all warnings into errors
			"-Wall"                         #enable all warnings
			"-Wextra"
			"-pthread"
			"-ffunction-sections"
			"-fdata-sections"
			"-Winit-self"
			"-Wpointer-arith"
			"-Wreturn-type"
			"-Waddress"
			"-Wsequence-point"
			"-Wformat-security"
			"-Wno-attributes"
			"-Wmissing-include-dirs"
			"-Wconversion"
			"-Wfloat-equal"
			"-Wundef"
			"-Wshadow"
			"-Wcast-align"
			"-Wcast-qual"
			"-Wredundant-decls"
			)
	if(NOT DEFINED SANITIZE_FLAGS)
		set(SANITIZE_FLAGS
				"-Wl,-z,relro" "-Wl,-z,now" #data relocation and protection (RELRO)
				"-Wl,-z,noexecstack"       #stack execution protection
				"-Wl,--no-undefined")
	endif()
	set(CMAKE_CXX_FLAGS
			"${CMAKE_C_FLAGS}"
			"-std=c++14"                    #currently used C++ standard
			"-Wnon-virtual-dtor"
			"${CMAKE_CXX_FLAGS_CLANG}"
			"${CMAKE_CXX_FLAGS_GCC}"
			"${IGNORED_WARNINGS}"           #but ignore some unimportant ones
			"${SANITIZE_FLAGS}"
			)
	set(CMAKE_CXX_FLAGS_RELEASE "-O2 -D_FORTIFY_SOURCE=2 -fvisibility=hidden")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -DDEBUG -UNDEBUG -UEDEBUG")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

	set(COMMON_LINKER_FLAGS
			"-ldl" "-lrt" "-lpthread"
			"${SANITIZE_FLAGS}"
			)

	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${COMMON_LINKER_FLAGS}" "-pie" "${CMAKE_EXE_LINKER_FLAGS_RELEASE_CLANG}")
	set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${COMMON_LINKER_FLAGS}")

	string(REPLACE ";" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS}") #Convert colon-separated CMake lists back to space-separated values that clang likes
	string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}") #Convert colon-separated CMake lists back to space-separated values that clang likes
	string(REPLACE ";" " " CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}") #Convert colon-separated CMake lists back to space-separated values that clang likes
	string(REPLACE ";" " " CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}") #Convert colon-separated CMake lists back to space-separated values that clang likes

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	set(CMAKE_CXX_FLAGS
			"/analyze"
			"/DYNAMICBASE"
			"/EHsc" # catch C++ exceptions, assume extern "C" functions never throw C++ exceptions
			"/MP" # Multi processor compilation
			"/guard:cf"
			"/INCREMENTAL"
			"/sdl"  # superset of the baseline security checks
			"${SANITIZE_FLAGS}"
			)

	set(COMMON_LINKER_FLAGS
			"/NXCOMPAT"
			"/FORCE:MULTIPLE"
			)
	if(NOT "${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
		set(COMMON_LINKER_FLAGS "${COMMON_LINKER_FLAGS}" "/SAFESEH") # valid for x86 build only
	endif()

	set(CMAKE_EXE_LINKER_FLAGS "${COMMON_LINKER_FLAGS}")
	set(CMAKE_SHARED_LINKER_FLAGS "${COMMON_LINKER_FLAGS}")
	set(CMAKE_STATIC_LINKER_FLAGS "${COMMON_LINKER_FLAGS}")
	set(CMAKE_MODULE_LINKER_FLAGS "${COMMON_LINKER_FLAGS}")

	string(REPLACE ";" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS}") #Convert colon-separated CMake lists back to space-separated values that clang likes
	string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}") #Convert colon-separated CMake lists back to space-separated values that clang likes
	string(REPLACE ";" " " CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}") #Convert colon-separated CMake lists back to space-separated values that clang likes
	string(REPLACE ";" " " CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}") #Convert colon-separated CMake lists back to space-separated values that clang likes
	string(REPLACE ";" " " CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS}") #Convert colon-separated CMake lists back to space-separated values that clang likes
	string(REPLACE ";" " " CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS}") #Convert colon-separated CMake lists back to space-separated values that clang likes

	cmake_minimum_required(VERSION 3.12)
	add_compile_definitions(_WINDOWS)
else()
	message(FATAL_ERROR "Unsupported compiler '${CMAKE_CXX_COMPILER_ID}'. Clang 5.0, GCC and MSVC are supported")
endif()

if(NOT BUILD_ENCLAVE)
	# Settings for coverage support in CLion (version 2019.3 and up)
	if("${CMAKE_C_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang"
			OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang")
		message("Building with llvm Code Coverage Tools")
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-instr-generate -fcoverage-mapping")
	elseif(CMAKE_COMPILER_IS_GNUCXX)
		message("Building with lcov Code Coverage Tools")
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage")
	endif()
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${QVL_BUILD_DIR}/out/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${QVL_BUILD_DIR}/out/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${QVL_BUILD_DIR}/out/bin)

# Order matters!
add_subdirectory(ThirdParty)
add_subdirectory(AttestationCommons)
if("${ATTESTATION_PARSERS_ROOT_DIR}" STREQUAL "")
	add_subdirectory(AttestationParsers)
endif()
if("${ATTESTATION_LIBRARY_ROOT_DIR}" STREQUAL "")
	add_subdirectory(AttestationLibrary)
endif()
if(BUILD_ATTESTATION_APP)
	add_subdirectory(AttestationApp)
endif()

add_custom_target(install_${PROJECT_NAME}
		$(MAKE) install
		DEPENDS AttestationApp
		COMMENT "Installing ${PROJECT_NAME}")

if (BUILD_TESTS)
	if(UNIX)
		add_custom_target(runTests
				COMMAND cd ${QVL_DIST_DIR}/bin &&
				LD_LIBRARY_PATH=../lib ./AttestationCommons_UT --gtest_output="xml:../results/AttestationCommons_UTResults.xml" &&
				LD_LIBRARY_PATH=../lib ./AttestationParsers_UT --gtest_output="xml:../results/AttestationParsers_UTResults.xml" &&
				LD_LIBRARY_PATH=../lib ./AttestationParsers_IT --gtest_output="xml:../results/AttestationParsers_ITResults.xml" &&
				LD_LIBRARY_PATH=../lib ./AttestationLibrary_UT --gtest_output="xml:../results/AttestationLibrary_UTesults.xml" &&
				LD_LIBRARY_PATH=../lib ./AttestationLibrary_IT --gtest_output="xml:../results/AttestationLibrary_ITResults.xml" &&
				LD_LIBRARY_PATH=../lib ./AttestationApp_UT --gtest_output="xml:../results/AttestationApp_UTResults.xml" &&
				cd ${CMAKE_SOURCE_DIR}
				DEPENDS install_${PROJECT_NAME})
	endif()

	if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
		if(UNIX)
            include(ProcessorCount)
            ProcessorCount(N)
			set(COVFILE "${CMAKE_SOURCE_DIR}/sgx.cov")

			add_custom_target(cov_on
					COMMAND ${BULLSEYE_COV_SELECT} -d --quiet
					COMMAND ${BULLSEYE_COV_SELECT} -i ${PROJECT_SOURCE_DIR}/BullseyeCoverageExclusions --no-banner
					COMMAND ${BULLSEYE_COV_ENABLE} -1  --no-banner
					)

			add_custom_target(cov_off
					COMMAND ${BULLSEYE_COV_ENABLE} -0 --no-banner)

			add_custom_target(cov_html
					COMMAND ${BULLSEYE_COV_HTML} -d${PROJECT_SOURCE_DIR} -f${COVFILE} ${PROJECT_SOURCE_DIR}/code-coverage-report  --no-banner)

			add_custom_target(cov_xml
					COMMAND ${BULLSEYE_COV_XML} -f${COVFILE} -o${PROJECT_SOURCE_DIR}/code-coverage-report/coverage.xml  --no-banner)

			add_custom_target(code-coverage
					COMMAND COVFILE=${COVFILE} $(MAKE) -j${N} -C ${QVL_BUILD_DIR} cov_on runTests cov_off cov_html cov_xml)
		else()
			message(FATAL_ERROR "Coverage build is only supported on Linux")
		endif()
	endif ()
endif ()
