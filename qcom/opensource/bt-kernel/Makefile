KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
M ?= $(shell pwd)

ifdef USE_DEDICATED_KERNEL_LE_TARGET
   KERNEL_SRC ?= /lib/modules/${KERNEL_VERSION}/build
endif
BT_ROOT=$(KERNEL_SRC)/$(M)
KBUILD_OPTIONS += BT_ROOT=$(BT_ROOT)
ifdef USE_DEDICATED_KERNEL_LE_TARGET
KBUILD_OPTIONS += BT_ROOT=$(shell cd $(KERNEL_SRC); readlink -e $(M))
KBUILD_OPTIONS += CONFIG_MSM_BT_POWER=m
KBUILD_OPTIONS += CONFIG_BTFM_SLIM=m
endif


all:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) modules $(KBUILD_OPTIONS)

modules_install:
	$(MAKE) INSTALL_MOD_STRIP=1 -C $(KERNEL_SRC) M=$(M) modules_install

%:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) $@ $(KBUILD_OPTIONS)

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean

