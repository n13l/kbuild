# kbuild, built on its own.
#
# This file is read only when kbuild is the srctree — a checkout of its own. In
# an integrated tree (un, ub) kbuild is vendored and that tree's Kbuild is the
# one make reads; this one is never seen there. So everything here is about the
# standalone build, and there is exactly one thing in it worth building on its
# own: the platform layer (arch/os/<platform>), and the self-tests that are the
# reason this file exists at all — kbuild's own code deserves kbuild's own tests.
#
# PLATFORM is derived from the target triple before the configuration is read
# (scripts/Makefile.target); the platform Kconfig under it is sourced by the
# standalone mainmenu (Kconfig), so the OS_* symbols below exist here.
need-os := $(CONFIG_OS_IO)$(CONFIG_OS_ENTROPY)$(CONFIG_OS_SCAN)$(CONFIG_OS_SC)

ifneq ($(need-os),)
subdir-y += arch/os/$(PLATFORM)
endif

subdir-$(CONFIG_TEST) += tools/testing/selftests

# <arch/os/...> resolves against the srctree root. Here arch is a real directory
# at that root; in a consumer tree it is a symlink to this one, reached the same
# way. The selftests include their headers like this.
subdir-ccflags-y += -I$(srctree)

# The selftests link the platform objects, so those are built first.
ifneq ($(need-os),)
tools/testing/selftests: | arch/os/$(PLATFORM)
endif
