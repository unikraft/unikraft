# Intel® Integrated Performance Primitives Cryptography (Intel® IPP Cryptography)

This is a list of notable changes to Intel(R) IPP Cryptography, in reverse chronological order.

## YYYY-MM-DD

## 2020-09-01
- Refactoring of Crypto Multi-buffer library, added build and installation of crypto_mb dynamic library and CPU features detection.

## 2020-08-19
- Added multi-buffer implementation of AES-CFB optimized with Intel(R) AES New Instructions (Intel(R) AES-NI) and vector extensions of Intel(R) AES New Instructions (Intel(R) AES-NI) instruction sets.
- Fixed compatibility issue with x64 ABI (restored non-volatile registers after function call in Intel® Advanced Vector Extensions (Intel® AVX)/Intel® Advanced Vector Extensions 2 (Intel® AVX2) assembly code).
- Updated Intel IPP Custom Library Tool.

## 2020-06-09
- AES-GCM algorithm was optimized for Intel(R) Microarchitecture Code Named Cascade Lake with Intel(R) AES New Instructions (Intel(R) AES-NI).
- Crypto Multi-buffer library installation instructions update.

## 2020-04-27
- The Readme file of Crypto Multi-buffer Library was updated by information about possible installation fails in specific environment.

## 2020-04-21
- Documentation of Crypto Multi-buffer Library was updated.
- Position Independent Execution (PIE) option was added to Crypto Multi-buffer Library build scripts.

## 2020-04-19
- AES-XTS optimization for Intel(R) Microarchitecture Code Named Ice Lake with vector extensions of Intel(R) AES New Instructions (Intel(R) AES-NI) was improved.
- Fixed a build issue that affect build of the Intel(R) IPP Cryptography library with MSVC\* compiler on Windows\* OS.
- Duplicated APIs of HASH, HMAC, MGF, RSA, ECCP functionality were marked as deprecated. For more information see [Deprecation notes](./DEPRECATION_NOTES.md)
- Added examples demonstrating usage of SMS4-CBC encryption and decryption.

## 2020-02-25
- SM4-ECB, SM4-CBC and SM4-CTR were enabled for Intel(R) Microarchitecture Code Named Ice Lake with Intel(R) Advanced Vector Extensions 512 (Intel(R) AVX-512) GFNI instructions.
- Added support of Clang 9.0 for Linux and Clang 11.0 for MacOS compilers.
- Added example of RSA Multi-Buffer Encryption/Decryption usage.
- The library was enabled with  Intel® Control-Flow Enforcement Technology (Intel® CET) on Linux and Windows.
- Changed API of ippsGFpECSignDSA, ippsGFpECSignNR and ippsGFpECSignSM2 functions: const-ness requirement of private ephemeral keys is removed and now the ephemeral keys are cleaned up after signing.

## 2019-12-13
- Removed Android support. Use Linux libraries instead.
- Added RSA PSS multi buffer signature generation and verification.
- Added RSA PKCS#1 v1.5 multi buffer signature generation and verification.
- Added RSA IFMA Muti-buffer Library.
- Fixed all build warnings for supported GCC\* and MSVC\* compilers.
- Assembler sources were migrated to NASM\* assembler.

## 2019-09-17
- Added RSA multi buffer encryption and decryption.

## 2019-08-27
- Added Intel(R) IPP Cryptography library examples: AES-CTR, RSA-OAEP, RSA-PSS.
- Fixed code generation for kernel code model in Linux* Intel(R) 64 non-PIC builds.
- Fixes in Intel IPP Custom Library Tool.

## 2019-07-23
- Added Microsoft\* Visual Studio\* 2019 build support.
- Added Intel IPP Custom Library Tool.

## 2019-06-24
- AES-GCM was enabled with vector extensions of Intel(R) AES New Instructions (Intel(R) AES-NI).
- A dynamic dispatcher library and a set of CPU-optimized dynamic libraries were replaced by a single merged dynamic library with an internal dispatcher.
- Removed deprecated multi-threaded version of the library.
- Removed Supplemental Streaming SIMD Extensions 3 (SSSE3) support on macOS.

## 2019-05-29
- AES-XTS was enabled with vector extensions of Intel(R) AES New Instructions (Intel(R) AES-NI).

## 2019-05-22
- Fixed GCC\* and MSVC\* builds of IA32 generic CPU code (pure C) with -DMERGED_BLD:BOOL=off option.
- Added single-CPU headers generation.
- Aligned structure of the build output directory across all supported operation systems.

## 2019-04-23
- AES-CFB was enabled with vector extensions of Intel(R) AES New Instructions (Intel(R) AES-NI).
- ippsGFpECGetPointOctString and ippsGFpECSetPointOctString now support elliptic curves over both prime and extended finite fields.

## 2019-04-01
- 1024, 2048, 3072 and 4096 bit RSA were enabled with AVX512 IFMA instructions.
- AES-ECB, AES-CBC and AES-CTR were enabled with vector extensions of Intel(R) AES New Instructions (Intel(R) AES-NI).
- Improved optimization of Intel(R) AES-NI based CMAC.
- Added the ippsGFpGetInfo function, which returns information about a finite field.
- Added the ippsHashGetInfo_rmf function, which returns information about a hash algorithm.
- Added the ability to build the Intel(R) IPP Cryptography library with GCC\* 8.2.
- Fixed selection of CPU-specific code in dynamic/shared libraries.

## 2018-10-15
- Added the new SM2 encryption scheme.
- Added the ability to build the Intel(R) IPP Cryptography library with the Microsoft\* Visual C++ Compiler 2017.
- Added the ability to build the Intel(R) IPP Cryptography library with the Intel(R) C++ Compiler 19.
- Changed the range of the message being signed or verified by EC and DLP.
- Fixed a potential security problem in the DLP signing and key generation functions.
- Fixed a potential security problem in the AES-CTR cipher functions.
- Fixed a potential security problem in the AES-GCM cipher functions.

## 2018-09-07
- Deprecated the ARCFour functionality.
- Fixed a potential security problem in the signing functions over elliptic curves.
- Fixed a potential security problem in the key expansion function for AES Encryption.
- Fixed some of the compilation warnings observed when building the static dispatcher on Windows\* OS.
- Fixed minor issues with DLP functions.


------------------------------------------------------------------------
Intel is a trademark of Intel Corporation or its subsidiaries in the U.S. and/or other countries.
\* Other names and brands may be claimed as the property of others.
