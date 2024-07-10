
# Kbuild repository template for userland 
> The same code base is used for a different range of computing systems, from
supercomputers to very tiny devices.

## Kbuild 
- Much simpler makefiles without the glue code that are hard to read and maintain
- Reduce the burden of code dependency management from developers
- Easy and efficient way to manage all compilation and configuration options on top of platform capabilities
- Precise dependency tracking with strong support of parallelism
- Tasks are split up and run simultaneously on multiple processors with different input in order to obtain results faster.
- Build versioning in continuous delivery
- Readable and configurable log

## Kconfig
- Easy to change/browse configuration
- Clear dependency between features and capabilities
- Help docs in Kconfig rather than a README

## Native CPU detection (cpucap)

`scripts/cpucap.c` is a dependency-free host probe that reports the CPU
crypto-acceleration extensions of the build host. It is compiled with `$(HOSTCC)`
on every make (at parse time) to `$(objtree)/cpucap`, and its `--env` output is
exported so `arch/*/Kconfig` can read it via `option env`:

| Variable           | Consumed by                                    |
|--------------------|------------------------------------------------|
| `HOST_X86_MODEL`   | `arch/x86/Kconfig.cpu` "Processor family"       |
| `HOST_ARM_MODEL`   | `arch/arm64/Kconfig` "ARM CPU"                  |
| `HOST_ARM_HAS_DIT` | `arch/arm64/Kconfig` `ARM_DIT`                  |

Enabling `CPU_NATIVE` then auto-selects the processor family matching the build
host, pulling in its predefined capabilities (AES/PMULL/SHA-2, SHA-3, ...), and
points the compiler at `-march=native` / `-mcpu=native`.

```
make cpucap          # report the host's capabilities
./obj/cpucap --env   # just the detected model
```

Consuming projects get this for free — no per-project Makefile glue. A project
that wants to probe differently can export `_CPUCAP_DONE` plus the `HOST_*`
values from its own top-level Makefile before including kbuild's; the built-in
probe then stands down.

## Directory structure

| FilePath                | Description                                          |
|-------------------------|------------------------------------------------------|
| Makefile                | The top Makefile.                                    |
| .config                 | The package configuration file.                      |
| arch/$(ARCH)/           | The Architecture layer                               |
| os/$(PLATFORM)/         | The Platform layer                                   |
| scripts/                | Common rules, scripts and tools for the build system |
| kbuild Makefiles        | Custom Makfiles                                      |

| Supported Matrix | Status                                                   |
|------------------|----------------------------------------------------------|
| Architecture     | x86_32, x86_64, arm32, arm64, ppc64, os390 , os390x      |
| Platform         | Linux, Windows, MacOS, iOS, Android, IBM AIX, IBM Z/OS   |
| Compiler         | GCC, CLANG, XLC                                          |

## Build in Action

```
git clone git@github.com:n13l/crypto.git
cd crypto/
git submodule update --init
```

![Demo](https://github.com/n13l/crypto/blob/main/.github/assets/demo.gif)


