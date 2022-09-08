Intel(R) Software Guard Extensions Data Center Attestation Primitives (Intel(R) SGX DCAP) Rust Quote Verification SampleCode
================================================

## Linux
Supported operating systems:
* Ubuntu* 18.04 LTS Desktop 64bits
* Ubuntu* 18.04 LTS Server 64bits
* Ubuntu* 20.04 LTS Server 64bits
* Red Hat Enterprise Linux Server release 8.2 64bits
* CentOS 8.2 64bits

Requirements:
* make
* gcc
* g++
* bash shell
* clang
* Rust and Cargo

Prerequisite:
* Intel(R) SGX DCAP Driver
* Intel(R) SGX SDK
* Intel(R) SGX DCAP Packages
* Intel(R) SGX DCAP PCCS (Provisioning Certificate Caching Service)

*Please refer to SGX DCAP Linux installation guide "https://download.01.org/intel-sgx/sgx-dcap/#version#/linux/docs/Intel_SGX_DCAP_Linux_SW_Installation_Guide.pdf" to install above dependencies*<br/>
*Note that you need to change **\#version\#** to actual version number in URL, such as 1.4.*<br/>
*Note that you need to install **libsgx-dcap-quote-verify-dev** for this package.*

1. Generate an ECDSA quote with certification data of type 5 using *QuoteGenerationSample*
```
   $ cd ../QuoteGenerationSample/
   $ make
   $ ./app
```

2. Build and run *RustQuoteVerificationSample* to verify a given quote

   Trusted quote verification is processed inside the Intel(R) QvE. In this sample, we borrowed enclave from [QuoteVerificationSample](../QuoteVerificationSample) to perform quote verification.

   Go to *QuoteVerificationSample* and build *enclave.signed.so*. 
   ```
   $ cd ../QuoteVerificationSample/
   $ make SGX_DEBUG=1
   ```

   Go back to *RustQuoteVerificationSample* and build static library for *Enclave_u.o*.
   ```
   $ cd ../RustQuoteVerificationSample/
   $ ar rs libenclave_untrusted.a ../QuoteVerificationSample/App/Enclave_u.o
   ```

   
   Build the and run with default quote path:
   ```
   $ cargo build
   $ ./target/debug/app
   ```

   Or run with specified quote path:

   ```
   $ ./target/debug/app --quote </path/to/quote.dat [default=../QuoteGenerationSample/quote.dat]>
   ```
   You can also combine building and running with a single Cargo command:
   ```
   $ cargo run
   ```
   and to specify quote path:
   ```
   $ cargo run -- --quote </path/to/quote.dat>
   ```
