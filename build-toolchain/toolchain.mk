CPU_CORES=$(shell nproc --all)
SYSTEM_TRIPLET=$(shell make -v | grep -oP "(?<=Built for )\S+")

TOOLCHAIN_DIRECTORY=.toolchain
TOOLCHAIN_PREFIX=$(abspath $(TOOLCHAIN_DIRECTORY)/$(TARGET))

GCC_BUILD=$(TOOLCHAIN_DIRECTORY)/gcc-build
BINUTILS_BUILD=$(TOOLCHAIN_DIRECTORY)/binutils-build

export PATH := $(PATH):($TOOLCHAIN_PREFIX)/bin

export TARGET_CC = $(TARGET)-gcc
export TARGET_CXX = $(TARGET)-g++
export TARGET_LD = $(TARGET)-gcc

build_toolchain: toolchain_binutils toolchain_gcc

toolchain_binutils: toolchain_always
	cd $(TOOLCHAIN_DIRECTORY); wget $(BINUTILS_URL)
	cd $(TOOLCHAIN_DIRECTORY); tar -xf binutils-$(BINUTILS_VERSION).tar.xz
	cd $(TOOLCHAIN_DIRECTORY); rm -f binutils-$(BINUTILS_VERSION).tar.xz
	\
	mkdir $(BINUTILS_BUILD)
	cd $(BINUTILS_BUILD); ../binutils-$(BINUTILS_VERSION)/configure 	\
		--host=$(SYSTEM_TRIPLET)										\
		--build=$(SYSTEM_TRIPLET)										\
		--prefix="$(TOOLCHAIN_PREFIX)" 									\
		--target=$(TARGET) 												\
		--with-sysroot 													\
		--disable-nls 													\
		--disable-werror
	$(MAKE) -C $(BINUTILS_BUILD) -j $(CPU_CORES)
	$(MAKE) -C $(BINUTILS_BUILD) install

toolchain_gcc: toolchain_binutils toolchain_always
	cd $(TOOLCHAIN_DIRECTORY); wget $(GCC_URL)
	cd $(TOOLCHAIN_DIRECTORY); tar -xf gcc-$(GCC_VERSION).tar.xz
	cd $(TOOLCHAIN_DIRECTORY); rm -f gcc-$(GCC_VERSION).tar.xz
	\
	cd $(TOOLCHAIN_DIRECTORY)/gcc-$(GCC_VERSION); ./contrib/download_prerequisites
	mkdir $(GCC_BUILD)
	cd $(GCC_BUILD); ../gcc-$(GCC_VERSION)/configure 	\
		--host=$(SYSTEM_TRIPLET)						\
		--build=$(SYSTEM_TRIPLET)						\
		--prefix="$(TOOLCHAIN_PREFIX)"					\
		--target=$(TARGET) 								\
		--disable-nls			 						\	
		--enable-languages=c,c++ 						\
		--without-headers
	$(MAKE) -C $(GCC_BUILD) all-gcc all-target-libgcc -j $(CPU_CORES)
	$(MAKE) -C $(GCC_BUILD) install-gcc install-target-libgcc

toolchain_always:
	mkdir -p $(TOOLCHAIN_DIRECTORY)
	sudo apt-get update
	sudo apt-get install gcc-multilib

.PHONY: build_toolchain toolchain_binutils toolchain_gcc