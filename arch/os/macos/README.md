# arch/os/macos — the platform layer

```
arch/os/macos/io/       the system call, and making one with no libc under you
arch/os/macos/entropy/  the one call randomness is drawn with, the same way
arch/os/macos/scan/     where the system call instructions in a run of code are
```

The counterpart of `arch/os/linux`, directory for directory, minus one: there
is no `sc/` here. The last section says why, because it is the interesting half
of what this file has to say.

`arch/os/str.h` is shared with every other platform layer rather than copied
into this one. It is byte loops over NUL-terminated strings and a
`CONFIG_OS_IO` switch — no kernel appears in it anywhere — so there is one copy
and both `io.h`s include it.

## arch/os/macos/io — the system call

The same shape as Linux's: `_syscall6()` is the instruction, and `_sys_read()`,
`_sys_write()`, `_sys_open()`, `_sys_close()`, `_sys_readlink()`,
`_sys_access()`, `_sys_stat()`, `_sys_execve()`, `_sys_exit()`, `_sys_sendto()`,
`_sys_recvfrom()`, `_sys_getpid()`, `_sys_getppid()`, `_sys_gettid()` and
`_sys_gettimeofday()` are one instruction and a return each. No errno, no
buffering, no locale, no allocation.

Three things in it are Darwin's rather than Linux's.

**The number, and where it goes.**

```
arm64     mov x16, #4 ; svc #0x80            Linux: mov x8, #64 ; svc #0
x86-64    mov $0x2000004, %eax ; syscall     Linux: mov $1, %eax ; syscall
```

On arm64 the number is in `x16`, not `x8`, and the trap immediate is `#0x80`,
not `#0`. On x86-64 it is in `rax` as everywhere, but with the class in the top
byte: Darwin reaches several call tables through the one instruction — BSD,
Mach, machine-dependent — and the BSD one is class 2, so the number the
instruction carries is `0x2000000 | nr`. The numbers themselves are the plain
ones out of `<sys/syscall.h>`.

**The error.** Linux returns `-errno` in the result register and nothing else.
The BSD kernels return the error as a *positive* number and set the carry flag
to say that is what it is. Every caller in this tree is written against the
Linux convention, so the flag is read with the result and folded back into it —
which is the whole reason the asm here has a second output, an earlyclobber on
it and a `"cc"` clobber, and the Linux asm has none of those.

**`SYS_gettid` is not `gettid`.** Darwin's `<sys/syscall.h>` does define
`SYS_gettid`, and it is call 286, which reads the calling thread's effective uid
and gid. The thread identifier is `SYS_thread_selfid`. A caller that reaches for
the number it recognised gets the wrong call and no error to say so, which is
exactly the kind of fact a platform layer exists to own — so `_sys_gettid()` is
in `io.h` beside the rest and `hpc/log/write.c` asks for it by name.

There is no `nostdlib.c` beside `io.c`. The Linux one is the six data symbols an
ELF `-nostdlib` link leaves undefined, two of which are `stdout` and `stderr` —
which on macOS are not data symbols at all but macros over `__stdoutp`, so there
is nothing here for such a file to define. Nor is there a program that would
need it: see the last section.

## arch/os/macos/entropy — getrandom over getentropy

`_sys_getrandom(buf, len, flags)`, because that is the name and the shape the
callers of this tree are written against, over `getentropy(2)`, because that is
the call this kernel has. Two things are made up to keep the shape:

- **the flags**: there are none. `GRND_NONBLOCK` and `GRND_RANDOM` are choices
  about a pool that is not how this kernel is built. The argument is accepted
  and ignored rather than refused, so a caller passing what it passes on Linux
  gets randomness rather than `-EINVAL`.
- **the length**: `getentropy` takes at most 256 bytes and fails the whole call
  for one byte more. The loop is the difference, and a short count comes back
  the way `getrandom` returns one.

`_entropy_is_source_fd()` cannot be the pair of constants it is on Linux, where
`/dev/random` and `/dev/urandom` are character device 1:8 and 1:9 and always
have been. On macOS the major is whichever slot the random driver took in the
character device switch, which is a fact about this boot. So the devices are
asked for by the names they are guaranteed to have, once, and what is compared
after that is `st_rdev` — which is the question the constants were standing in
for in the first place.

## arch/os/macos/scan — the syscall sites in a run of code

Pure functions over a buffer, and the same two files Linux has: a
machine-independent driver and a step named after the architecture. arm64 only.
The step differs from Linux's in the two constants above — `svc #0x80` rather
than `svc #0`, `x16` rather than `x8` — and in nothing else, because four-byte
aligned instructions leave nothing to decode.

x86-64 is not here. `syscall` there is two bytes that also occur inside
immediates, displacements and data, so telling a site from a coincidence needs
the instruction length decoder `arch/os/linux/scan/x86_64.c` carries, and there
is no consumer on this platform that would have paid for a second one.

## Why there is no arch/os/macos/sc

The Linux half arms the system call instructions in this process — one byte over
each `syscall`, one word over each `svc` — and stands behind the `SIGILL` they
raise. Three things have to be true for that, and on macOS the first one is not.

