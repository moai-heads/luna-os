# POSIX-first plan

## Goal

Build a small, useful POSIX-like user environment for Luna before attempting a large application port. The first target is not full POSIX conformance; it is a documented subset that can run a shell and basic utilities on the existing x86-64 kernel.

The stable base will be:

- ELF64 little-endian x86-64 executables;
- the normal x86-64 System V user ABI;
- a small, explicitly versioned Luna syscall ABI; and
- POSIX/Linux-like semantics where they reduce application porting work.

BusyBox is now a **future validation target**, not a dependency or immediate milestone. A POSIX-first layer lets us test functionality with tiny programs before dealing with BusyBox's large Linux and libc surface.

## Layering

```text
POSIX applications and utilities
        -> luna-libc / startup files
        -> versioned Luna syscall ABI
        -> processes, VFS, TTY, signals, VM, timers
        -> x86-64 kernel and device drivers
```

Keep the boundaries strict:

- kernel code implements resources and security;
- libc translates the user-facing POSIX API into syscalls;
- utilities exercise the API but do not define it; and
- every intentional deviation from POSIX gets a test and a short ABI note.

## Implementation order

### 1. Safe execution substrate

Before user code can run, implement:

- GDT/TSS and IDT;
- exception handlers with register dumps;
- local APIC/IOAPIC setup and a timer;
- physical-page allocation and kernel heap;
- page-table creation, user/kernel permission separation, and a higher-half kernel;
- ring-3 entry/return and a per-process kernel stack; and
- ELF64 `PT_LOAD` mapping with W^X permissions.

Acceptance: a statically linked user program can enter ring 3, write a result, and exit without corrupting the kernel.

### 2. Small syscall core

Start with the smallest useful process/file API:

- `read`, `write`, `close`;
- `openat`, `lseek`, `fstat`, `getdents64`;
- `mmap`, `munmap`, `brk`;
- `exit`, `exit_group`, `getpid`, `getppid`;
- `execve`, `wait4`;
- `fork` or `vfork`, then `clone` only when needed;
- `dup`, `dup2`, `dup3`, `pipe2`;
- `clock_gettime`, `nanosleep`, `uname`; and
- `poll`.

Return negative, documented errors in the kernel and translate them to `errno` in libc. Keep syscall arguments fixed-width and pointer validation centralized.

Acceptance: tiny `hello`, `cat`, and `fork-exec-wait` programs pass under QEMU.

### 3. VFS, initramfs, and TTY

Implement:

- a VFS with regular files, directories, metadata, permissions, and symlinks;
- an initramfs reader and a read-only root filesystem;
- `/dev/console`, `/dev/null`, `/dev/zero`, and `/dev/tty`;
- a TTY line discipline with canonical/raw mode, echo, Ctrl-C, and termios basics;
- keyboard/mouse events routed to a character/TTY service; and
- serial console support for headless testing.

Acceptance: a user process can read keyboard input, write the framebuffer/serial terminal, use redirection and pipelines, and receive Ctrl-C.

### 4. Minimal libc and utilities

Build `luna-libc` with:

- process startup/termination (`crt1`, `_start`, `exit`);
- syscall wrappers and `errno`;
- strings and memory;
- `malloc`/`free` on `mmap` or `brk`;
- stdio and file streams;
- environment handling;
- directory and stat APIs;
- signals and basic termios; and
- a small `printf` implementation.

Start with small utilities rather than a monolithic port:

```text
init, sh, echo, printf, true, false,
cat, pwd, env, ls, mkdir, rm, cp, mv,
grep, uname, id, kill, sleep
```

The first shell may be a deliberately small POSIX shell. It must support commands, arguments, environment variables, pipelines, redirection, background jobs, and child reaping before adding advanced syntax.

Acceptance: `/init` starts the shell and the utility suite runs from a clean initramfs.

### 5. Persistent storage and system interfaces

After the initramfs path is stable:

- add virtio-blk for QEMU;
- add a read/write FAT32 or ext2 filesystem;
- add mount/unmount and a block cache;
- add `/proc` for process inspection;
- add `/sys` only for interfaces we can maintain; and
- add permissions, users/groups, and job control incrementally.

Acceptance: files survive reboot, `ps`/`dmesg`-style tools can inspect the system, and an unprivileged process cannot write kernel memory or another process's files without permission.

### 6. Optional compatibility expansion

Only after the small POSIX environment is reliable:

- sockets and IPv4 networking;
- pthreads/TLS;
- dynamic ELF and a dynamic linker;
- locale and wide-character support;
- extended attributes, ACLs, and richer signals; and
- BusyBox as a broad integration test.

## Initial syscall contract

The first ABI should cover these families:

| Family | Initial operations | Consumer |
|---|---|---|
| Process | `execve`, `exit`, `wait4`, `getpid`, `fork`/`vfork` | shell, init |
| Descriptors | `read`, `write`, `close`, `dup2`, `fcntl`, `pipe2` | every utility |
| Files | `openat`, `lseek`, `fstatat`, `getdents64`, `mkdirat`, `unlinkat`, `renameat`, `readlinkat` | `ls`, `cp`, `rm`, shell |
| Memory | `mmap`, `munmap`, `mprotect`, `brk` | libc allocator, ELF programs |
| TTY | `ioctl`, `poll`, `isatty`-supporting operations | shell, `stty`, interactive apps |
| Signals | `rt_sigaction`, `rt_sigprocmask`, `kill`, `rt_sigreturn` | Ctrl-C, job control |
| Time | `clock_gettime`, `nanosleep` | `sleep`, timeouts, libc |
| Identity | `uname`, `getuid`, `getgid`, `setpgid` | shell and diagnostics |

The names are Linux-like where practical, but the exact numbers, structures, flags, alignment, and error behavior belong to Luna's versioned ABI. Do not silently copy a Linux ABI detail without either implementing its semantics or documenting the deviation.

## Userland tree

```text
user/
  include/               installed UAPI and libc headers
  crt/                   process startup and termination objects
  libc/                  syscall wrappers and POSIX subset
  tests/                 one focused test per kernel/libc feature
  bin/                   small utilities
  init/                  first-process code
  rootfs/                initramfs contents
```

Keep the kernel build and userland build separate. Kernel objects remain freestanding; user programs use the documented user ABI and a sysroot.

## Testing strategy

- Boot every change from a clean initramfs in QEMU.
- Test syscall success, invalid pointers, bad descriptors, short reads/writes, and interruption by signals.
- Test process isolation, `execve` argument/environment setup, child reaping, and file-descriptor inheritance.
- Test canonical/raw TTY mode, echo, Ctrl-C, Ctrl-D, pipes, redirection, and background jobs.
- Test filesystem traversal, metadata, permissions, symlinks, rename, unlink, and power-loss-safe write ordering where applicable.
- Keep a small POSIX conformance checklist in `user/tests/`; do not call a feature complete until it has a regression test.

## Definition of “basic POSIX” for Luna

The initial definition is: a user process can start from ELF, allocate memory, access files and directories, communicate through descriptors and pipes, use a TTY, receive basic signals, query time/identity, launch/reap children, and run a small shell plus utilities.

That scope is intentionally smaller than full POSIX. It is large enough to make Luna usable and to expose the kernel interfaces that mature software will eventually need.
