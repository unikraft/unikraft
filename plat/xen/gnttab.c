/* SPDX-License-Identifier: MIT */
/*
 ****************************************************************************
 * (C) 2006 - Cambridge University
 ****************************************************************************
 *
 *        File: gnttab.c
 *      Author: Steven Smith (sos22@cam.ac.uk)
 *     Changes: Grzegorz Milos (gm281@cam.ac.uk)
 *
 *        Date: July 2006
 *
 * Environment: Xen Minimal OS
 * Description: Simple grant tables implementation. About as stupid as it's
 *  possible to be and still work.
 *
 ****************************************************************************
 */
#include <stdint.h>
#ifdef DBGGNT
#include <string.h>
#endif
#include <uk/arch/limits.h>
#include <uk/arch/atomic.h>
#include <uk/plat/lcpu.h>
#include <uk/semaphore.h>
#include <common/gnttab.h>
#include <xen-x86/mm.h>

#include <xen-x86/hypercall.h>

/* NR_GRANT_FRAMES must be less than or equal to that configured in Xen */
#define NR_GRANT_FRAMES         4
#define NR_GRANT_ENTRIES \
	(NR_GRANT_FRAMES * PAGE_SIZE / sizeof(grant_entry_v1_t))

static struct gnttab {
	int initialized;
	struct uk_semaphore sem;
	grant_entry_v1_t *table;
	grant_ref_t gref_list[NR_GRANT_ENTRIES];
#ifdef DBGGNT
	char inuse[NR_GRANT_ENTRIES];
#endif
} gnttab;


static grant_ref_t get_free_entry(void)
{
	grant_ref_t gref;
	unsigned long flags;

	uk_semaphore_down(&gnttab.sem);

	flags = ukplat_lcpu_save_irqf();

	gref = gnttab.gref_list[0];
	UK_ASSERT(gref >= GNTTAB_NR_RESERVED_ENTRIES &&
		gref < NR_GRANT_ENTRIES);
	gnttab.gref_list[0] = gnttab.gref_list[gref];
#ifdef DBGGNT
	UK_ASSERT(!gnttab.inuse[gref]);
	gnttab.inuse[gref] = 1;
#endif

	ukplat_lcpu_restore_irqf(flags);

	return gref;
}

static void put_free_entry(grant_ref_t gref)
{
	unsigned long flags;

	flags = ukplat_lcpu_save_irqf();

#ifdef DBGGNT
	UK_ASSERT(gnttab.inuse[gref]);
	gnttab.inuse[gref] = 0;
#endif
	gnttab.gref_list[gref] = gnttab.gref_list[0];
	gnttab.gref_list[0] = gref;

	ukplat_lcpu_restore_irqf(flags);

	uk_semaphore_up(&gnttab.sem);
}

static void gnttab_grant_init(grant_ref_t gref, domid_t domid,
		unsigned long mfn)
{
	gnttab.table[gref].frame = mfn;
	gnttab.table[gref].domid = domid;

	/* Memory barrier */
	wmb();
}

static void gnttab_grant_permit_access(grant_ref_t gref, domid_t domid,
		unsigned long mfn, int readonly)
{
	gnttab_grant_init(gref, domid, mfn);
	readonly *= GTF_readonly;
	gnttab.table[gref].flags = GTF_permit_access | readonly;
}

grant_ref_t gnttab_grant_access(domid_t domid, unsigned long mfn,
		int readonly)
{
	grant_ref_t gref = get_free_entry();

	gnttab_grant_permit_access(gref, domid, mfn, readonly);

	return gref;
}

grant_ref_t gnttab_grant_transfer(domid_t domid, unsigned long mfn)
{
	grant_ref_t gref = get_free_entry();

	gnttab_grant_init(gref, domid, mfn);
	gnttab.table[gref].flags = GTF_accept_transfer;

	return gref;
}

