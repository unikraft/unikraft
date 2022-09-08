#include <uk/sgx_user.h>
#include <uk/sgx_internal.h>
#include <uk/sgx_asm.h>
#include <uk/print.h>
#include <uk/arch/paging.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* 
 * TODO: We need to handle the VMA issue manually (no API supported yet)
 * to access the list of SGX-related VMA so that we can retrieves the pages
 */
int sgx_get_encl(unsigned int addr, struct sgx_encl **encl)
{
	(void) addr;
	(void) encl;
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
	struct sgx_enclave_create *createp = (struct sgx_enclave_create *)arg;
	void *src = (void *) createp->src;
	struct sgx_secs *secs;
	int ret;

	/* initialize SECS */
	secs = (struct sgx_secs *) malloc(sizeof(*secs));
	if (!secs)
		return -ENOMEM;

	ret = (int) memcpy((void *)secs, src, sizeof(*secs)); /* don't need to care about copy_from_user() */
	if (ret) {
		free(secs);
		return ret;
	}

	/* create enclave from given SECS */
	ret = sgx_encl_create(secs);
	
	free(secs);

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
    struct sgx_enclave_add_page *addp = (void *)arg;
	unsigned long secinfop = (unsigned long)addp->secinfo;
	struct sgx_secinfo secinfo;
	struct sgx_encl *encl;
	void *data;
	int ret;

	ret = sgx_get_encl(addp->addr, &encl);
	if (ret)
		return ret;

	if (memcpy(&secinfo, (void *)secinfop, sizeof(secinfo))) {
		uk_refcount_put(&encl->refcount, sgx_encl_release);
		return -EFAULT;
	}

	/* alloc_page() and kmap() */
	data = malloc(__PAGE_SIZE);
	if (data == NULL) {
		uk_refcount_put(&encl->refcount, sgx_encl_release);
		return -ENOMEM;
	}

	ret = memcpy(data, (void *)addp->src, __PAGE_SIZE);
	if (ret) 
		goto out;
	

	ret = sgx_encl_add_page(encl, addp->addr, data, &secinfo, addp->mrmask);
	if (ret)
		goto out;
    return 0;

out:
	uk_pr_crit("failed to add page\n");
	uk_refcount_put(&encl->refcount, sgx_encl_release);
	free(data);
	return ret;
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
    struct sgx_enclave_init *initp = (struct sgx_enclave_init *)arg;
	unsigned long sigstructp = (unsigned long)initp->sigstruct;
	unsigned long einittokenp = (unsigned long)initp->einittoken;
	unsigned long encl_id = initp->addr;
	struct sgx_sigstruct *sigstruct;
	struct sgx_einittoken *einittoken;
	struct sgx_encl *encl;
	int ret;

	sigstruct = malloc(sizeof(struct sgx_sigstruct));
	einittoken = (struct sgx_einittoken *)
		((unsigned long)sigstruct + PAGE_SIZE / 2);

	ret = memcpy(sigstruct, (void *)sigstructp, sizeof(*sigstruct));
	if (ret)
		goto out;

	ret = memcpy(einittoken, (void *)einittokenp, sizeof(*einittoken));
	if (ret)
		goto out;

	ret = sgx_get_encl(encl_id, &encl);
	if (ret)
		goto out;

	ret = sgx_encl_init(encl, sigstruct, einittoken);

	uk_refcount_put(&encl->refcount, sgx_encl_release);

out:
	free(sigstruct);
	return ret;
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