**The text has to be writable by the process that owns it.** What is worth
arming is a C library's text, because that is where a program's system calls
are. On macOS that text is `libsystem_kernel`'s, and it is not in the program's
own images at all: it is in the dyld shared cache, one file mapped into every
process on the machine at the same address. Those pages are shared, they are
mapped read-only and execute, and on Apple silicon they are validated against
their code signature by the page fault handler. `mprotect(PROT_WRITE)` over them
returns `EACCES`, and if it did not, a byte written there would be a byte
written into every process on the machine. A program's *own* `__TEXT` is no
better placed: arm64 macOS enforces W^X per mapping, and a signed page that is
made writable is a page that stops being executable.

There is no entitlement that changes this for an ordinary build, and the ones
that come close (`get-task-allow`, disabling library validation) are about a
debugger attaching from outside, which is the out-of-process mechanism this
whole directory exists to not be.

**The trap has to be the process's own signal.** This part would have worked.
`SIGILL` is delivered the same way and `ucontext_t` on arm64 carries the same
registers under different names — `uc_mcontext->__ss.__x[]`, `__pc` — so the
dispatcher and the register discipline would have ported almost word for word.

**Nothing may modify a signal disposition behind the dispatcher's back.** This
part would have needed different work rather than the same work: the calls
Linux's `sc/trap.c` answers on the frame instead of performing —
`rt_sigprocmask`, `rt_sigaction`, `clone3` — are `__pthread_sigmask`,
`sigaction` and `posix_spawn` here, and `posix_spawn` on macOS is a system call
in its own right rather than a page of library code to survive.

So the first reason is the one that settles it, and the other two are why this
is a note rather than a TODO. `CONFIG_OS_SC` is not offered on this platform,
and `<arch/os/sc.h>` is a `#error` here rather than a header that compiles into
something that returns an error at run time.

## What else is not here

`tools/ub-patch` is Linux-only in this tree, for reasons of file format rather
than of kernel. It writes a `DT_NEEDED` into a file's dynamic array; the Mach-O
counterpart is an `LC_LOAD_DYLIB` load command, which is a different editor
entirely, and a file whose load commands are edited is a file whose code
signature no longer matches it.

## tools/ub, and what it costs here

`ub` is built on this platform, and neither of the two sentences that describe
it on Linux survives the crossing. Both are worth stating plainly, because each
is a fact about macOS rather than a shortcut taken in the port.

**It is two files, not one.** On Linux `ub` is a single object that is both the
program you run and the thing the loader preloads — a `-shared` link given a
`PT_INTERP`, an entry point of its own and the permission bit. A Mach-O carries
one filetype in its header and dyld reads it, so each half of that is refused in
its own words: `exec` of a `MH_DYLIB` is "cannot execute binary file", and
inserting a `MH_EXECUTE` is dyld's "cannot link against a main executable". So
the agent is `ub-agent`, a dylib linked from the same archives and named by
`DYLD_INSERT_LIBRARIES`. With the two halves separate, the `-nostdlib`
discipline goes with them: that rule is about the object the loader puts in
every process, and the program half is now an ordinary program.

**The shell is a copy.** System Integrity Protection does not ignore
`DYLD_INSERT_LIBRARIES` on the way into an Apple binary, it erases it — the
agent is not loaded and the variable is not there for the shell to pass on, so
with a system shell the chain ends before the first command is typed. What SIP
is protecting says so in the filesystem, with `SF_RESTRICTED`, which is one
`stat(2)` to ask about. Where the shell carries it, `ub` execs an ad-hoc-signed
copy of that shell kept under `~/.cache/ub`: a copy is not a *platform* binary,
the insert is honoured, and the variable survives into everything the shell
starts.

What that does not buy is Apple's own binaries below the shell. SIP strips the
insert for each of them too, so `/usr/bin/curl`, `/usr/bin/ssh` and
`/usr/bin/python3` run unwatched under `ub` while a Homebrew or locally built
program in the same shell is watched. Nothing in a process can change that.

**The hooks are asked for rather than exported.** A flat namespace is what makes
`LD_PRELOAD` work by defining `read`: the name is looked up across the whole
image list. Mach-O binds each reference to the library it was linked against, so
an inserted dylib that defines `read` defines a second one that nothing calls.
`__DATA,__interpose` is dyld's mechanism for it and the one this uses — pairs of
addresses that dyld rewrites bindings to, whoever they were linked against — so
`tools/ub/un.map` has no counterpart here and needs none. One consequence is
worth carrying: dyld does not interpose inside the image that declares the
tuples, so the agent reaches the real function by naming it, and must *not* ask
`dlsym()` for it — `dlsym` does apply interpositions, and returns the agent its
own hook.

`CONFIG_OS_SC` is still not offered, so the syscall-site half of the agent is
not here and `ub` on this platform watches the symbols only, as the log line
says. The entropy module's crypto-library hooks (`RAND_bytes` and the rest) are
in the same position as `read` was and are not interposed, so
`CONFIG_MODULE_UB_ENTROPY` has nothing to catch here yet.
