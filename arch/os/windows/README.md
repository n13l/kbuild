# arch/os/windows — the platform layer

```
arch/os/windows/io/       the calls a program makes with no C library under it
arch/os/windows/entropy/  the one call randomness is drawn with, the same way
arch/os/windows/scan/     where the system call instructions in a run of code are
arch/os/windows/include/  the POSIX headers Windows does not have (not built)
```

The counterpart of `arch/os/linux` and `arch/os/macos`, directory for directory
where Windows has an answer, plus one directory neither of them needs. There is
no `sc/` here yet; the last section but one says what it would take, and why
it is a TODO here when on macOS it is a note.

The target is mingw-w64: `x86_64-w64-mingw32` or `aarch64-w64-mingw32` with
gcc or clang, which is what kbuild calls `PLATFORM=windows`
(`scripts/host_from_sys.sh` also maps clang's `*-windows-gnu` and MSYS/Cygwin
there). The floor is Windows 10 (`_WIN32_WINNT=0x0A00`, set for the whole tree
in `scripts/Makefile.shared`).

`arch/os/str.h` is shared with every other platform layer, as it is for
macOS: byte loops and a `CONFIG_OS_IO` switch, no kernel anywhere in it.

## arch/os/windows/io — the system call that is not an instruction

The Linux and macOS layers make the system call themselves: `_syscall6()` is
the instruction, and each `_sys_` name is one instruction and a return. Here
there is no `_syscall6()` at all, and that is the first thing this layer has to
say.

The instruction exists — `syscall` in ntdll on x86-64, `svc` on arm64 — but the
numbers behind it are renumbered by every build of Windows, documented by none,
and known only to the copy of ntdll.dll that ships in the same update. A
program that made one would be right on the machine it was tested on. So the
stable boundary is one layer up: kernel32, ntdll's documented exports and
Winsock. None of those is a C library — kernel32 is the system's own interface,
the one Microsoft's CRT is written over — so the rule this directory exists for
holds: nothing in `io.c` calls msvcrt, and

```
nm -u obj/arch/os/windows/io/io.o
```

lists `__imp_` names and nothing else. Structures are filled member by member,
and the directory is built `-fno-tree-loop-distribute-patterns` beside the
usual two, because gcc turns a hand-written copy loop into `memcpy` at `-O2`
whatever `-fno-builtin` says.

Three things the callers assume and Windows does not have are made up:

**The descriptor.** An `int`, where Windows has a `HANDLE`. The int is the
handle's value — Windows guarantees handles fit 32 bits, and they are multiples
of four — except that 0, 1 and 2 are answered with `GetStdHandle()`, looked up
each time because `SetStdHandle()` can move them. A socket is a `SOCKET`, and
its value is the int the same way. `_sys_close()` asks Winsock whether the
descriptor is one of its sockets and uses `closesocket()` if it is, because a
layered provider keeps state only `closesocket()` releases.

**The error.** A negative errno, as on Linux and as macOS's carry flag is
folded into: `io_errno()` translates `GetLastError()` and `WSAGetLastError()`,
and is exported for a caller with an error code of its own.

**The path.** UTF-8, like every other string in this tree, converted to the
UTF-16 the wide API takes. The ANSI API would read the same bytes in whatever
code page the machine was installed with. Short paths are converted on the
stack (under a page, so no stack probe — that is a libgcc call); long ones get
a block from the process heap, so the wide API's 32767 characters is the limit.

And per call:

- `_sys_open()` maps the flags onto `CreateFileW` dispositions. `O_APPEND`
  is write access without `FILE_WRITE_DATA`, which leaves `FILE_APPEND_DATA`
  and makes the kernel put every write at the end atomically — the guarantee
  `O_APPEND` actually makes. Every open shares read, write and delete, as a
  POSIX open does, and every handle is non-inheritable: each descriptor here is
  `O_CLOEXEC`.
- `_sys_readlink()` reads the reparse data of a symbolic link or a junction
  (`FSCTL_GET_REPARSE_POINT`; the layout is the DDK's and is written out in
  `io.c`) and returns the print name. A file that is not a link is `EINVAL`, as
  on Linux. `/proc/self/exe` is answered by name with `GetModuleFileNameW`,
  because it is the idiom a program finds itself with.
- `_sys_access()` reads attributes, not the ACL: `W_OK` fails on a read-only
  file, `X_OK` passes for a directory and for the extensions `CreateProcess`
  runs (`.exe .com .bat .cmd`) — there is no execute bit.
- `_sys_stat()` opens the file (following links) and asks by handle, so
  `st_dev`/`st_ino` are the volume serial and file index, the pair that names a
  file on Windows.
- `_sys_execve()` is the one that cannot be what it says. There is no call
  that replaces a running image, so it starts the program, waits, and exits
  with its status: to the caller and to whatever started the caller, that is
  `execve`. The difference is visible only from outside — a new process id, and
  this process waiting behind it. msvcrt's `_execve()` does not wait, so a
  shell gets its prompt back while the program is still writing; this one
  waits so that it does not. The argument vector is quoted so that
  `CommandLineToArgvW()` gives back exactly the vector that went in.
- `_sys_getppid()` is ntdll's `NtQueryInformationProcess`: the id of the
  creating process, which Windows never reparents — after the parent exits it
  is a number that may belong to someone else.
- `_sys_gettimeofday()` is the precise clock, not the tick-resolution one.
- `_sys_sendto()`/`_sys_recvfrom()` are Winsock's, and need `WSAStartup()` to
  have been called by the program; without it they return `-ENOSYS`.

There is no `nostdlib.c` here, for the reason there is none under macOS:
nothing on this platform links without the C runtime's startup code.

## arch/os/windows/entropy — getrandom over BCryptGenRandom

