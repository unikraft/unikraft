# hostfs — host-mediated filesystem for Hyperlight guests

`lib/hostfs` mounts as a standard Unikraft `vfscore` filesystem that
forwards every POSIX operation to the Hyperlight host via the
`__dispatch` host function call. Path resolution and sandboxing live
host-side: the host only serves files under the directory passed to
`hyperlight-unikraft --mount <dir>`, and rejects any attempt to escape
it (via `..` or symlinks).

This completes Phase B of the sandboxed-filesystem design. Phase A
exposed explicit `fs_read` / `fs_write` / `fs_list` / `fs_stat` /
`fs_mkdir` / `fs_unlink` RPCs that the guest called directly via
`/dev/hcall`; Phase B lets unmodified POSIX code hit those same
handlers transparently.

## Configuration

```
CONFIG_LIBHOSTFS=y                  # enable the driver
CONFIG_LIBHOSTFS_AUTOMOUNT=y        # auto-mount at boot (else call mount() yourself)
CONFIG_LIBHOSTFS_MOUNTPOINT="/host" # default mount point
```

Requires `PLAT_HYPERLIGHT` and `LIBVFSCORE`; auto-selects
`HYPERLIGHT_HCALL`.

## Architecture

```
guest POSIX (open/read/write/stat/mkdir/…)
        ↓
vfscore — VOP_OPEN, VOP_READ, VOP_WRITE, …
        ↓
hostfs_{open,read,write,stat,mkdir,…}   (this library)
        ↓
hostfs_rpc_*            (JSON builders + response parsers)
        ↓
hyperlight_hcall()      (plat/hyperlight)
        ↓
outb VM-exit → Hyperlight host
        ↓
FsSandbox::fs_*_bytes handlers        (hyperlight-unikraft host)
        ↓
host filesystem (scoped to --mount root)
```

Each vnode carries a mount-relative path (`struct hostfs_node::path`).
Lookup issues `fs_stat`, allocates a fresh vnode via `vfscore_vget`
keyed by djb2(path) for a stable inode number, and records the child
path. Read/write split large transfers into `HOSTFS_CHUNK`-sized RPCs.
Binary payloads ride in base64-encoded JSON; for files up to about
32 KiB the overhead is a single `fs_write_bytes` or `fs_read_bytes`
call per `write()` / `read()`.

## Supported vnops

| vnop        | status      |
|-------------|-------------|
| open        | no-op       |
| close       | no-op       |
| read        | `fs_read_bytes` (chunked) |
| write       | `fs_write_bytes` (chunked, honours `IO_APPEND`) |
| seek        | no-op (offset lives in the file struct) |
| lookup      | `fs_stat`   |
| create      | zero-byte `fs_write_bytes` |
| remove      | `fs_unlink` |
| mkdir       | `fs_mkdir`  |
| rmdir       | `fs_unlink` |
| readdir     | `fs_list`   |
| getattr     | `fs_stat`   |
| setattr     | no-op       |
| truncate    | `fs_truncate` |
| inactive    | frees the `hostfs_node` |
| ioctl / fsync / link / rename / symlink / readlink / fallocate / poll | `ENOTSUP` / no-op |

## Known limitations

- **`opendir` crashes the guest.** `open("/host", O_RDONLY)` and
  `stat("/host", …)` both succeed on their own, but `opendir` —
  which opens the directory, calls `fstat`, and wraps the fd in a
  `DIR` struct — triggers a kernel fault somewhere between the `open`
  and the `close` that vfscore issues. Reproduces on the hostfs root
  only; plain file open/read/write/stat are unaffected. Needs more
  investigation (possibly the libc `DIR` allocation or an
  interaction with `sys_getdents64`).
- **Cross-mount path walks are fragile.** Lookups that cross from
  hostfs into the guest ramfs (e.g. `/host/../etc/passwd`) can crash
  during path traversal. Sandbox enforcement for in-mount paths still
  works — `fs_stat("../..")` and similar get rejected host-side.
- **Single-threaded.** `hostfs_rpc_*` share static request/response
  buffers. Add per-thread buffers (or a mutex) before running the
  filesystem concurrently.
- **No timestamps or ownership.** `va_mtime` / `va_uid` / `va_gid` are
  all zero; the host handlers don't report them yet.
