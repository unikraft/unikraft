#include <uk/sgx_user.h>
#include <uk/sgx_internal.h>
#include <uk/print.h>
#include <errno.h>

int sgx_get_encl(unsigned int addr, struct sgx_encl **encl)
{
    WARN_STUBBED();
	return 0;
}

/**
 * sgx_ioc_enclave_create - handler for %SGX_IOC_ENCLAVE_CREATE
 * @dev:	open file to /dev/sgx
 * @cmd:	the command value
 * @arg:	pointer to the &struct sgx_enclave_create
 *
 * Validates SECS attributes, allocates an EPC page for the SECS and performs
 * ECREATE.
 *
 * Return:
 * 0 on success,
 * system error on failure
 */
static int sgx_ioc_enclave_create(struct device *dev, unsigned int cmd,
				   unsigned int arg)
{
    // WARN_STUBBED();
	// test switch_to_ring3
	__u64 addr;
	// get current rip and save it to addr
	asm volatile(" \
	lea 0(%%rip), %0; \
	": "=r" (addr));
	switch_to_ring3();
	return 0;
}

/**
 * sgx_ioc_enclave_add_page - handler for %SGX_IOC_ENCLAVE_ADD_PAGE
 *
 * @dev:	open file to /dev/sgx
 * @cmd:	the command value
 * @arg:	pointer to the &struct sgx_enclave_add_page
 *
 * Creates a new enclave page and enqueues an EADD operation that will be
 * processed by a worker thread later on.
 *
 * Return:
 * 0 on success,
 * system error on failure
 */
static int sgx_ioc_enclave_add_page(struct device *dev, unsigned int cmd,
				     unsigned int arg)
{
    WARN_STUBBED();
    return 0;
}

/**
 * sgx_ioc_enclave_init - handler for %SGX_IOC_ENCLAVE_INIT
 *
 * @dev:	open file to /dev/sgx
 * @cmd:	the command value
 * @arg:	pointer to the &struct sgx_enclave_init
 *
 * Flushes the remaining enqueued EADD operations and performs EINIT.
 *
 * Return:
 * 0 on success,
 * system error on failure
 */
static int sgx_ioc_enclave_init(struct device *dev, unsigned int cmd,
				 unsigned int arg)
{
    WARN_STUBBED();
	return 0;
}

typedef int (*sgx_ioc_t)(struct device *, unsigned int, void *);
#define ENOIOCTLCMD	515	/* No ioctl command */

int sgx_ioctl(struct device *dev, unsigned long cmd, void *arg)
{
	char data[256];
	sgx_ioc_t handler = __NULL;
	int ret;
    
	switch (cmd) {
	case SGX_IOC_ENCLAVE_CREATE:
		handler = sgx_ioc_enclave_create;
		break;
	case SGX_IOC_ENCLAVE_ADD_PAGE:
		handler = sgx_ioc_enclave_add_page;
		break;
	case SGX_IOC_ENCLAVE_INIT:
		handler = sgx_ioc_enclave_init;
		break;
	default:
		return -ENOIOCTLCMD;
	}

	// if (copy_from_user(data, (void *)arg, _IOC_SIZE(cmd)))
	// 	return -EFAULT;

	ret = handler(dev, cmd, (unsigned int)((void *)data));
	// if (!ret && (cmd & IOC_OUT)) {
	// 	if (copy_to_user((void *)arg, data, _IOC_SIZE(cmd)))
	// 		return -EFAULT;
	// }

	return ret;
}

int sgx_open(struct device *dev, int flags)
{
    WARN_STUBBED();
    return 0;
}

int sgx_close(struct device *dev)
{
    WARN_STUBBED();
    return 0;
}