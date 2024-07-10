KBUILD_DEFCONFIG := defconfig

KBUILD_CFLAGS += -D__LINUX_ARM_ARCH__=8

# HOST_ARM_MODEL (the cpucap-detected build-host core that CPU_NATIVE selects)
# is probed and exported once by kbuild's top-level Makefile, from
# scripts/cpucap.c, so every recursive make sees an identical value; see the
# comment there.

mcpu-$(CONFIG_ARM_CPU_CORTEX_A53)   := cortex-a53
mcpu-$(CONFIG_ARM_CPU_CORTEX_A55)   := cortex-a55
mcpu-$(CONFIG_ARM_CPU_CORTEX_A57)   := cortex-a57
mcpu-$(CONFIG_ARM_CPU_CORTEX_A72)   := cortex-a72
mcpu-$(CONFIG_ARM_CPU_CORTEX_A76)   := cortex-a76
mcpu-$(CONFIG_ARM_CPU_NEOVERSE_N1)  := neoverse-n1
mcpu-$(CONFIG_ARM_CPU_NEOVERSE_V1)  := $(shell $(CC) -mcpu=neoverse-v1 -x c -c /dev/null -o /dev/null 2>/dev/null && echo neoverse-v1 || echo cortex-a76)
mcpu-$(CONFIG_ARM_CPU_NEOVERSE_V2)  := $(shell $(CC) -mcpu=neoverse-v2 -x c -c /dev/null -o /dev/null 2>/dev/null && echo neoverse-v2 || echo neoverse-n1)
mcpu-$(CONFIG_ARM_CPU_CORTEX_A710)  := $(shell $(CC) -mcpu=cortex-a710 -x c -c /dev/null -o /dev/null 2>/dev/null && echo cortex-a710 || echo cortex-a76)
mcpu-$(CONFIG_ARM_CPU_CORTEX_X2)    := $(shell $(CC) -mcpu=cortex-x2 -x c -c /dev/null -o /dev/null 2>/dev/null && echo cortex-x2 || echo cortex-a76)
mcpu-$(CONFIG_ARM_CPU_APPLE_M1)     := $(shell $(CC) -mcpu=apple-m1 -x c -c /dev/null -o /dev/null 2>/dev/null && echo apple-m1 || echo generic)
# Newer Apple cores may be unknown to older compilers; degrade to the newest
# apple-m* the compiler accepts, and ultimately to generic.
mcpu-$(CONFIG_ARM_CPU_APPLE_M4)     := $(shell for c in apple-m4 apple-m3 apple-m2 apple-m1 generic; do $(CC) -mcpu=$$c -x c -c /dev/null -o /dev/null 2>/dev/null && echo $$c && break; done)
mcpu-$(CONFIG_ARM_CPU_APPLE_M5)     := $(shell for c in apple-m5 apple-m4 apple-m3 apple-m2 apple-m1 generic; do $(CC) -mcpu=$$c -x c -c /dev/null -o /dev/null 2>/dev/null && echo $$c && break; done)

mcpu-feat-$(CONFIG_ARM_CRYPTO_EXTENSIONS) += +crypto
mcpu-feat-$(CONFIG_ARM_SHA3_EXTENSIONS)   += +sha3

# Native detection: compile for the exact host CPU. This overrides the
# auto-selected core's -mcpu (which only conveys the capability tier to Kconfig)
# so codegen is always correct for the host; -mcpu=native already covers the
# crypto/SHA-3 extensions, so the +crypto/+sha3 modifiers are dropped here.
ifdef CONFIG_CPU_NATIVE
mcpu-y := $(shell $(CC) -mcpu=native -x c -c /dev/null -o /dev/null 2>/dev/null && echo native || echo generic)
mcpu-feat-y :=
endif

ifneq ($(mcpu-y),)
mflags-y += -mcpu=$(mcpu-y)$(subst $(space),,$(mcpu-feat-y))
endif

KBUILD_CFLAGS += $(mflags-y)
KBUILD_AFLAGS += $(mflags-y)
