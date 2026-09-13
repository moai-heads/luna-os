# Deferred BusyBox port plan

> This document is retained as a future integration target. The active direction is the POSIX-first plan in [POSIX_PLAN.md](POSIX_PLAN.md).

## Objective

Boot a statically linked x86-64 BusyBox binary as `/bin/busybox`, run it as `/init`, and reach an interactive `ash` shell on a Luna TTY. Start with a small, testable applet set and expand the ABI only when an applet needs it.

ELF64 is necessary for this, but it is not sufficient: BusyBox expects a C library, Linux/POSIX process semantics, file descriptors, `ioctl`/termios, signals, and a useful filesystem/device model. The compatibility target should therefore be **a deliberate Linux-like user ABI subset**, not “ELF compatibility” alone.

## Recommended strategy

1. **Keep kernel-specific code out of BusyBox.** Implement a Luna syscall veneer and a small libc (`luna-libc`) instead of adding Luna branches throughout every applet.
2. **Build static first.** Defer the dynamic linker, shared libraries, locale, NSS, and threads until the basic shell works.
3. **Use an initramfs first.** It avoids blocking userland work on a disk driver and filesystem writer.
4. **Pin BusyBox to a source commit** when implementation starts, record its license/version in `ports/busybox/README.md`, and keep any compatibility patches small and reviewable.
5. **Prefer QEMU/virtio during bring-up.** Add physical storage/network drivers after the user ABI is stable.

## Kernel prerequisites

### Process and syscall layer

- Ring-3 entry/return path and per-process address spaces.
- `execve`, `exit`/`exit_group`, `wait4`, and a scheduler.
- `fork` or `clone` plus `vfork` semantics sufficient for `ash`.
- File-descriptor tables, `dup2`/`dup3`, `fcntl`, close-on-exec, and pipes.
- `errno` propagation and a stable syscall numbering/ABI document.
- Signals: at minimum `SIGCHLD`, `SIGINT`, `SIGTERM`, `SIGPIPE`, `SIGKILL`, and default/ignored/handler dispositions.

### Memory and executable loading

- `mmap`, `munmap`, `mprotect`, and `brk` or an equivalent libc heap primitive.
- ELF64 `PT_LOAD` mapping with zero-filled BSS and W^X permissions.
- User stack, arguments, environment, and auxiliary-vector policy.
- Optional initially: ASLR, dynamic ELF, TLS, and copy-on-write fork.

### Filesystem and devices

- VFS path lookup, regular files, directories, metadata, permissions, and symlinks.
- `openat`, `read`, `write`, `pread64`, `pwrite64`, `lseek`, `stat`/`fstat`/`fstatat`, `getdents64`, `mkdirat`, `unlinkat`, `renameat`, and `readlinkat`.
- Initramfs reader; FAT32 or ext2 can follow for persistent storage.
- `/dev/console`, `/dev/null`, `/dev/zero`, `/dev/tty`, and a basic devfs.
- TTY line discipline and termios ioctls for interactive `ash`.
- Optional early pseudo-filesystems: `/proc` for `ps`/`top`, `/sys` for hardware inspection, and `/tmp` as tmpfs.

### Time, identity, and terminal

- `clock_gettime`, `nanosleep`, `gettimeofday`, and a monotonic timer.
- `uname`, `getpid`, `getppid`, `getuid`, `geteuid`, `getgid`, `getegid`, and basic `set*id` policy.
- `poll` or `select` for shell input and later networking.
- `ioctl` for TTYs and device files.

### Networking, later

- `socket`, `bind`, `listen`, `accept`, `connect`, `sendto`, `recvfrom`, `getsockopt`, `setsockopt`, `shutdown`, and `poll`.
- A QEMU virtio-net driver, loopback, IPv4, UDP, TCP, DNS, and a minimal route/configuration interface.

## Userland/toolchain plan

Create a port subtree like this:

