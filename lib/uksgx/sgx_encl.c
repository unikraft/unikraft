#include <uk/sgx_arch.h>
#include <uk/sgx_asm.h>
#include <uk/sgx_internal.h>
#include <uk/arch/paging.h>
#include <uk/arch/atomic.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

extern __u64 sgx_encl_size_max_64;
extern __u64 sgx_xfrm_mask;
extern __u32 sgx_xsave_size_tbl[64];
extern int sgx_va_pages_cnt;
extern struct uk_list_head sgx_tgid_ctx_list;
extern struct uk_mutex sgx_tgid_ctx_mutex;

static int sgx_secs_validate(const struct sgx_secs *secs, unsigned long ssaframesize)
{
	int i;

	if (secs->size == 0)
		return -EINVAL;
	
	/* BASEADDR must be naturally aligned on an SECS.SIZE boundary. */
	if (secs->base & (secs->size - 1))
		return -EINVAL;

	if (secs->size < (2 * PAGE_SIZE))
		return -EINVAL;

	if ((secs->size & (secs->size - 1)) != 0)
		return -EINVAL;

	if (secs->size > sgx_encl_size_max_64)
		return -EINVAL;

	if ((secs->xfrm & 0x3) != 0x3 )
		return -EINVAL;

	/* Check that BNDREGS and BNDCSR are equal. */
	if (((secs->xfrm >> 3) & 1) != ((secs->xfrm >> 4) & 1))
		return -EINVAL;

	if (!secs->ssaframesize || ssaframesize > secs->ssaframesize)
		return -EINVAL;

	for (i = 0; i < SGX_SECS_RESERVED1_SIZE; i++)
		if (secs->reserved1[i])
			return -EINVAL;

	for (i = 0; i < SGX_SECS_RESERVED2_SIZE; i++)
		if (secs->reserved2[i])
			return -EINVAL;

	for (i = 0; i < SGX_SECS_RESERVED3_SIZE; i++)
		if (secs->reserved3[i])
			return -EINVAL;

	for (i = 0; i < SGX_SECS_RESERVED4_SIZE; i++)
		if (secs->reserved4[i])
			return -EINVAL;

	return 0;
}

static __u32 sgx_calc_ssaframesize(__u32 miscselect, __u64 xfrm)
{
	__u32 size_max = PAGE_SIZE;
	__u32 size;
	int i;

	for (i = 2; i < 64; i++) {
		if (!((1 << i) & xfrm))
			continue;

		size = SGX_SSA_GPRS_SIZE + sgx_xsave_size_tbl[i];
		if (miscselect & SGX_MISC_EXINFO)
			size += SGX_SSA_MISC_EXINFO_SIZE;

		if (size > size_max)
			size_max = size;
	}

	return (size_max + PAGE_SIZE - 1) >> PAGE_SHIFT;
}


static struct sgx_encl *sgx_encl_alloc(struct sgx_secs *secs)
{
    unsigned long ssaframesize;
	struct sgx_encl *encl;
    int ret;

    ssaframesize = sgx_calc_ssaframesize(secs->miscselect, secs->xfrm);
	ret = sgx_secs_validate(secs, ssaframesize);
	if (ret) {
		uk_pr_err("Invalid ssaframesize\n");
		return (struct sgx_encl *) 0;
	}
    /* ignore shared memory since Unikraft is single address space */

	encl = malloc(sizeof(struct sgx_encl));

	encl->base = secs->base;
	encl->size = secs->size;
	encl->ssaframesize = ssaframesize;
	encl->attributes = secs->attributes;
	encl->xfrm = secs->xfrm;

	UK_INIT_LIST_HEAD(&encl->encl_list);
	uk_mutex_init(&encl->lock);

	return encl;
}

static int sgx_init_page(struct sgx_encl *encl, struct sgx_encl_page *entry,
			 unsigned long addr, unsigned int alloc_flags)
{
	struct sgx_va_page *va_page;
	struct sgx_epc_page *epc_page = NULL;
	unsigned int va_offset = PAGE_SIZE;
	void *vaddr;
	int ret = 0;

	uk_list_for_each_entry(va_page, &encl->va_pages, list) {
		va_offset = sgx_alloc_va_slot(va_page);
		if (va_offset < PAGE_SIZE)
			break;
	}

	if (va_offset == PAGE_SIZE) {
		va_page = malloc(sizeof(*va_page));
		if (!va_page)
			return -ENOMEM;

		epc_page = sgx_alloc_page(alloc_flags);
		if (epc_page == NULL) {
			free(va_page);
			return NULL;
		}

		vaddr = sgx_get_page(epc_page);
		if (!vaddr) {
			uk_pr_warn("kmap of a new VA page failed %d\n",
				 ret);
			sgx_free_page(epc_page, encl);
			free(va_page);
			return -EFAULT;
		}

		ret = __epa(vaddr);
		sgx_put_page(vaddr);

		if (ret) {
			uk_pr_warn("EPA returned %d\n", ret);
			sgx_free_page(epc_page, encl);
			free(va_page);
			return -EFAULT;
		}

		ukarch_inc(&sgx_va_pages_cnt);

		va_page->epc_page = epc_page;
		va_offset = sgx_alloc_va_slot(va_page);

		mutex_lock(&encl->lock);
		list_add(&va_page->list, &encl->va_pages);
		mutex_unlock(&encl->lock);
	}

	entry->va_page = va_page;
	entry->va_offset = va_offset;
	entry->addr = addr;

	return 0;
}

/**
 * sgx_encl_create - create an enclave
 *
 * @secs:	page aligned SECS data
 *
 * Validates SECS attributes, allocates an EPC page for the SECS and creates
 * the enclave by performing ECREATE.
 *
 * Return:
 * 0 on success,
 * system error on failure
 */
int sgx_encl_create(struct sgx_secs *secs) 
{
    struct sgx_pageinfo pginfo;
	struct sgx_secinfo secinfo;
	struct sgx_encl *encl;
	struct sgx_epc_page *secs_epc;
	void *secs_vaddr;
	long ret;

    encl = sgx_encl_alloc(secs);
    if (!encl)
        goto err;

	memset(&pginfo, 0, sizeof(struct sgx_pageinfo));
	memset(&secinfo, 0, sizeof(struct sgx_secinfo));
	pginfo.linaddr = 0;
	pginfo.srcpge = (unsigned long) secs;
	pginfo.secinfo = &secinfo;
	pginfo.secs = 0;

	secs_epc = sgx_alloc_page(0);
	if (!secs_epc) {
		goto err;
	}

	encl->secs.epc_page = secs_epc;

	/*
	 * pid and tgid related operations are not required, as Unikraft is 
	 * single-process and every thread belongs to the same process.
	 */

	ret = sgx_init_page(encl, &encl->secs, encl->base + encl->size, 0);
	if (ret) {
		goto err;
	}

	secs_vaddr = sgx_get_page(secs_epc);

	/* perform the real ECREATE instruction */
	ret = __ecreate((void *)&pginfo, secs_vaddr);

	sgx_put_page(secs_vaddr);

	if (ret) {
		uk_pr_err("ECREATE returned %ld\n", ret);
		ret = -EFAULT;
		goto err;
	}

	/* 
	 * We need to handle the VMA issue manually (no API supported yet)
	 * to maintain a list of SGX-related VMA so that we can manage the
	 * map/unmap of the pages to be added
	 */


    return 0;

err:
    return -EINVAL;
}