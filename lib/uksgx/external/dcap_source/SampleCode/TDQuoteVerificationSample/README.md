Intel(R) Software Guard Extensions Data Center Attestation Primitives (Intel(R) SGX DCAP) Quote Verification SampleCode
================================================

## Linux
Supported operating systems:
* CentOS 8.3  64bits

Requirements:
* make
* gcc
* g++
* bash shell

Prerequisite:
* Installed Intel(R) TDX DCAP Verfication Packages
* Installed Intel(R) Quote Generation Service Packages
* Installed Intel(R) SGX DCAP PCCS (Provisioning Certificate Caching Service)
* Intel(R) SGX DCAP Packages(only needed in trusted mode of quote verification)
If want verified quote in trusted mode
* Intel(R) SGX SDK

*Please refer to SGX DCAP Linux installation guide "https://download.01.org/intel-sgx/sgx-dcap/#version#/linux/docs/Intel_SGX_DCAP_Linux_SW_Installation_Guide.pdf" to install above dependencies*<br/>
*Note that you need to change **\#version\#** to actual version number in URL, such as 1.11.*


1. Generate an ECDSA quote with certification data of type 5 using this Quote Generation Sample Code
```
   $ cd /opt/intel/tdx-quote-generation-sample
   $ make
   $ ./test_tdx_attest
```

2. Build and run TD-based Quote Verification Sample to verify a given quote
```
   Untrusted mode build:
   $ make UNTRUSTED_VERIFY=1
   Trused & Untrusted mode debug build:
   $ make DEBUG=1
   $ ./app -quote </path/to/quote.dat> [default=../tdx-quote-generation-sample/quote.dat]>
   Trused & Untrusted mode Release build:
   $ make `#You need to sign ISV enclave with your own key in this mode`
   $ ./app -quote </path/to/quote.dat> [default=../tdx-quote-generation-sample/quote.dat]>
```
**Note**: Our libdcap_quoteprov.so is not built with Intel(R) Control Flow Enforcement Technology(CET) feature. If the sample is built with CET feature(it can be enabled by the compiler's default setting) and it is running on a CET enabled platform, you may encounter such an error message(or something similar): "Couldn't find the platform library. rebuild shared object with SHSTK support enabled". It means the system glibc enforces that a CET-enabled application can't load a non-CET shared library. You need to rebuild the sample by adding  -fcf-protection=none option explicitly to disable CET.
