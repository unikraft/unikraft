Intel(R) Software Guard Extensions Driver for Data Center Attestation Primitives (Intel(R) SGX DCAP) for Windows* OS
================================================================================

## Prerequisites
- Ensure that you have the following required operating systems:
   * Windows* Server 2016 (Long-Term Servicing Channel)
   * Windows* Server 2019 (Long-Term Servicing Channel)
- Ensure that you have the following required hardware:
  * 8th Generation Intel(R) Core(TM) Processor or newer with **Flexible Launch Control** support*
  * Intel(R) Atom(TM) Processor with **Flexible Launch Control** support*
- Configure the system with the **SGX hardware enabled** option.
- Ensure that you have installed Microsoft Visual C++ Compiler* version 14.14 or higher provided by Microsoft Visual Studio* 2017 version 15.7.
- Ensure that you have installed Windows Driver Kit for Win 10, version 10.0.17763.

## Build the Intel(R) SGX DCAP driver
Open the Microsoft Visual Studio* solution `WinLe.sln` and trigger a build.


## Install the Intel(R) SGX DCAP driver
- On Windows Server 2016 LTSC:    
    To install the Launch Configuration Driver Set, use the devcon utility, which is provided with the Windows* 10 Driver Kit and located in the following directory: "C:\Program Files (x86)\Windows Kits\10\tools\x64\devcon.ex”. You may need to add this directory to your paths.    
    Run the following command in the Administrator Command Window:
```    
    Devcon.exe  install  sgx_base_dev.inf  root\SgxLCDevice
```	
- On Windows Server 2019 LTSC:    
    You can trigger `sgx_base.inf` directly to install Launch Service integrated SGX Driver.

