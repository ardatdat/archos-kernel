KERNEL_BUILD_DIR=$(BASE_DIR)/
KERNEL_CONFIG=$(KERNEL_BUILD_DIR)/linux/.config
KERNEL_CONFIG_SRC=$(KERNEL_SRC_REAL_DIR)/linux.config

ifeq ($(ARCH),arm)
KERNEL=$(KERNEL_BUILD_DIR)/linux/arch/arm/boot/zImage
else
KERNEL=
endif

KERNEL_SRC_REAL_DIR:=$(BASE_DIR)/../linux
KERNEL_SRC_DIR:=$(KERNEL_SRC_REAL_DIR)

ifeq ($(KERNEL_FLAVOUR),init)
USE_TINY=n
else
ifeq ($(KERNEL_FLAVOUR),recovery)
USE_TINY=n
else
USE_TINY=n
endif
endif

## copy kernel source only if for patching it!!!
ifeq ($(USE_TINY),y)
KERNEL_SRC_DIR=$(BUILD_DIR)/$(KERNEL_FLAVOUR)-linux-src
endif
ifeq ($(USE_KGDB),y)
KERNEL_SRC_DIR=$(BUILD_DIR)/$(KERNEL_FLAVOUR)-linux-src
endif

$(KERNEL_SRC_DIR):
ifneq ($(KERNEL_SRC_DIR),$(KERNEL_SRC_REAL_DIR))
	mkdir -p $(KERNEL_SRC_DIR)
	tar -C $(KERNEL_SRC_REAL_DIR) --exclude=.svn -cpf - . | tar -C $(KERNEL_SRC_DIR) -xf -
ifeq ($(USE_KGDB),y)
	@echo "Apply KGDB patches"
	$(BUILDROOT)/toolchain/patch-kernel.sh $(KERNEL_SRC_DIR) $(TOOLS_DIR)/kgdb-patches \*.patch*
endif
ifeq ($(USE_TINY),y)
	@echo "Apply Tiny Linux patches"
	$(BUILDROOT)/toolchain/patch-kernel.sh $(KERNEL_SRC_DIR) $(TOOLS_DIR)/tiny-patches \*.patch*
endif
else
	@echo "Use unpatched kernel"
endif

$(KERNEL_BUILD_DIR)/linux: $(KERNEL_SRC_DIR)
	@mkdir -p $(KERNEL_BUILD_DIR)/linux

$(KERNEL_CONFIG): $(KERNEL_CONFIG_SRC)
	cp $(KERNEL_CONFIG_SRC) $(KERNEL_CONFIG)
ifeq ($(USE_KGDB),y)
	@echo "Apply KGDB configuration"
	sed -i -e 's/.*\(CONFIG_DEBUG_KERNEL\).*/\1=y/' $(KERNEL_CONFIG)
	@echo "Set default KGDB configuration"
	@echo "# Default KGDB options" 				>> $(KERNEL_CONFIG)
	@echo "CONFIG_KGDB=y" 					>> $(KERNEL_CONFIG)
	@echo "CONFIG_HAVE_ARCH_KGDB=y" 			>> $(KERNEL_CONFIG)
	@echo "CONFIG_KGDB_SERIAL_CONSOLE=y" 			>> $(KERNEL_CONFIG)
	@echo "CONFIG_KGDB_8250=y" 				>> $(KERNEL_CONFIG)
	@echo "# CONFIG_KGDBOE is not set" 			>> $(KERNEL_CONFIG)
	@echo "# CONFIG_KGDB_TESTS is not set" 			>> $(KERNEL_CONFIG)
	@echo "# CONFIG_DEBUG_USER is not set" 			>> $(KERNEL_CONFIG)
	@echo "# CONFIG_DEBUG_ERRORS is not set" 		>> $(KERNEL_CONFIG)
endif
ifeq ($(USE_TINY),y)
	@echo "Apply Tiny Linux configuration"
	@echo "# Default Tiny Linux options" 			>> $(KERNEL_CONFIG)
	@echo "CONFIG_AIO=y" 					>> $(KERNEL_CONFIG)
	@echo "CONFIG_XATTR=y" 					>> $(KERNEL_CONFIG)
	@echo "# CONFIG_FILE_LOCKING is not set" 		>> $(KERNEL_CONFIG)
	@echo "# CONFIG_ETHTOOL is not set" 			>> $(KERNEL_CONFIG)
	@echo "# CONFIG_INETPEER is not set" 			>> $(KERNEL_CONFIG)
	@echo "# CONFIG_NET_DEV_MULTICAST is not set" 		>> $(KERNEL_CONFIG)
	@echo "# CONFIG_MEASURE_INLINES is not set" 		>> $(KERNEL_CONFIG)
	@echo "# CONFIG_PRINTK_FUNC is not set" 		>> $(KERNEL_CONFIG)
	@echo "CONFIG_PANIC=y" 					>> $(KERNEL_CONFIG)
	@echo "CONFIG_FULL_PANIC=y"	 			>> $(KERNEL_CONFIG)
	@echo "CONFIG_NET_SMALL=y" 				>> $(KERNEL_CONFIG)
	@echo "# CONFIG_CRC32_TABLES is not set" 		>> $(KERNEL_CONFIG)
	@echo "# CONFIG_CC_FUNIT_AT_A_TIME is not set" 		>> $(KERNEL_CONFIG)
	@echo "# CONFIG_LINUXTINY_DO_UNINLINE is not set" 	>> $(KERNEL_CONFIG)
	@echo "CONFIG_BINFMT_SCRIPT=y" 				>> $(KERNEL_CONFIG)
	@echo "CONFIG_MAX_SWAPFILES_SHIFT=0" 			>> $(KERNEL_CONFIG)
	@echo "CONFIG_NR_LDISCS=2" 				>> $(KERNEL_CONFIG)
	@echo "CONFIG_MAX_USER_RT_PRIO=5" 			>> $(KERNEL_CONFIG)
	@echo "CONFIG_CRC32_CALC=y" 				>> $(KERNEL_CONFIG)
endif
	@(PATH=$(shell pwd)/$(TOOLCHAIN_PATH):$(PATH) $(MAKE) -C $(KERNEL_SRC_DIR) oldconfig O=$(KERNEL_BUILD_DIR)/linux ARCH=arm CROSS_COMPILE=arm-linux-)

$(KERNEL): $(KERNEL_BUILD_DIR)/linux $(KERNEL_CONFIG) FORCE
	(PATH=$(shell pwd)/$(TOOLCHAIN_PATH):$(PATH) $(MAKE) $(MAKE_J) -C $(KERNEL_SRC_DIR) O=$(KERNEL_BUILD_DIR)/linux ARCH=arm CROSS_COMPILER=arm-linux-) 
	
kernel-clean:
	@(PATH=$(shell pwd)/$(TOOLCHAIN_PATH):$(PATH) $(MAKE) -C $(KERNEL_SRC_DIR) O=$(KERNEL_BUILD_DIR)/linux ARCH=arm CROSS_COMPILE=arm-linux- clean)

kernel-cleaner: kernel-clean
	rm -rf $(KERNEL_BUILD_DIR)/linux

kernel: $(KERNEL)

.PHONY: kernel-clean kernel-cleaner FORCE

TARGETS	+= kernel

