# SPDX-License-Identifier: GPL-2.0-only

ifeq ($(CONFIG_ARCH_PINEAPPLE), y)
include $(WLAN_PLATFORM_ROOT)/cnss_genl/pineapple_gki_defconfig
endif

ifeq ($(CONFIG_ARCH_SUN), y)
include $(WLAN_PLATFORM_ROOT)/cnss_genl/sun_gki_defconfig
endif

ccflags-y += -Wmissing-prototypes

ifeq ($(CONFIG_CNSS_OUT_OF_TREE),y)
ccflags-y += -I$(WLAN_PLATFORM_ROOT)/inc
endif

obj-$(CONFIG_CNSS_GENL) := cnss_nl.o
