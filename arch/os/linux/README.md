# arch/os/linux — the platform layer

```
arch/os/linux/io/    the system call, and making one with no libc under you
arch/os/linux/scan/  where the system call instructions in a run of code are
arch/os/linux/sc/    arming them in this process, and standing behind the trap
```

`scan/` is pure functions over a buffer, no system calls, no allocation, no
state and is linked by both things that need the answer.
`sc/` is the in-process half. `io/` is underneath both of them, and is where
this file starts.

## arch/os/linux/io — the system call

Two callers want the system call as an *instruction* rather than as a libc
function, for opposite reasons, so it is written once and inlined from a header:

So `_syscall6()` is the instruction, and `_sys_read()`, `_sys_write()`,
`_sys_open()`, `_sys_close()`, `_sys_readlink()`, `_sys_access()`,
`_sys_stat()`, `_sys_execve()`, `_sys_exit()`, `_sys_sendto()` and
`_sys_recvfrom()` are one instruction and a return each. There is no errno —
the calls return the kernel's negative error the way the kernel gives it — no
buffering, no locale and no allocation. The `_sys_` prefix is what marks that
— everything under it is an instruction to the kernel and nothing else.

The `nolibc_` loops are why this directory is built `-fno-builtin` (see `io/Kbuild`)

hpc's logger is the third caller, and it is the one that is neither of the two
above: `hpc/log/write.c` reaches this header when `CONFIG_OS_LINUX_IO` is set
and makes its `open(2)` and its `write(2)` here. Both reasons apply to it at
once — a `-nostdlib` program that logs has no `write()` to call, and an object
that exports `write` to interpose on other people's would otherwise bind its
own log lines to its own hook. Where the symbol is not set the file is the libc
one it always was; the formatting half is libc's either way.

The command line comes from `/proc/self/cmdline` and
the environment from `/proc/self/environ`, both split in place, and the
environment is then handed to `execve(2)`

### The six symbols that are not a matter of not calling anything

`arch/os/linux/io/nostdlib.c`. Building without a C library is otherwise
entirely a matter of not calling one, and these are the exception: six data
symbols that hpc's own objects *refer to* whether or not the code around them
runs.

```
R_X86_64_GLOB_DAT   optarg, optind, opterr, optopt, stdout, stderr
R_X86_64_JUMP_SLOT  everything else
```

A jump slot is bound lazily, so an unresolved one costs nothing until it is
called. A `GLOB_DAT` is resolved when the image is loaded, so an unresolved one
is the loader refusing to start the program.

The constructor asks first, with a weak undefined reference, the loader
resolves one of those to zero rather than refusing to start, and
`__libc_start_main` is in every C library there is:

```c
extern char __libc_start_main[] __attribute__((weak));
```

and does nothing when the answer is no. Which is correct rather than merely
safe: there is no program in that process yet to watch. `ub_entry()` is about to
exec the shell, and the agent arrives properly in the shell and in everything
below it. The same answer covers the agent's six exported wrappers, whose
fallback is `arch/os/linux/io` and not `syscall(3)` — hpc's own `write()`
binds to
the agent's, and in that process there is no `syscall(3)` to fall back to.

## The mechanism

```
0f 05   ->   0f 0b            syscall  ->  ud2
d4000001 -> 00000000          svc #0   ->  udf #0
```

One byte on x86-64, one word on arm64, in place, and the instruction is the
same length it was — so nothing moves, nothing is relocated and no trampoline
is needed. The trap that raises is SIGILL, and the handler in `sc/trap.c` makes
the call the instruction would have made, hands the result back through the
trapping thread's saved registers and steps the program counter over it.

Three choices in that are worth the sentences they take.

**Why one byte.** `syscall` and `ud2` differ in the second byte only, so arming
a site is a single-byte store: a thread executing that address at that moment
sees either the whole of one instruction or the whole of the other, never a
third thing. There is no window and nothing has to be stopped. On arm64 the two
encodings share no bytes, but an aligned word store is atomic for the same
reason.

