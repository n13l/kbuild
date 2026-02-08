KBUILD_DEFCONFIG := defconfig

KBUILD_CFLAGS += -D__LINUX_ARM_ARCH__=8

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

mcpu-feat-$(CONFIG_ARM_CRYPTO_EXTENSIONS) += +crypto
mcpu-feat-$(CONFIG_ARM_SHA3_EXTENSIONS)   += +sha3

ifneq ($(mcpu-y),)
mflags-y += -mcpu=$(mcpu-y)$(subst $(space),,$(mcpu-feat-y))
endif

KBUILD_CFLAGS += $(mflags-y)
KBUILD_AFLAGS += $(mflags-y)
