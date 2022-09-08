## Build the Intel(R) SGX Quote Verification Library
- To set the environment variables, enter the following command:
```
  $ source ${SGX_PACKAGES_PATH}/sgxsdk/environment
```
- To build the Intel(R) SGX Quote Verification Library, enter the following command:
```
   $ cd linux
   $ make [Optional: DCAP_DIR=<DCAP_ROOT_DIR>
```
The target library named ``libdcap_quoteverify.so`` will be generated.

- To build debug library, enter the following command:
```
  $ make DEBUG=1
```