**Why SIGILL and not SIGTRAP.** SIGTRAP belongs to debuggers. A program being
debugged would have every one of these taken by gdb before the process ever saw
it, and a JIT that plants its own breakpoints would be fighting for the same
signal. SIGILL is a signal almost nothing installs a handler for, and the one
program that does, a CPU-feature probe, does it before this is armed.

**Why not seccomp.** A filter is inherited across `execve(2)` and a signal
handler is not, so a filter always outlives the thing that made it survivable:
what is left after an exec is a trap with nothing behind it. Armed instructions
do not have that problem, because they are in the image and an exec replaces
the image.

## Which sites are armed

Not all of them. A signal is one to two microseconds against a tenth of that
for the call itself, so arming everything would make a program that takes a
lock in a loop measurably slower for no reason.

The scan reads the number out of the instruction before the site. `mov $nr,
%eax` on x86-64, `mov x8, #nr` on arm64, which is how every stub in every libc
is written and a site whose number is decidable and is not one the caller
asked about is left alone. In a current glibc the number is decidable for 365
of the 450 sites in the library, and almost none of those 365 are calls anybody
here asked about.

The remaining 85 are armed, and the first time each one is used it says what it
makes. If that is not a call anybody asked about, the site is written off after
`SC_MISS_MAX` such calls and disarmed for the life of the process. Not on the
first one, and the reason is specific: a current glibc does not give each socket
call its own instruction —

```
send:  push $0x2c ; jmp __syscall_cancel
recv:  push $0x2d ; jmp __syscall_cancel
```

— so one site carries `send`, `recv`, `sendto`, `recvfrom`, `sendmsg`,
`recvmsg`, and `connect` and `accept` and `poll` besides. Disarming it because
the first call through it happened to be a `connect(2)` would give up every
socket call in the process.

## The four calls that are not simply performed

Making a system call from inside a signal handler is fine for almost all of
them. Four are not, and each of the four had to be found the hard way.

**`rt_sigprocmask` is answered on the signal frame.** It cannot be performed —
`rt_sigreturn(2)` reloads the mask out of the frame, so a mask set inside a
handler is undone the moment the handler returns, and the program would believe
it had blocked signals it had not. It cannot be refused either: let the program
block signals for real and it blocks SIGILL with them, and a synchronous SIGILL
raised while SIGILL is blocked is not queued, it is *forced* — the kernel kills
the process. glibc blocks every signal around every `pthread_create(3)` and
every `fork(3)`, so that window is not an edge case, it is thread creation. So
the new mask is computed and written into the frame, where `rt_sigreturn` will
read it, with SIGILL taken out of it on the way past.

**`rt_sigaction` on SIGILL is answered.** The program is told what it asked for
and nothing is installed, so the dispatcher keeps the signal. Not hypothetical:
`git` resets the whole signal table to `SIG_DFL` in a child it is about to run,
and the next armed instruction after that would kill it. A SIGILL that is *not*
ours is still delivered to whatever the program last asked for.

**`clone3` is refused with `ENOSYS`, after `CLONE_CLEAR_SIGHAND` is cleared out
of the request.** That flag asks the kernel to reset every handler in the child,
and `posix_spawn(3)` sets it — the child then runs a page of glibc before it
execs, and every armed instruction it touches is fatal. `ENOSYS` is what an
older kernel says, and every `clone3` caller in a C library has a `clone(2)`
fallback for exactly that answer; the fallback re-reads the same `clone_args`,
which by then has the flag cleared. This is the only place anything here
modifies an argument the program passed.

**`clone`, `fork`, `vfork`, `rt_sigreturn` and `sigaltstack` are refused.** The
child of a clone made from a handler would return *inside the handler*, on the
stack the caller passed, and then return through a signal frame belonging to its
parent; a sigreturn would unwind the handler's own frame; an alternate stack
would move the ground it is standing on. Such a site is disarmed and the
instruction re-executed, so the program makes the call itself with everything
where it belongs — and stays disarmed, because whatever it is, it is not what
the site was armed for.

## Register and stack discipline

The dispatcher is a signal handler, which means the kernel saves the interrupted
thread's registers and `rt_sigreturn(2)` restores every one of them — including
the ones the handler's own compiled code used. The frame is built below the red
zone on the machines that have one, so a function with live data there still has
it. There is no hand-written save and restore to get wrong, and no trampoline
entered with the program's registers live in the middle of a compiled function.

