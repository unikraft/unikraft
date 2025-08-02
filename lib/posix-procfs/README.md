# The procfs filesystem

Currently the library offers support for 5 files:


| Path              | Description |
| -----------       | ----------- |
| `/proc/meminfo` | Returns the current memory usage. |
| `/proc/version` | Returns the current version of Unikraft.  |
| `/proc/cmdline` | Return the arguments receive by the kernel. |
| `/proc/filesystems`| Return the currently mounted filesystems.|
| `/proc/mounts`   | Return the currently mounted filesystems and their mount points.|

## Prerequisites:
In order to use the library one must include vfscore and ukstore.
Furthermore in order to mount proc at /proc one must mount a root
file system.

Of the implemented files the first two have no further prerequisites,
while the latter three require libnewlib for text formatting.

## Implementation:
The library creates the information on demand, when reading from the
files. This is done with the help of static getters from the ukstore 
library.
The filesystem is mounted at /proc, while the containing files are 
linked to the interior root (/).
In order to know which files are read and return the proper
information we parse the input in the read function.

## Adding new entries into procfs
In order to further expand the filesystem and add more entries there
are a few steps necessary for the current implementation:
- after mounting the procfs root file, create a new entry and link it to the last created file (which can be the `/` or any other file)

    ```C
    create_entry(<mounting_point>, <entries_vector>, <file_name>,   
                 <file_type>, <existing_files_no>);
    ```
- in the `procfs_read()` function identify the proc file and create the buffer with the needed information
  ```C
  if (strcmp(fp->f_dentry->d_path, "/meminfo") == 0) {
		return_me = read_meminfo();
		return vfscore_uiomove(return_me, strlen(return_me) + 1, uio);
	}
  ```
## Testing 
For example, you can read the entries in procfs with:
```C
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
	char buf[200];
	int fd;
	fd = open("/proc/meminfo", O_RDONLY);
	if (fd > 0) {
		printf("/proc/meminfo:\n");
		read(fd, buf, 200);
		printf("%s", buf);
	}
```