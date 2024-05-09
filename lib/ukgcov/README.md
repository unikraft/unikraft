# Unikraft Library for Profiling and Test Coverage

This library introduces initial setup with config options for console, binary file, and memory output. It takes advantage of `GCC's`` Profiling and Test Coverage in Freestanding Environments.

## Prerequisites

This feature was introduced in `GCC-13`, as such, this library won't work with any version lower than that.
One easy way to build with a specific `GCC` version is to use a `Docker` container - [DockerImage](https://hub.docker.com/_/gcc/)

## Configuration

To utilize this library, you will need to select and configure some options:

- Select `ukgcov` from `Library Configuration` KConfig menu.
- Choose the way you want to extract the `gcov` information from the kernel.

### File System Configuration for extracting via File Output

- In `lib` -> `vfscore`, select:
  - `Automatically mount a root filesystem (/)`
  - `Default root filesystem (9PFS)`

- Create a directory `fs0` where the resulting filename with the name provided in `KConfig` will reside.

## Usage

To extract the coverage information, you can call the `ukgcov_dump_info()` function at the end of the program. Various output methods are provided:

### 1. Console Output

*Avoid using the `-nographic` option as it clashes with the redirection of standard output.*

Example:

```bash
qemu-system -chardev stdio,id=char0,logfile=serial.log,signal=off -serial chardev:char0
```

### 2. File Output

Configure the file system as detailed above, and the results will reside in the directory `fs0`.

### 3. Memory Output

Use GDB and issue the commands to dump the coverage data into a binary file named `memory.bin`.

Example:

```bash
dump memory memdump.bin gcov_output_buffer gcov_output_buffer+gcov_output_buffer_pos
```

## Creating Coverage Report

Process the output using `ukgcov`'s `gcov_process.sh` script.
Install dependencies `dos2unix` and `lcov`, then invoke `gcov_process.sh` with parameters depending on the output method selected.

*Notice:*
In order for the processing script to work, lcov must come from the toolchain used for compiling. If you are using a docker container to build with GCC-13, make sure to run gov_process.sh from within the container.

Examples:

**For Console Output:**

```bash
./support/scripts/ukgcov/gcov_process.sh -o <result_dir>  -c <console_output> <build_directory>
```

**For Binary File:**

```bash
./support/scripts/ukgcov/gcov_process.sh -o <result_dir> -b <binary_output> <build_directory>
```