What is written back into the saved context is the list in `sc_uc_finish()` and
nothing else:

| what | why |
| --- | --- |
| `rax` / `x0` | the result the kernel returned |
| `rcx`, `r11` (x86-64) | which the `syscall` instruction itself clobbers |
| the program counter | forward by the length of the instruction |

`errno` is saved and restored across the whole handler, because the report
callback is ordinary code that may set it and the interrupted thread is about to
read its own.

The handler runs on the stack of the thread that took the trap, with no
alternate stack. `sigaltstack(2)` is per-thread and only the thread that runs
`sc_open()` could be given one, so an alternate stack does not make the
mechanism safe for small stacks, it makes one thread behave differently from
all the others, and if it is smaller than that thread's own stack it makes that
one thread *worse*. A 64 KiB alternate stack under un's flow layer and its TLS
decoder is a segmentation fault in the middle of a conversation that would have
decoded fine on the 8 MiB stack the main thread already had.

## `SA_NODEFER` is not optional

The report callback is a caller's ordinary code: it takes locks, it allocates,
it writes a log line, and each of those reaches the kernel through an
instruction that may itself be armed. So the handler has to be able to enter
itself. Without `SA_NODEFER` SIGILL is blocked for the duration of the handler,
and a synchronous SIGILL raised while SIGILL is blocked is forced. With it, the
nested trap arrives, sees the per-thread guard set, makes the call and says
nothing. Two deep, never more.

## The x86-64 length decoder

`scan/x86_64.c`. `syscall` is two bytes that also occur inside immediates,
inside displacements, inside the constant pools some hand-written assembler
leaves in `.text`, and in the tail of any instruction whose operands happen to
end that way. Writing over one of those corrupts an instruction that had nothing
to do with a system call, and the program dies somewhere else entirely, later,
for no reason a reader could connect to this.

So the run is swept: decoded instruction by instruction from a point known to be
an instruction boundary, and a candidate is a site only if the sweep arrives
exactly on it. The decoder measures rather than validates, VEX, EVEX and XOP
included, because a current glibc's string routines are written in them and a
run of AVX-512 it could not measure would desync the sweep for the rest of the
function.

The points come from `.eh_frame_hdr`, which every image compiled in the last
twenty years carries: the binary-search table an unwinder uses is one entry per
function, already sorted by address. That is a better anchor list than the
dynamic symbol table, it covers the local functions, nothing has to be sorted,
and no guess is needed about whether the loader relocated `.dynamic` in place.

One anchor lies. glibc gives its signal trampoline an unwind record that starts
one byte before the first instruction, so the sweep anchored there decodes two
instructions that are not there and arrives at the right site with no number —
and that site is `rt_sigreturn`, the way out of every signal handler in the
process. `scan_lookback()` is the backstop: where the sweep found no number, the
bytes immediately before the site are matched against the encodings that could
have loaded one. It can only ever turn "unknown" into a number, so a wrong match
can at worst arm a site that did not need it.

Measured against `objdump -d` over glibc, ld.so, libcrypto, libssl, libstdc++,
bash, openssl and python, about three million instructions, the decoder
mismeasures none, and the anchored sweep finds exactly the 450 `syscall`
instructions in glibc and no others.

## What it will not survive

- **A program that installs its own SIGILL handler through a path this does not
  see.** The `rt_sigaction` interception covers the ordinary one; a runtime that
  writes the system call inline in its own text, in a site the scan ruled out,
  is not covered. `UB_UN_SITES=0` turns the whole half off.
- **A debugger.** gdb stops on SIGILL like anything else; `handle SIGILL nostop
  pass` is what makes a watched program debuggable.
- **Go.** That runtime handles SIGILL itself, makes its system calls from its
  own text rather than from a libc, and runs on goroutine stacks measured in
  kilobytes.
- **An image loaded after `sc_arm_self()` ran.** Nothing here is told about a
  `dlopen()`. `LD_PRELOAD` has no such gap, which is one reason the agent's
  symbol wrappers are still there.
