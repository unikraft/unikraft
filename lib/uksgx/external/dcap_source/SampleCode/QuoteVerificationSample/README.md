Intel(R) Software Guard Extensions Data Center Attestation Primitives (Intel(R) SGX DCAP) Quote Verification SampleCode
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

Prerequisite:
* Intel(R) SGX DCAP Driver
* Intel(R) SGX SDK
* Intel(R) SGX DCAP Packages
* Intel(R) SGX DCAP PCCS (Provisioning Certificate Caching Service)

*Please refer to SGX DCAP Linux installation guide "https://download.01.org/intel-sgx/sgx-dcap/#version#/linux/docs/Intel_SGX_DCAP_Linux_SW_Installation_Guide.pdf" to install above dependencies*<br/>
*Note that you need to change **\#version\#** to actual version number in URL, such as 1.4.*


1. Generate an ECDSA quote with certification data of type 5 using QuoteGenerationSample
```
   $ cd SampleCode/QuoteGenerationSample/
   $ make
   $ ./app
```

2. Build and run QuoteVerificationSample to verify a given quote
```
   Release build:
   $ make  `#You need to sign ISV enclave with your own key in this mode`
   Or Debug build:
   $ make SGX_DEBUG=1
   $ ./app -quote </path/to/quote.dat [default=../QuoteGenerationSample/quote.dat]>
```
**Note**: Our libdcap_quoteprov.so is not built with Intel(R) Control Flow Enforcement Technology(CET) feature. If the sample is built with CET feature(it can be enabled by the compiler's default setting) and it is running on a CET enabled platform, you may encounter such an error message(or something similar): "Couldn't find the platform library. rebuild shared object with SHSTK support enabled". It means the system glibc enforces that a CET-enabled application can't load a non-CET shared library. You need to rebuild the sample by adding  -fcf-protection=none option explicitly to disable CET.

## Windows
Supported operating systems:
   * Windows* Server 2016 (Long-Term Servicing Channel)
   * Windows* Server 2019 (Long-Term Servicing Channel)

Requirements:
* Microsoft Visual Studio 2019 or newer.

Prerequisite:
* Intel(R) SGX DCAP Driver
* Intel(R) SGX SDK
* Intel(R) SGX DCAP Packages
* Intel(R) SGX DCAP PCCS (Provisioning Certificate Caching Service)


*Please refer to [SGX DCAP Windows installation guide](https://software.intel.com/en-us/sgx/sdk) to install above dependencies*<br/>
*Note that you need to sign in IDZ first, then download & extract product "Intel(R) Software Guard Extensions Data Center Attestation Primitives"*

1. Generate an ECDSA quote with certification data of type 5 using QuoteGenerationSample
   You need to follow QuoteGenerationSample to setup env and run sample
```
   a. Open VS solution QuoteGenerationSample.sln, build with Debug/Release | x64 configuration
   b. Run App.exe
```

2. Build and run QuoteVerificationSample to verify a given quote
```
   a. Open VS solution QuoteVerificationSample.sln, butil with Debug/Release | x64 configuration
      Note that Release mode need to sign ISV enclave with your own key
   b. Run App.exe
      > App.exe -quote </path/to/quote.dat [default=..\..\..\QuoteGenerationSample\x64\Debug\quote.dat]>
```
