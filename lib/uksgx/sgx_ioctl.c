#include <uk/sgx_user.h>
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
    WARN_STUBBED();
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

int sgx_ioc_page_modpr(struct device *dev, unsigned int cmd,
			unsigned int arg)
{
    WARN_STUBBED();
	return 0;
}

/**
 * sgx_ioc_page_to_tcs() - Pages defined in range are switched to TCS.
 * These pages should be of type REG.
 * eaccept needs to be invoked after return.
 * @arg range address of pages to be switched
 */
int sgx_ioc_page_to_tcs(struct device *dev, unsigned int cmd,
			 unsigned int arg)
{
    WARN_STUBBED();
	return 0;
}

/**
 * sgx_ioc_trim_page() - Pages defined in range are being trimmed.
 * These pages still beint to the enclave and can not be removed until
 * eaccept has been invoked
 * @arg range address of pages to be trimmed
 */
int sgx_ioc_trim_page(struct device *dev, unsigned int cmd,
		       unsigned int arg)
{
    WARN_STUBBED();
	return 0;
}

// /**
//  * sgx_ioc_page_notify_accept() - Pages defined in range will be moved to
//  * the trimmed list, i.e. they can be freely removed from now. These pages
//  * should have PT_TRIM page type and should have been eaccepted priorly
//  * @arg range address of pages
//  */
// int sgx_ioc_page_notify_accept(struct device *dev, unsigned int cmd,
// 				unsigned int arg)
// {
// 	struct sgx_range *rg;
// 	unsigned int address, end;
// 	struct sgx_encl *encl;
// 	int ret, tmp_ret = 0;

// 	if (!sgx_has_sgx2)
// 		return -ENOSYS;

// 	rg = (struct sgx_range *)arg;

// 	address = rg->start_addr;
// 	address &= ~(PAGE_SIZE-1);
// 	end = address + rg->nr_pages * PAGE_SIZE;

// 	ret = sgx_get_encl(address, &encl);
// 	if (ret) {
// 		pr_warn("sgx: No enclave found at start address 0x%lx\n",
// 			address);
// 		return ret;
// 	}

// 	for (; address < end; address += PAGE_SIZE) {
// 		tmp_ret = remove_page(encl, address, true);
// 		if (tmp_ret) {
// 			sgx_dbg(encl, "sgx: remove failed, addr=0x%lx ret=%d\n",
// 				 address, tmp_ret);
// 			ret = tmp_ret;
// 			continue;
// 		}
// 	}

// 	kref_put(&encl->refcount, sgx_encl_release);

// 	return ret;
// }

// /**
//  * sgx_ioc_page_remove() - Pages defined by address will be removed
//  * @arg address of page
//  */
// int sgx_ioc_page_remove(struct device *dev, unsigned int cmd,
// 			 unsigned int arg)
// {
// 	struct sgx_encl *encl;
// 	unsigned int address = *((unsigned int *) arg);
// 	int ret;

// 	if (!sgx_has_sgx2)
// 		return -ENOSYS;

// 	if (sgx_get_encl(address, &encl)) {
// 		pr_warn("sgx: No enclave found at start address 0x%lx\n",
// 			address);
// 		return -EINVAL;
// 	}

// 	ret = remove_page(encl, address, false);
// 	if (ret) {
// 		pr_warn("sgx: Failed to remove page, address=0x%lx ret=%d\n",
// 			address, ret);
// 	}

// 	kref_put(&encl->refcount, sgx_encl_release);
// 	return ret;
// }

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
	case SGX_IOC_ENCLAVE_EMODPR:
		handler = sgx_ioc_page_modpr;
		break;
	case SGX_IOC_ENCLAVE_MKTCS:
		handler = sgx_ioc_page_to_tcs;
		break;
	case SGX_IOC_ENCLAVE_TRIM:
		handler = sgx_ioc_trim_page;
		break;
	// case SGX_IOC_ENCLAVE_NOTIFY_ACCEPT:
	// 	handler = sgx_ioc_page_notify_accept;
	// 	break;
	// case SGX_IOC_ENCLAVE_PAGE_REMOVE:
	// 	handler = sgx_ioc_page_remove;
	// 	break;
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