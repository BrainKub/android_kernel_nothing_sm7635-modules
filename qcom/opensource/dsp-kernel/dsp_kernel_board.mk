ifneq ($(TARGET_KERNEL_DLKM_DISABLE), true)
ifneq ($(ENABLE_HYP), true)
ifneq (,$(call is-board-platform-in-list2,$(TARGET_BOARD_PLATFORM)))
BOARD_VENDOR_KERNEL_MODULES += $(KERNEL_MODULES_OUT)/frpc-adsprpc.ko
#BOARD_VENDOR_KERNEL_MODULES += $(KERNEL_MODULES_OUT)/frpc-trusted-adsprpc.ko
BOARD_VENDOR_KERNEL_MODULES += $(KERNEL_MODULES_OUT)/cdsp-loader.ko
endif
endif
endif