`_sys_getrandom(buf, len, flags)` over `BCryptGenRandom` with the
system-preferred generator, which needs no algorithm handle and so has nothing
to open first and nothing to fail at startup. As on macOS, the flags are
accepted and ignored — the generator never blocks and there is one of it — and
the length is a loop, because the call takes a `ULONG`.

`_entropy_is_source_fd()` is always false, and that is not a shortcut. Linux
has 1:8 and 1:9, macOS has whatever `st_rdev` its two device names carry;
Windows has no file a `read()` draws randomness from at all. The kernel's
generator is `\Device\CNG`, which takes I/O controls, and nothing opens it by
name.

## arch/os/windows/scan — the syscall sites in ntdll

The same interface as the other two, and both machines, which is more than
macOS has: on Windows every system call is in one image, every one of them is
a stub of the same few instructions, and so the question has a precise answer.

**arm64.** The number is the `svc` immediate — Linux traps on `#0` and reads
`x8`, macOS on `#0x80` and reads `x16`, Windows encodes the call in the
instruction. So a site is any `svc` and its number is read off the word. Armed
is `udf` with the same immediate: the trap carries the number, and disarming is
possible at all, because there is no constant to restore the way there is on
the other two.

**x86-64.** `syscall` is the same two bytes and is armed the same way, as
`ud2`. Telling it from the same bytes inside an immediate takes the instruction
length decoder `arch/os/linux/scan/x86_64.c` carries — and nothing in that
decoder is Linux's, so `x86_64.c` here compiles it again rather than carrying a
copy, with the one platform-specific function, `scan_lookback()`, renamed out
of the way. What differs is where the number is set. A current stub is

```
mov r10, rcx ; mov eax, NN ; test byte [SharedUserData+0x308], 1 ; jne ; syscall
```

so the `mov` is three instructions back and the sweep, which keeps a number for
one instruction, has lost it; the lookback matches that template first and
leaves the Windows 7/8 shape (`mov eax` immediately before) to the Linux
lookback. The `int 2e` each stub also carries is not reported: `cd 2e` has no
one-byte store that makes it undefined and back.

A number a site reports is a fact about this ntdll, not a name: the same Nt
call is a different number on the next build. What makes it useful is the
export it was found in, which the caller knows.

## arch/os/windows/include — the headers Windows does not have

Not built, and not a platform layer the way `io/` is: POSIX for sources that
include it, setting errno like the calls it stands in for. kbuild puts it on
every Windows compile with `-idirafter`, after the toolchain's own headers, and
nothing in it has a name mingw also uses. Everything is `static inline`, so it
needs no object and a tree links nothing extra for it.

- `sys/mman.h` — `mmap`/`munmap`/`mprotect`/`madvise`/`msync` over
  `VirtualAlloc` and file mappings. `madvise(MADV_DONTNEED)` decommits and
  recommits, so the range reads back as zeroes, which is what the hpc slab's
  release relies on (`SLAB_VM_RELEASES`). `MAP_NORESERVE` cannot be honoured:
  Windows does not overcommit, so a mapping is charged against the commit limit
  when it is made, though it uses no RAM until touched. No `mremap`, and no
  `MREMAP_MAYMOVE`, which is how callers ask.
- `sys/ioctl.h` — `TIOCGWINSZ` from the console's visible window, `FIONBIO` and
  `FIONREAD` through `ioctlsocket()`; anything else is `ENOTTY`.
- `netinet/in.h`, `arpa/inet.h` — Winsock under the POSIX names, plus the
  `in_addr_t`/`in_port_t` typedefs Winsock never adopted.

## Why there is no arch/os/windows/sc yet

The Linux half arms the syscall instructions of this process and stands behind
the trap. On macOS the first of the three things that needs is impossible — the
text worth arming is in the dyld shared cache, mapped read-only, shared and
signature-checked. On Windows all three are possible:

- **The text is writable by its owner.** ntdll is mapped copy-on-write into
  each process, and `VirtualProtect(PAGE_EXECUTE_READWRITE)` over its `.text`
  works for an ordinary process — it is how every hooking library works —
  unless the process has opted into Arbitrary Code Guard.
- **The trap is the process's own.** `ud2` and `udf` raise
  `EXCEPTION_ILLEGAL_INSTRUCTION`, and a vectored exception handler
  (`AddVectoredExceptionHandler`, first in line) sees it with the full
  `CONTEXT` before any frame-based handler does.
- **The dispatcher can make the call itself.** It cannot use the stub it armed,
  but it does not need a number: it calls the same Nt export through a copy of
  the original stub bytes, or disarms, single-steps and rearms.

What is not here is the work: walking ntdll's export table instead of
`dl_iterate_phdr`, a `CONTEXT` instead of a `ucontext_t`, and the calls a
vectored handler must not make. `CONFIG_OS_SC` is not offered on this platform
until that is written, and `<arch/os/sc.h>` is a `#error` here, as it is on
macOS — but for a different reason.

## What a Windows build of this tree leaves out

Decided in Kconfig, not by `#ifdef`, so `make menuconfig` says it:

- **Accelerated crypto** — `CC_CPU_ACCEL_NONE` is the Windows default. The
  backends are aws-lc and s2n-bignum assembly in ELF and Mach-O spellings, and
  aws-lc's Windows assembly is NASM. With it, `CRYPTO_VERIFIED` defaults off
  too, since the verified implementations are that assembly.
- **RCU** — liburcu has no Windows port.
- **cmocka suites** — offered only when the target toolchain can link
  `-lcmocka` (`CC_HAS_LIBCMOCKA`, probed by kbuild).
- **tools/unprobe** — DPDK, through the target's pkg-config (`HAVE_PKG_LIBDPDK`).

Programs are linked `-static`, so an `.exe` needs only system DLLs beside it.
