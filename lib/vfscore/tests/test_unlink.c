#include <uk/test.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <sys/stat.h>

#define dir "/foo"
#define file_path "/foo/file.txt"
#define test_msg "hello\n"

static int fd;
static int ret;

UK_TESTCASE(vfscore_unlink_testsuite, unlink_closed_file)
{
	ret = mount("", "/", "ramfs", 0, NULL);
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);

	ret = mkdir(dir, S_IRWXU);
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);

	fd = open(file_path, O_WRONLY | O_CREAT, S_IRWXU);
	UK_TEST_EXPECT_SNUM_GT(fd, 2);

	fd = close(fd);
	UK_TEST_EXPECT_SNUM_EQ(fd, 0);

	ret = unlink(file_path);
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);
}

UK_TESTCASE(vfscore_unlink_testsuite, unlink_then_write)
{
	fd = open(file_path, O_WRONLY | O_CREAT, S_IRWXU);
	UK_TEST_EXPECT_SNUM_GT(fd, 2);

	ret = unlink(file_path);
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);

	ret = unlink(file_path);
	UK_TEST_EXPECT_SNUM_EQ(ret, -1);

	UK_TEST_EXPECT_SNUM_EQ(
		write(fd, test_msg, sizeof(test_msg)),
		sizeof(test_msg)
	);

	fsync(fd);

	ret = close(fd);
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);

	UK_TEST_EXPECT_SNUM_EQ(
		write(fd, test_msg, sizeof(test_msg)),
		-1
	);
}

uk_testsuite_register(vfscore_unlink_testsuite, NULL);