/* Reset flags to zero in order to stop using the grant */
static int gnttab_reset_flags(grant_ref_t gref)
{
	__u16 flags, nflags;
	__u16 *pflags;

	pflags = &gnttab.table[gref].flags;
	nflags = *pflags;

	do {
		if ((flags = nflags) & (GTF_reading | GTF_writing)) {
			uk_pr_warn("gref=%u still in use! (0x%x)\n",
				   gref, flags);
			return 0;
		}
	} while ((nflags = ukarch_compare_exchange_sync(pflags, flags, 0))
			!= flags);

	return 1;
}

int gnttab_update_grant(grant_ref_t gref,
		domid_t domid, unsigned long mfn,
		int readonly)
{
	int rc;

	UK_ASSERT(gref >= GNTTAB_NR_RESERVED_ENTRIES &&
		gref < NR_GRANT_ENTRIES);

	rc = gnttab_reset_flags(gref);
	if (!rc)
		return rc;

	gnttab_grant_permit_access(gref, domid, mfn, readonly);

	return 1;
}

int gnttab_end_access(grant_ref_t gref)
{
	int rc;

	UK_ASSERT(gref >= GNTTAB_NR_RESERVED_ENTRIES &&
		gref < NR_GRANT_ENTRIES);

	rc = gnttab_reset_flags(gref);
	if (!rc)
		return rc;

	put_free_entry(gref);

	return 1;
}

unsigned long gnttab_end_transfer(grant_ref_t gref)
{
	unsigned long frame;
	__u16 flags;
	__u16 *pflags;

	UK_ASSERT(gref >= GNTTAB_NR_RESERVED_ENTRIES &&
		gref < NR_GRANT_ENTRIES);

	pflags = &gnttab.table[gref].flags;
	while (!((flags = *pflags) & GTF_transfer_committed)) {
		if (ukarch_compare_exchange_sync(pflags, flags, 0) == flags) {
			uk_pr_info("Release unused transfer grant.\n");
			put_free_entry(gref);
			return 0;
		}
	}

	/* If a transfer is in progress then wait until it is completed. */
	while (!(flags & GTF_transfer_completed))
		flags = *pflags;

	/* Read the frame number /after/ reading completion status. */
	rmb();
	frame = gnttab.table[gref].frame;

	put_free_entry(gref);

	return frame;
}

grant_ref_t gnttab_alloc_and_grant(void **map, struct uk_alloc *a)
{
	void *page;
	unsigned long mfn;
	grant_ref_t gref;

	UK_ASSERT(map != NULL);
	UK_ASSERT(a != NULL);

	page = uk_palloc(a, 1);
	if (page == NULL)
		return GRANT_INVALID_REF;

	mfn = virt_to_mfn(page);
	gref = gnttab_grant_access(0, mfn, 0);

	*map = page;

	return gref;
}

static const char * const gnttabop_error_msgs[] = GNTTABOP_error_msgs;

const char *gnttabop_error(__s16 status)
{
	status = -status;
	if (status < 0 || (__u16) status >= ARRAY_SIZE(gnttabop_error_msgs))
		return "bad status";
	else
		return gnttabop_error_msgs[status];
}

void gnttab_init(void)
{
	grant_ref_t gref;

	UK_ASSERT(gnttab.initialized == 0);

	uk_semaphore_init(&gnttab.sem, 0);

#ifdef DBGGNT
	memset(gnttab.inuse, 1, sizeof(gnttab.inuse));
#endif
	for (gref = GNTTAB_NR_RESERVED_ENTRIES; gref < NR_GRANT_ENTRIES; gref++)
		put_free_entry(gref);

	gnttab.table = gnttab_arch_init(NR_GRANT_FRAMES);
	if (gnttab.table == NULL)
		UK_CRASH("Failed to initialize grant table\n");

	uk_pr_info("Grant table mapped at %p.\n", gnttab.table);

	gnttab.initialized = 1;
}

void gnttab_fini(void)
{
	struct gnttab_setup_table setup;
	int rc;

	setup.dom = DOMID_SELF;
	setup.nr_frames = 0;

	rc = HYPERVISOR_grant_table_op(GNTTABOP_setup_table, &setup, 1);
	if (rc) {
		uk_pr_err("Hypercall error: %d\n", rc);
		return;
	}

	gnttab.initialized = 0;
}
