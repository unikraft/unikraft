# sgx_switchless

sgx_switchless is a collection of *switchless features*, which can eliminate *enclave transitions* from SGX applications. This branch of the codebase only includes **Fast ECalls/OCalls**.

## Motivation

In a running SGX application, enclave transitions occur whenever the CPU switches between enclave and non-enclave mode (e.g., user mode or privileged mode). Enclave transitions are *extremely expensive*: on the current hardware platform, the overhead of an enclave transition is measured to be *60X more expensive* than that of switching between user and privileged mode (typically, when making system calls). 

From the perspective of SGX developers, the only way to trigger enclave transitions from inside enclaves is via OCalls. And it is this OCall mechanism that--- behind the scene--- enables the users to synchronize between SGX threads (see `sgx_threads.h`), request application-specific services from outside the enclave and perform I/O operations with the untrusted environment. Therefore, SGX applications that heavily depend on OCalls may suffer a considerable performance degradation due to enclave transitions.

## How to Use

sgx_switchless has been built and tested on Ubuntu 16.04. For now, it does not support 32-bit CPUs or SGX simulation mode.

### Build

Run the following command in directory `/sdk/switchless`

    $ make  

to build two libraries of sgx_switchless:  the trusted library `libsgx_tswitchless.a` and the untrusted library `libsgx_uswitchless.so`. Users should link the two libraries to the object files for trusted and untrusted components, respectively.

### Test

Run the following command at `/sdk/switchless/test`

    $ make && make test 

to compile and run the test cases.


### Sample Code

See the sample project at  `/SampleCode/FastOCalls`. To compile and run the sample project correctly, SGX SDK and PSW with Switchless SGX should be compiled and installed.
