# ukrsi: Realm Services Interface library

## Implementation

The latest [Realm Management Monitor specification](https://developer.arm.com/documentation/den0137/latest/) specifies the RSI commands, which provide certain functionalities for the realm VM.
These commands include:

* `RSI_ATTESTATION_TOKEN_CONTINUE`
* `RSI_ATTESTATION_TOKEN_INIT`
* `RSI_HOST_CALL`
* `RSI_IPA_STATE_GET`
* `RSI_IPA_STATE_SET`
* `RSI_MEASUREMENT_EXTEND`
* `RSI_MEASUREMENT_READ`
* `RSI_REALM_CONFIG`
* `RSI_VERSION`

In `ukrsi`, each command is implemented as a call to `smccc_invoke` with the corresponding function ID and parameters.

`ukrsi` also adds functions like `uk_rsi_init`, `uk_rsi_generate_attestation_token`, `uk_rsi_init_device` to initialize the environment or simplify the usage of RSI commands.

## Testing Applications in the Realm World

To run an application in the realm world, we need to use the following three steps:

* Preparing the FVP environment.
* Building the application with Arm CCA support.
* Running the application in the realm world.

### Environment Setup

As a reference, Arm has provided an [integration stack](https://gitlab.arm.com/arm-reference-solutions/arm-reference-solutions-docs/-/blob/master/docs/aemfvp-a-rme/user-guide.rst) for Arm's reference CCA software architecture and implementation.
The main components in the reference integration are [Linux-CCA](https://gitlab.arm.com/linux-arm/linux-cca), [Kvmtool-CCA](https://gitlab.arm.com/linux-arm/kvmtool-cca), [Trusted-Firmware-A](https://trustedfirmware-a.readthedocs.io/en/latest/), [Hafnium](https://review.trustedfirmware.org/plugins/gitiles/hafnium/hafnium/+/HEAD/README.md), [TF-RMM](https://tf-rmm.readthedocs.io/en/latest/).
This project uses this integration stack to run applications in the realm world.
To run the code from this project, please follow [the guide](https://gitlab.arm.com/arm-reference-solutions/arm-reference-solutions-docs/-/blob/master/docs/aemfvp-a-rme/user-guide.rst) to setup build environment, build the software stack and obtain the Armv-A Base RevC AEM FVP.

## Configuring Applications to Use `ukrsi`

Kvmtool supports running an VM in the realm world, so we need to build the image of Unikraft applications into a format that kvmtool can use.

To configure the application to run in the realm world we follow the steps below:

1. Enter the configuration interface by running:

   ```console
   $ make menuconfig
   ```

2. Under `Platform Configuration -> KVM guest -> Virtual Machine Monitor`, select `Firecracker`, whose boot protocol is compatible with kvmtool.

3. Under `Console Options` select `ns16550` serial console instead of `pl011`.

4. Select `Platform Configuration -> Platform Interface Options -> Virtual memory API`.

5. Select `Device Drivers -> Interrupt controller -> Arm Generic Interrupt Controller (GICv3)`.

6. Select `Architecture Selection -> Armv9-A Extensions -> Armv9 Realm Management Extension`.

7. Select `Device Drivers -> Arm CCA -> ukrsi: Realm Services Interface library`.

8. To enable the tests of `ukrsi`, select `Enable tests` in the `ukrsi` menu.

9. After building the application, we need to copy the images and relevant files to the image for FVP, which might be located in `rme-stack/output/aemfvp-a-rme/host-fs.ext4`.

### Running Applications

To start the working environment with Arm CCA support, please run the following command:

```shell
./model-scripts/aemfvp-a-rme/boot.sh -p aemfvp-a-rme shell
```

This command boots the entire software stack and runs a Linux shell in the normal world.
After logging in, we can then run various applications using kvmtool.

The following are the commands to run applications in the realm world using kvmtool.

```shell
lkvm run --realm -k <path-to-image> -c 1 --nodefaults
```

To additionally setup `initrd`, `virtio-9p`, and `virtio-net`, we can append the following options to the command above respectively:

* `initrd`: `-i <path-to-initrd>`
* `virtio-9p`: `--9p ./fs0,fs0`
* `virtio-net`: `--network virtio -p "netdev.ipv4_addr=192.168.33.15 netdev.ipv4_gw_addr=192.168.33.1 netdev.ipv4_subnet_mask=255.255.255.0 --"`

The following are related options of `lkvm run` that we would use to run applications.
```shell
 usage: lkvm run [<options>] [<kernel image>]

Basic options:
        --9p <dir_to_share,tag_name>
                          Enable virtio 9p to share files between host and guest

Kernel options:
    -k, --kernel <kernel>
                          Kernel to boot in virtual machine
    -i, --initrd <initrd>
                          Initial RAM disk image
    -p, --params <params>
                          Kernel command line arguments

Networking options:
    -n, --network <network params>
                          Create a new guest NIC

Arch-specific options:
        --realm           Create VM running in a realm using Arm RME
```
