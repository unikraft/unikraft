#include <uk/config.h>

#include <stddef.h>
#include <stdio.h>
#include <errno.h>

#include <uk/plat/lcpu.h>
#include <uk/plat/irq.h>
#include <uk/plat/bootstrap.h>
#include <uk/plat/memory.h>
#include <uk/essentials.h>
#include <uk/print.h>
#include <uk/ctors.h>
#include <uk/init.h>
#include <uk/argparse.h>
#include "banner.h"

int main(int argc, char *argv[]) __weak;

void ukplat_entry_argp(char *arg0, char *argb, __sz argb_len)
{
	static char *argv[CONFIG_LIBUKMINBOOT_MAXNBARGS];
	int argc = 0;

	if (arg0) {
		argv[0] = arg0;
		argc += 1;
	}
	if (argb && argb_len) {
		argc += uk_argnparse(argb, argb_len, arg0 ? &argv[1] : &argv[0],
				     arg0 ? (CONFIG_LIBUKMINBOOT_MAXNBARGS - 1)
					  : CONFIG_LIBUKMINBOOT_MAXNBARGS);
	}
	ukplat_entry(argc, argv);
}

void ukplat_entry(int argc, char *argv[])
{
	int kern_args = 0;
	int rc = 0;

	uk_ctor_func_t *ctorfn;
	uk_init_func_t *initfn;
	int i;

    /* Call the constructors */
	uk_ctortab_foreach(ctorfn, uk_ctortab_start, uk_ctortab_end) {
		UK_ASSERT(*ctorfn);
		(*ctorfn)();
	}

	argc -= kern_args;
	argv = &argv[kern_args];

    /* Call init functions */
	uk_inittab_foreach(initfn, uk_inittab_start, uk_inittab_end) {
		UK_ASSERT(*initfn);
		rc = (*initfn)();
		if (rc < 0) {
			uk_pr_err("Init function at %p returned error %d\n",
				  *initfn, rc);
			rc = UKPLAT_CRASH;
			goto exit;
		}
	}

    /**
     * Enable interrupts. This may be removed and a stub could be called
     * in its place. Remains to be seen.
     */
    ukplat_lcpu_enable_irq();

	/* Run init table */
	uk_inittab_foreach(initfn, uk_inittab_start, uk_inittab_end) {
		UK_ASSERT(*initfn);
		rc = (*initfn)();
		if (rc < 0) {
			uk_pr_err("Init function at %p returned error %d\n", *initfn, rc);
			rc = UKPLAT_CRASH;
			goto exit;
		}
	}

    /* Display the banner */
	print_banner(stdout);
	fflush(stdout);

    /* Call pre-init constructors */
	uk_ctortab_foreach(ctorfn, __preinit_array_start, __preinit_array_end) {
		if (!*ctorfn)
            continue;
		(*ctorfn)();
	}

    /* Call init constructors */
	uk_ctortab_foreach(ctorfn, __init_array_start, __init_array_end) {
		if (!*ctorfn)
            continue;
		(*ctorfn)();
	}

	uk_pr_info("Calling main(%d, [", argc);
	for (i = 0; i < argc; ++i) {
		uk_pr_info("'%s'", argv[i]);
		if ((i + 1) < argc)
			uk_pr_info(", ");
	}
	uk_pr_info("])\n");

    /* Call main */
	rc = main(argc, argv);
	rc = (rc != 0) ? UKPLAT_CRASH : UKPLAT_HALT;

exit:
	ukplat_terminate(rc); /* does not return */
}
