export ASM = nasm
export TARGET_ASMFLAGS =
export CLFAGS = -std=c99 -g
export CC = gcc
export CXX = g++
export LD = gcc
export LIBS =

export TARGET = i686-elf
export TARGET_ASM = nasm
export TARGET_ASMFLAGS =
export TARGET_CLFAGS=-std = c99 -g
export TARGET_CC = $(TARGET)-gcc
export TARGET_CXX = $(TARGET)-g++
export TARGET_LINKFLAGS =
export TARGET_LD = $(TARGET)-gcc
export TARGET_LIBS =

export ROOT_DIR = $(abspath .)
export BUILD_DIR = $(abspath build)
export TOOLS_DIR = $(abspath tools)

export TOOLCHAIN_DIRECTORY = .toolchain
export TOOLCHAIN_PREFIX = $(abspath $(TOOLCHAIN_DIRECTORY)/$(TARGET))

export PATH := $(PATH):$(TOOLCHAIN_PREFIX)/bin

BINUTILS_VERSION = 2.37
BINUTILS_URL = https://ftp.gnu.org/gnu/binutils/binutils-$(BINUTILS_VERSION).tar.xz

GCC_VERSION = 11.1.0
GCC_URL = https://ftp.gnu.org/gnu/gcc/gcc-$(GCC_VERSION)/gcc-$(GCC_VERSION).tar.xz
