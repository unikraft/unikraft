# SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
# Copyright(c) 2016-21 Intel Corporation.

ifneq ($(KERNELRELEASE),)

obj-m += intel_sgx.o
intel_sgx-y := encl.o main.o driver.o ioctl.o

else

KDIR := /lib/modules/$(shell uname -r)/build
KSYM_MMPUT_ASYNC := $(shell grep  "mmput_async\svmlinux\sEXPORT" $(KDIR)/Module.symvers)
KSYM_LOOKUP := $(shell grep "kallsyms_lookup_name\svmlinux\sEXPORT" $(KDIR)/Module.symvers)
EXTRA_CFLAGS :=
ifneq ($(KSYM_MMPUT_ASYNC),)
	EXTRA_CFLAGS += -DHAVE_MMPUT_ASYNC
endif
ifneq ($(KSYM_LOOKUP),)
	 EXTRA_CFLAGS += -DHAVE_KSYM_LOOKUP
endif
INKERNEL_SGX :=$(shell cat $(KDIR)/.config | grep "CONFIG_X86_SGX=y\|CONFIG_INTEL_SGX=y")
ifneq ($(INKERNEL_SGX),)
default:
	$(error Can't install DCAP SGX driver with inkernel SGX support)

else

PWD  := $(shell pwd)
EXTRA_CFLAGS += -I$(PWD) -I$(PWD)/include -D_FORTIFY_SOURCE=2 -Wl,-z,relro,-z,now
EXTRA_LDFLAGS := -z noexecstack

default:
	$(MAKE) -C $(KDIR) M=$(PWD) LDFLAGS_MODULE="$(EXTRA_LDFLAGS)" CFLAGS_MODULE="$(EXTRA_CFLAGS)" modules

endif
endif

clean:
	rm -vrf *.o *.ko *.order *.symvers *.mod.c .tmp_versions .*.cmd *.o.ur-safe *.mod
