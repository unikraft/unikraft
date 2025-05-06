# posix-process

libposix-process provides a POSIX-compliant process implementation allowing Unikraft to run unmodified applications written for *NIX systems.
It provides syscalls for task identification and lifetime management, as well as support for signals.

To accommodate different application requirements while preserving Unikraft's minimal and modular design, libposix-process offers three feature levels:

### Single thread
Bare minimum process-like behavior for single process applications that execute on a single thread.
Provides stubbed implementations of task identification facilities required by musl.
Although appearing as single-threaded, the process does not necessarily need to be backed by an underlying `uk_thread`, as it does not rely on thread state.
That is, this feature level is independent of [libuksched](https://github.com/unikraft/unikraft/tree/staging/lib/uksched).
Configuration:
  * Select `CONFIG_LIBPOSIX_PROCESS` without additional options.

### Multithreading
Single process with multithreading.
Provides true task identification and clone() support for threads.
Configuration:
  * Select `CONFIG_LIBPOSIX_PROCESS` and `CONFIG_LIBPOSIOX_PROCESS_MULTITHREADING`.
  * Optional: Select `CONFIG_LIBPOSIX_PROCESS_EXECVE` to enable `execve()` (requires VFS).
  * Optional: Select `CONFIG_LIBPOSIX_PROCESS_SIGNAL` to enable POSIX signals.

### Multiprocess:
Full-featured multiprocess support.
Configuration:
  * Select `CONFIG_LIBPOSIX_PROCESS` and `CONFIG_LIBPOSIOX_PROCESS_MULTIPROCESS`. Requires VFS.
  * Optional: Select `CONFIG_LIBPOSIX_PROCESS_SIGNAL` to enable POSIX signals.

A summary of the syscalls provided by libposix-process and their behavior on each configuration is shown below:

|                            | Single Thread     | Multithreading                   | Multiprocess |
| -------------------------- | ----------------- | -------------------------------- | ------------ |
| *gettid()*                 | Yes (stubbed)     | Yes                              | Yes          |
| *getpid()*                 | Yes (stubbed)     | Yes                              | Yes          |
| *getppid()*                | Yes (stubbed)     | Yes                              | Yes          |
| *clone()*                  | No (`ENOSYS`)     | Yes (`ENOTSUP` on `CLONE_VFORK`) | Yes          |
| *vfork()*                  | No (`ENOSYS`)     | No (`ENOSYS`)                    | Yes          |
| *execve()*                 | No (`ENOSYS`)     | Conditional [^2]                 | Yes          |
| *_exit()* / *exit_group()* | Conditional [^1]  | Yes                              | Yes          |
| *wait4()* / *waitid()*     | No (`ECHILD`)     | No (`ECHILD`)                    | Yes          |

[^1]: When `CONFIG_LIBUKBOOT_MAINTHREAD` is selected, the application is executed on a dedicated `uk_thread`.
In that case, single process single thread configurations can emulate `_exit()` and `exit_group()`, as any of these syscalls will result in cleanly terminating the underlying `uk_thread`.
If `CONFIG_LIBUKBOOT_MAINTHREAD` is not selected, the application executes on Unikraft's boot thread, which cannot be terminated.
Similarly if `CONFIG_LIBUKSCHED` is not selected, the application executes without being backed by an underlying `uk_thread`.
For these reasons `_exit()` / `exit_group()` cannot be meaningfully implemented under such configurations.
Since these syscalls never return, and thus cannot signal failure to their caller, calling them is an unrecoverable runtime error, on which Unikraft will panic with an appropriate error message.

[^2]: Calling `execve()` without multiprocessing support is possible, and results into the calling process' context being replaced with that of the executed program.
Because `execve()` is passed a filesystem path, `CONFIG_LIBPOSIX_PROCESS_EXECVE` depends on VFS.
To avoid unnecessary bloat on appliations that may not use `execve()`, `CONFIG_LIBPOSIX_PROCESS_EXECVE` is provided as an optional selection in multithreaded configurations.

## Requirements of execve()

As stated earlier `execve()` depends on VFS.
Loading and execution is handled by [libukbinfmt](https://github.com/unikraft/unikraft/tree/staging/lib/ukbinfmt), which requires a binfmt handler for the corresponding binary format.
The binfmt handler for ELF is provided by [app-elfloader](https://github.com/unikraft/app-elfloader).
Additionally, since Unikraft executes on a single address space, it is required that executables passed to `execve()` must be compiled as PIE.

## Notes on multiprocess

Being a single-address-space operating system, instead of the traditional `fork()` that creates a new address space for the child, Unikraft implements multiprocess with `vfork()` only.
That includes the `vfork(2)` syscall, the equivalent of calling `clone()` with flags set to `CLONE_VM | CLONE_VFORK | SIGCHLD`.
When calling `vfork()`, the child shares the same address space as the parent, and the parent's execution is suspended until the child either calls `execve()`, or terminates by calling `_exit()` / receiving a terminating signal.
This introduces limitations on what the child can do until it calls `execve()` (or terminates), as it must ensure that it doesn't leave the parent's memory in an incosistent state.
Specifically, according to [vfork(2)](https://man7.org/linux/man-pages/man2/vfork.2.html), the only operations the child can do are: (1) update the memory location that stores return value of `vfork()`, (2) call `execve()`, (3) call `_exit()`.
In any other case the resulting behavior is undefined.
Fortunately it is common that appliations spawn new processes by calling `fork()` immediately followed by an `execve()`, so running a multiprocess application on Unikraft that does not natively support `vfork()` should be possible with minimal changes.

> [!TIP]
> Applications can use the `posix_spawn()` libc function that spawns a process using `vfork()` in a safe way.

Another important aspect of multiprocess systems is the existence of an init process.
Unikraft implements `/sbin/init`-like logic as part of [libukboot](https://github.com/unikraft/unikraft/tree/staging/lib/ukboot), and is enabled by selecting `CONFIG_LIBUKBOOT_INIT`.
The implementation is minimal; essentially it spawns a new process for the application, fosters orphaned children, reaps zombies, and coordinates system shutdown.
In most cases, an init process is necessary on multiprocess configurations, yet `CONFIG_LIBPOSIX_PROCESS_MULTIPROCESS` does not enforce the selection of `CONFIG_LIBUKBOOT_INIT` by default, to give applications the possibility to override the default implementation, or operate without an init.
The latter is possible if the application does not exit early leaving daemonized processes behind, and does not require graceful shutdown (described below).
In conclusion, multiprocess applications should therefore either (1) select `CONFIG_LIBUKBOOT_INIT` (2) provide their own implementation of init, or (3) make sure that they don't need one.

## System Shutdown

From Unikraft 0.16.0, libukboot supports asynchronously triggered shutdown.
At the most fundamental level, when the system receives a shutdown request, the application must be terminated before proceeding to terminate system-wide components like the network and filesystem.
When libposix-process is selected, all processes must be terminated and their resources must be released.
That requires additional handling at the level of libposix-process.

The way libposix-process implements shutdown at the process level depends on the system configuration.
If signals are enabled, the *application process*[^4] is sent `SIGTERM` and shutdown blocks until the application process terminates.
Once the application process terminates, any remaining processes are terminated forcefully.
If signals are not enabled, all processes are terminated forcefully.
In both cases the result is the same: all process resources are directly released.

In multiprocess configurations, we have the option to send a shutdown signal to the init process, who can in turn signal remaining processes and wait for a predetermined amount of time for them to terminate gracefully.
This is referred to as *graceful shutdown*, and is implemented as part of the `/sbin/init` logic provided by `libukboot`.
If the timeout is reached and there are processes still running, the init process returns and any surviving processes are terminated depending on the system configuration as described above.

The table below summarizes process termination during shutdown for the possible configurations of libposix-process:

|                           | Single Thread     | Multithreading | Multiprocess |
| ------------------------- | ----------------- | -------------- | ------------ |
| **Asynchronous Shutdown** | Yes[^3]           | Yes [^4]       | Yes [^4]     |
| **Graceful Shutdown**     | No                | No [^5]        | Yes [^6]     |

[^3]: The application thread is terminated and the system proceeds to shutdown.
Notice that asynchronous shutdown depends on `CONFIG_LIBUKBOOT_MAINTHREAD`, which makes it possible to return the application's exit status.

[^4]: If signals are enabled, libposix-process issues `SIGTERM` and blocks until the *application process* terminates.
On multithreaded configurations that is the only process.
On multiprocess with an implementation of init, that is the init process.
On multiprocess without init, that is the process that calls `main()`.
Once the application process terminates, the system proceeds to forcefully terminate any remaining processes by directly releasing their resources.

[^5]: Graceful termination, however, can still be implemented by the application.
From the system's perspective the termination path is the same one followed during asynchronous shutdown[^4].

[^6]: Enabled by selecting `CONFIG_LIBUKBOOT_GRACEFUL_SHUTDOWN` in libukboot.
Both the termination signal and the timeout are configurable.
