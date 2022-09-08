Transition from DCAP driver to kernel
=====================================

This documents impact of transitioning SGX applications from the DCAP driver to in-kernel SGX support once the SGX upstream patches are in mainline releases (5.11 or higher).
The upstream patches are currently hosted [here](https://git.kernel.org/pub/scm/linux/kernel/git/tip/tip.git/log/?h=x86/sgx) and merged into 5.11 release candidates.

Transition for apps using the common loader
------------------------------------------
This section is for the developers building apps on top of the [Intel SGX common loader](https://github.com/intel/linux-sgx/blob/master/psw/enclave_common/sgx_enclave_common.h) as the interface layer to kernel space SGX support. Known such apps include all those applications built with [Intel SGX SDK](https://github.com/intel/linux-sgx), [Open Enclave SDK](https://github.com/openenclave/openenclave), [Rust SGX SDK](https://github.com/apache/incubator-teaclave-sgx-sdk), [Occlum LibOS](https://github.com/occlum/occlum). 

The SGX common loader is provided in the package named "libsgx-enclave-common" as part of Intel SGX PSW releases. It abstracts differences between in-kernel SGX support and Intel supported out-of-tree drivers: both this driver and [the legacy non-FLC driver](https://github.com/intel/linux-sgx-driver). Therefore,  apps using libsgx-enclave-common for enclave loading would not require any code changes. However, deployment time adjustments may be needed as described in this section.

For app developers using an alternative SGX runtime not built on top of the common loader, please consult with developer/provider of that runtime for exact transition information.

For alternative SGX runtime implementors, this section can be adapted to create transition guide for apps using your runtime. The [next section](#notes-for-runtime-implementors) documents differences between the DCAP driver and in-kernel SGX support.

### udev rules

A main-line kernel with SGX support will expose dev nodes to flat files.  Instead of /dev/sgx/enclave and /dev/sgx/provision exposed by the DCAP driver and earlier versions of upstream patches, the kernel exposes those at /dev/sgx_enclave and /dev/sgx_provision.

To remap those nodes to be compatible with existing user space software (e.g., Intel SGX PSW 2.12 or earlier), as root user, add a new udev rules file and activate it as following. 

```
# groupadd sgx_prv
# cat > /etc/udev/rules.d/90-sgx-v40.rules <<EOF
SUBSYSTEM=="misc",KERNEL=="sgx_enclave",MODE="0666",SYMLINK+="sgx/enclave"
SUBSYSTEM=="misc",KERNEL=="sgx_provision",GROUP="sgx_prv",MODE="0660",SYMLINK+="sgx/provision"
EOF
# udevadm trigger
```
*Note:* This step won't be needed in following cases. 
- When the sgx_prv group name and udev rules are upstreamed to systemd repo and included in distros by default. 
- Some distro image (e.g., ubuntu for azure) may adopt these changes earlier to include the udev rules and sgx_prv group.
- Later versions of Intel SGX PSW packages libsgx-enclave-common and sgx-aesm-service will automatically install the udev rules and add the group.

### Apps with "in-process" quote

If your app uses Intel SGX AESM service for "out-of-process" quote generation (quote generated in AESM process using Intel signed PCE and QE), then the AESM installer will do the configuration described here and you can ignore this section.

If your app is doing so-called "in-process" quote generation, i.e., it loads provisioning/quoting enclaves by itself including Intel signed PCE, QE, then the app needs be run with a uid in sgx_prv group, please refer to installation steps detailed in main [README](https://github.com/intel/SGXDataCenterAttestationPrimitives/tree/master/driver/linux#launching-an-enclave-with-provision-bit-set).

*Note:* Without proper access, the app will fail on loading the provisioning enclaves with an error of ENCLAVE_NOT_AUTHORIZED(0x0006) from the enclave common loader. Different runtime may translate it to different higher level code, e.g. SGX_ERROR_SERVICE_INVALID_PRIVILEGE(0x4004) in Intel SGX uRTS. 

Notes for runtime implementors
---------------------------------

If you are implementing an SGX runtime directly interacting with kernel or the DCAP driver, you need (likely already) follow closely the upstream SGX kernel development and adjust your implementation to any new changes.

For your reference, this section lists the adjustment we have made over time for different versions of SGX kernel patches and the differences between latest kernel implementation and the DCAP driver.

### Change history for different upstream patch versions and impact

- v25 (DCAP driver 1.30): Migrated /dev/sgx/* to misc
  - no code changes, updated [udev rules](https://github.com/intel/SGXDataCenterAttestationPrimitives/blob/771604d36945bba463e012702da8f306bccc2bf7/driver/linux/10-sgx.rules)
- v27 (DCAP driver 1.33): Disallow Read-Implies-Excute processes to use enclaves as there could a permission conflict between VMA and enclave permissions.
  - No common loader code changes, only need make sure app hosting enclave does not have "execstack"
  - Turn on "-z noexecstack" linker option for the app like [this](https://github.com/intel/linux-sgx/blob/master/SampleCode/SampleCommonLoader/Makefile#L35)
- v29 (DCAP driver 1.34): Disallow mmap(PROT_NONE) from /dev/sgx.
  - The [common loader changes](https://github.com/intel/linux-sgx/commit/32169592d0c4eecaed7c669e7f9147934eb0dfb5#diff-3d3b2c74e81956c18c2d7f74ad33ff6c1dadb7496d71697a1f6ec254255191fb): Anonymous mapping is used to reserve the address range, then remapped after EADD.
- v40: moved device nodes from /dev/sgx/\[enclave,provision\] to flat nodes /dev/sgx_\[enclave,provision\]
  - no code changes, but new udev rules described in the section above. 

### Differences between kernel and the DCAP driver

- [vDSO Interfaces](https://git.kernel.org/pub/scm/linux/kernel/git/tip/tip.git/commit/?h=x86/sgx&id=84664369520170f48546c55cbc1f3fbde9b1e140)
  - Kernel provides a vDSO interface for EENTER and ERESUME. This is not possible to implement in OOT drivers.
- [PFEC.SGX support](https://git.kernel.org/pub/scm/linux/kernel/git/tip/tip.git/commit/?h=x86/sgx&id=74faeee06db81a06add0def6a394210c8fef0ab7)
   - Kernel signals SIGSEV to user space for EPCM induced #PF, in cases that PFEC.SGX bit is set. 
- [New vma_ops->mprotect hook](https://git.kernel.org/pub/scm/linux/kernel/git/tip/tip.git/commit/?h=x86/sgx&id=95bb7c42ac8a94ce3d0eb059ad64430390351ccb)
  - Kernel caps PTE permissions of an enclave page to EPCM permissions through both existing mmap and the new mprotect hook.
  - DCAP driver can not have the new hook for mprotect, only enforces limits during mmap. 
- Intel key in provisioning allowed list
  - The DCAP driver loads Intel signed provisioning enclaves without checking permissions to /dev/sgx_provision. The kernel does not treat any key differently.
  - This was intended to ease deployment for some legacy applications during transition. Future DCAP driver will remove the allowed list.
