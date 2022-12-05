#include <uk/test.h>
#include <uk/fdtab/fd.h>
#include <string.h>
#include <unistd.h>

static int test_fdops_free_ctr;
static int test_fdops_free(struct fdtab_file *fp __unused)
{
	test_fdops_free_ctr++;
	return 0;
}

static struct fdops test_fdops = {
	.fdop_free = test_fdops_free,
};

UK_TESTCASE(posix_fdtab_fdtab_testsuite, fdtab_dup2)
{
	struct uk_alloc *a = uk_alloc_get_default();
	struct fdtab_table *tab, *prev;
	struct fdtab_file f1, *f2;
	int rc, fd1, fd2;

	test_fdops_free_ctr = 0;

	tab = fdtab_alloc(a);
	UK_TEST_EXPECT_NOT_NULL(tab);
	if (tab == NULL)
		return;

	prev = fdtab_get_active();
	fdtab_set_active(tab);

	/* Create a new file */
	memset(&f1, 0, sizeof(struct fdtab_file));
	fdtab_file_init(&f1);
	f1.f_op = &test_fdops;

	/* Allocate a new file descriptor number for the file */
	fd1 = fdtab_alloc_fd(tab);
	UK_TEST_EXPECT_SNUM_GE(fd1, 0);
	if (fd1 < 0)
		goto exit_free_tab;

	/* Install the file at the file descriptor, but keep a reference for
	 * ourselves to register it a second time at a different fd number
	 */
	rc = fdtab_install_fd(tab, fd1, &f1);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto exit_free_tab;

	/* Duplicate the descriptor */
	UK_TEST_EXPECT_SNUM_EQ(test_fdops_free_ctr, 0);
	fd2 = fd1 + 1;
	rc = dup2(fd1, fd2);
	UK_TEST_EXPECT_SNUM_EQ(fd2, rc);
	UK_TEST_EXPECT_SNUM_EQ(test_fdops_free_ctr, 0);

	/* Fetch the file from the duplicated descriptor */
	f2 = fdtab_get_file(tab, fd2);
	UK_TEST_EXPECT_NOT_NULL(f2);
	if (f2 == NULL)
		goto exit_free_tab;
	UK_TEST_EXPECT_PTR_EQ(&f1, f2);

	/* And put it back */
	fdtab_put_file(f2);
	f2 = NULL;
	UK_TEST_EXPECT_SNUM_EQ(test_fdops_free_ctr, 0);

	/* Remove the first file descriptor number */
	rc = fdtab_put_fd(tab, fd1);
	UK_TEST_EXPECT_SNUM_EQ(rc, 0);
	UK_TEST_EXPECT_SNUM_EQ(test_fdops_free_ctr, 0);

	/* Remove the second one. Also that should free the file description */
	rc = fdtab_put_fd(tab, fd2);
	UK_TEST_EXPECT_SNUM_EQ(rc, 0);
	UK_TEST_EXPECT_SNUM_EQ(test_fdops_free_ctr, 1);

exit_free_tab:
	fdtab_set_active(prev);
	fdtab_clear(tab);
	uk_free(a, tab);
}

UK_TESTCASE(posix_fdtab_fdtab_testsuite, fdtab_clone)
{
	struct uk_alloc *a = uk_alloc_get_default();
	struct fdtab_table *tab1, *tab2 = NULL, *prev;
	struct fdtab_file f1, *f2;
	int r, fd1;

	test_fdops_free_ctr = 0;

	tab1 = fdtab_alloc(a);
	UK_TEST_EXPECT_NOT_NULL(tab1);
	if (tab1 == NULL)
		return;

	prev = fdtab_get_active();
	fdtab_set_active(tab1);

	/* Create a new file */
	memset(&f1, 0, sizeof(struct fdtab_file));
	fdtab_file_init(&f1);
	f1.f_op = &test_fdops;

	/* Install the file */
	r = fdtab_fdalloc(&f1, &fd1);
	UK_TEST_EXPECT_ZERO(r);
	UK_TEST_EXPECT_SNUM_GE(fd1, 0);
	if (r != 0 || fd1 < 0)
		goto exit_free_tab;
	fdtab_fdrop(&f1);

	/* Duplicate the file descriptor table */
	tab2 = fdtab_clone(a, tab1);
	UK_TEST_EXPECT_NOT_NULL(tab2);
	if (tab2 == NULL)
		goto exit_free_tab;

	/* And check whether our file description is there */
	f2 = fdtab_get_file(tab2, fd1);
	UK_TEST_EXPECT_NOT_NULL(f2);
	if (f2 == NULL)
		goto exit_free_tab;
	fdtab_fdrop(f2);
	UK_TEST_EXPECT_ZERO(test_fdops_free_ctr);

	/* Clear the first table */
	fdtab_clear(tab1);

	/* Nothing should have been removed so far... */
	UK_TEST_EXPECT_ZERO(test_fdops_free_ctr);

	/* ... only after clearing the second table */
	fdtab_clear(tab2);
	UK_TEST_EXPECT_SNUM_EQ(test_fdops_free_ctr, 1);

exit_free_tab:
	fdtab_set_active(prev);

	if (tab2) {
		fdtab_clear(tab2);
		uk_free(a, tab2);
	}

	fdtab_clear(tab1);
	uk_free(a, tab1);
}

uk_testsuite_register(posix_fdtab_fdtab_testsuite, NULL);
