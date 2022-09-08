Intel(R) Software Guard Extensions Data Center Attestation Primitives
================================================

Introduction
-------
Intel(R) Software Guard Extensions (Intel(R) SGX) Data Center Attestation Primitives (Intel(R) SGX DCAP) provides SGX attestation support targeted for data centers, cloud services providers and enterprises. This attestation model leverages Elliptic Curve Digital Signature algorithm (ECDSA) versus the current client based SGX attestation model which is EPID based (Enhanced Privacy Identification).

License
-------
This project is BSD license. See [License.txt](License.txt)

But Linux driver code is dual licensed under BSD/GPL v2. See [License.txt](driver/linux/License.txt) 

Third-party code is also used in this project. See [ThirdPartyLicenses.txt](QuoteGeneration/ThirdPartyLicenses.txt), [ThirdPartyLicenses.txt](QuoteVerification/QVL/ThirdPartyLicenses.txt) and [ThirdPartyLicenses.txt](driver/win/ThirdPartyLicenses.txt) for details.

Contributing
-------
See [CONTRIBUTING.md](CONTRIBUTING.md) for details.

Instruction
-------
## Build and Install the Intel(R) SGX Driver
   `README.md` files are provided under [Windows driver](driver/win) folder and [Linux driver](driver/linux) folder individually. Please follow the instructions in the corresponding `README.md` to build and install the Intel(R) SGX driver based on your target OS.

## Build Intel(R) SGX DCAP Quote Generation and Intel(R) SGX DCAP Quote Verification projects
Intel(R) SGX DCAP Quote Generation and Intel(R) SGX DCAP Quote Verification can be built on Linux by running ``make`` from root directory. To build on Windows, please refer the README.md in subdirectories.

## Build and Install the Intel(R) SGX DCAP Quote Generation Library
A [README.md](QuoteGeneration/README.md) is provided under [QuoteGeneration](QuoteGeneration) folder. Please follow the instructions in the `README.md` to build and install Intel(R) SGX DCAP Quote Generation Library.

## Build and Install the Intel(R) SGX DCAP Quote Verification Library
A [README.md](QuoteVerification/README.md) is provided under [QuoteVerification](QuoteVerification) folder. Please follow the instructions in the `README.md` to build and install Intel(R) SGX DCAP Quote Verification Library.

