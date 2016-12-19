
# Kbuild repository template for userland development
> The same code base is used for a different range of computing systems, from supercomputers to very tiny devices.
> [![Build Status](https://travis-ci.org/n13l/kbuild.png?branch=master)](https://travis-ci.org/n13l/kbuild) [![Build Status](https://snap-ci.com/n13l/kbuild/branch/master/build_image)](https://snap-ci.com/n13l/kbuild/branch/master) [![Release](https://img.shields.io/github/release/n13l/kbuild.svg)](https://packagecloud.io/n13l/openaaa) 

## Kbuild 
- Much simpler makefiles without the glue code that are hard to read and maintain
- Reduce the burden of dependency management from developers
- Take the responsibility of ever increasing code dependency management that ultimately becomes unmanageble using traditional build system after some point.
- Easy and efficient way to manage all compilation and configuration options on top of architecture and platform capabilities
- Readable log
- Precise dependency tracking

## Kconfig
- Easy to change/browse configuration
- Clear dependency between features and capabilities
- Help docs in Kconfig rather than a README


| Directory structure     | Description                                          |
|-------------------------|------------------------------------------------------|
| Makefile                | The top Makefile.                                    |
| .config                 | The package configuration file.                      |
| arch/$(ARCH)/           | The Architecture layer                               |
| sys/$(PLATFORM)/        | The Platform layer                                   |
| sys/unix/               | System interfaces compatible with Unix and extensions|
| mem/                    | Generic, high performance and lock-free Memory Management      |
| scripts/                | Common rules, scripts and tools for the build system |
| kbuild Makefiles        | Custom Makfiles                                      |

| Supported Matrix | Status                                                   |
|------------------|----------------------------------------------------------|
| Architecture     | x86_32, x86_64, arm32, arm64, ppc64, os390 , os390x      |
| Platform         | Linux, Windows, MacOS, iOS, Android, IBM AIX, IBM Z/OS   |
| Compiler         | GCC, CLANG, XLC                                          |

