# Intel® Integrated Performance Primitives Cryptography

[Build Instructions](./BUILD.md) | [Contributing Guide](#how-to-contribute) | [Documentation](#documentation) | [Get Help](#get-help) | [Intel IPP Product Page](https://software.intel.com/en-us/intel-ipp)

Intel® Integrated Performance Primitives (Intel® IPP) Cryptography is a secure, fast and lightweight library of building blocks for cryptography, highly-optimized for various Intel® CPUs.

## Key Features
The library provides a comprehensive set of routines commonly used for cryptographic operations, including:
 - Symmetric Cryptography Primitive Functions:
    - AES (ECB, CBC, CTR, OFB, CFB, XTS, GCM, CCM, SIV) 
    - SM4 (ECB, CBC, CTR, OFB, CFB, CCM)
    - TDES (ECB, CBC, CTR, OFB, CFB)
    - RC4
- One-Way Hash Primitives:
    - SHA-1, SHA-224, SHA-256, SHA-384, SHA-512
    - MD5
    - SM3
- Data Authentication Primitive Functions:
   - HMAC
   - AES-CMAC
- Public Key Cryptography Functions:
   - RSA, RSA-OAEP, RSA-PKCS_v15, RSA-PSS 
   - DLP, DLP-DSA, DLP-DH
   - ECC (NIST curves), ECDSA, ECDH, EC-SM2
- Multi-buffer RSA, ECDSA, SM3, x25519
- Finite Field Arithmetic Functions
- Big Number Integer Arithmetic Functions
- PRNG/TRNG and Prime Numbers Generation

## Reasons to Use Intel IPP Cryptography
- Security (constant-time execution for secret processing functions)
- Designed for the small footprint size
- Optimized for different Intel CPUs and instruction set architectures (including hardware cryptography instructions support):
    - Intel® Streaming SIMD Extensions 2 (Intel® SSE2)
    - Intel® SSE3
    - Intel® SSE4.2
    - Intel® Advanced Vector Extensions (Intel® AVX)
    - Intel® Advanced Vector Extensions 2 (Intel® AVX2)
    - Intel® Advanced Vector Extensions 512 (Intel® AVX-512)
- Configurable CPU dispatching for the best performance
- Kernel mode compatibility
- Thread-safe design

## Installation

[How to Get and Build the Intel IPP Cryptography Library](./BUILD.md)

## Documentation

- [Introduction to Intel IPP Cryptography Library](./OVERVIEW.md)
- [Introduction to Crypto Multi-buffer Library](./sources/ippcp/crypto_mb/Readme.md)
- [Intel IPP Cryptography Build Instructions](./BUILD.md)
- [Intel IPP Release Notes](https://software.intel.com/en-us/articles/intel-ipp-release-notes-and-new-features)
- [Intel IPP Cryptography Developer Reference](https://software.intel.com/en-us/ipp-crypto-reference)
- [Intel IPP Documentation](https://software.intel.com/en-us/intel-ipp/documentation)

## How to Contribute

We welcome community contributions to Intel IPP Cryptography. If you have an idea how to improve the product, let us know about your proposal via the Intel IPP Forum or GitHub* Issues.

### License
Intel IPP Cryptography is licensed under Apache License, Version 2.0. By contributing to the project, you agree to the license and copyright terms therein and release your contribution under these terms.

## Certification

Intel IPP Cryptography library is not certified for FIPS-140-2 (Security Requirements for Cryptographic Modules) and CMVP (Cryptographic Module Validation Program).