```text
ports/busybox/
  README.md              pinned source and local patches
  busybox.config         minimal static configuration
  build.sh               reproducible cross-build
  patches/               only Luna-specific fixes
user/
  libc/                  luna-libc syscall wrappers and POSIX subset
  crt/                   crt1, crti, crtn, startup/exit glue
  include/               installed user headers
  lib/                   libc and optional libgcc support
  rootfs/
    init                 /bin/busybox init script or symlink
    bin/
    dev/
    etc/
    proc/
    sys/
    tmp/
```

Toolchain sequence:

1. Build or configure an `x86_64-luna` binutils/GCC target.
2. Install kernel/user headers into a sysroot.
3. Build `crt1` and the first `luna-libc` syscall wrappers.
4. Link a tiny test program (`write`, `getpid`, `mmap`, `fork`, `execve`, `wait4`).
5. Build static BusyBox with `--sysroot`, `-static`, and no unsupported applets.
6. Package BusyBox and the initramfs into a Limine module or disk image.

Kernel-only flags such as `-mno-red-zone` should not automatically be applied to user programs. Userland should use the normal x86-64 System V ABI unless Luna intentionally documents a deviation.

## Applet rollout

### Phase A: prove the shell and file descriptors

Enable:

- `ash`
- `echo`, `printf`, `true`, `false`
- `cat`, `head`, `tail`, `grep`
- `pwd`, `env`, `export`, `unset`
- `ls`, `stat`, `mkdir`, `rm`, `cp`, `mv`
- `uname`, `id`, `kill`

Acceptance: `/init` mounts/initializes the console, starts `ash`, accepts keyboard input, runs pipelines, redirects files, and reaps child processes.

### Phase B: system administration

Add:

- `mount`, `umount`, `df`, `find`, `dmesg`, `ps`
- `/proc`, `/sys`, `/dev` support
- permissions, users/groups, signals, job control, and `stty`

Acceptance: the system can inspect processes/devices, mount a persistent filesystem, and recover from a crashed child process.

### Phase C: networking and remote access

Add:

- `ip`/`ifconfig` subset, `route`, `ping`, `nc`
- `wget` or a small HTTP client
- DHCP, DNS, and a serial-console fallback

Acceptance: a static BusyBox shell can configure QEMU networking and fetch a file.

### Phase D: broaden compatibility

Only after the previous stages are stable:

- threads/TLS and selected pthread APIs;
- dynamic ELF and a dynamic linker;
- user/group databases and locale;
- compression, archive, init, logging, and rescue applets;
- broader Linux syscall and ioctl compatibility.

## Initial root filesystem

```text
/init -> /bin/busybox
/bin/busybox
/bin/sh -> busybox
/bin/ls -> busybox
/bin/cat -> busybox
/dev/console
/dev/null
/dev/zero
/dev/tty
/etc/profile
/proc/
/sys/
/tmp/
```

Start `/init` with a serial/framebuffer console and an `ash` prompt. Add USB input events to the TTY only after the current normalized kernel input queue is connected to a character-device/TTY service.

## Test strategy

- **ABI tests:** one small program per syscall family; verify return values and `errno`.
- **libc tests:** string, memory, allocator, stdio, `fork`/`exec`, signal, and termios tests.
- **BusyBox smoke test:** boot QEMU, run every enabled applet with `--help`, then run scripted shell pipelines/redirections.
- **Filesystem tests:** create, rename, unlink, seek, directory iterate, permissions, symlink, and short-write cases.
- **Interactive tests:** keyboard input, Ctrl-C, Ctrl-D, arrow keys, resize/termios behavior, and serial console.
- **Regression:** every new syscall or driver must have a QEMU test that runs from a clean initramfs.

## Milestones

1. User ELF loader and syscall entry.
2. Static “hello” and `fork`/`exec` test programs.
3. `luna-libc` plus a working TTY and initramfs.
4. BusyBox Phase A with an interactive shell.
5. Persistent FAT32/ext2 and Phase B applets.
6. virtio-net and Phase C networking.
7. Dynamic linking and broader compatibility, only if needed.

The first useful target is not all of BusyBox. It is a reproducible static BusyBox build whose `ash` shell can launch a small set of reliable file/process utilities on a keyboard-driven Luna TTY.
