The ippcp library is built based on the Open Source project ipp-crypto:
   * https://github.com/intel/ipp-crypto/
   * tag: [ippcp_2021.3](https://github.com/intel/ipp-crypto/tree/ippcp_2021.3)

In order to build your own IPP crypto, please follow below steps:
1. Download the prebuilt mitigation tools package `as.ld.objdump.{ver}.tar.gz` from [01.org](https://download.01.org/intel-sgx/latest/linux-latest/), extract the package and copy the tools to `/usr/local/bin`.
2. Read the ipp-crypto README to prepare your build environment.
3. Make sure ipp-crypto source code are prepared.
4. Build the target ippcp library with the prepared Makefile:
   a. Build the target ippcp library with All-Loads-Mitigation:
      $ make MITIGATION-CVE-2020-0551=LOAD
   b. Build the target ippcp library with Branch-Mitigation:
      $ make MITIGATION-CVE-2020-0551=CF
   c. Build the target ippcp library with No-Mitigation:
      $ make
The built-out static library `libippcp.a` and header files will be copied into the right place.
Remember to "make clean" before switching the build.

For IPP crypto reproducible build, please follow the instructions in [reproducibility README.md](../../linux/reproducibility/README.md) to reproduce the prebuilt IPP crypto.
