# Makefile for Unikraft
#
# Copyright (C) 1999-2005 by Erik Andersen <andersen@codepoet.org>
# Copyright (C) 2006-2014 by the Buildroot developers <buildroot@uclibc.org>
# Copyright (C) 2014-2016 by the Buildroot developers <buildroot@buildroot.org>
# Copyright (C) 2016-2017 by NEC Europe Ltd. <simon.kuenzer@neclab.eu>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#

HOSTOSENV := $(shell uname)

# Set initial and basic tools that we need to operate
ifeq ($(HOSTOSENV),Darwin)
SED := gsed
else
SED := sed
endif

# Trick for always running with a fixed umask
UMASK = 0022
ifneq ($(shell umask),$(UMASK))
.PHONY: _all $(MAKECMDGOALS)

$(MAKECMDGOALS): _all
	@:

_all:
	@umask $(UMASK) && $(MAKE) --no-print-directory $(MAKECMDGOALS)

else # umask

# This is our default rule, so must come first
.PHONY: all
all:

# Disable built-in rules
.SUFFIXES:

# Enable secondary expansion
.SECONDEXPANSION:

# Save running make version
RUNNING_MAKE_VERSION := $(MAKE_VERSION)

# Check for minimal make version (note: this check will break at make 10.x)
MIN_MAKE_VERSION = 4.1
ifneq ($(firstword $(sort $(RUNNING_MAKE_VERSION) $(MIN_MAKE_VERSION))),$(MIN_MAKE_VERSION))
ifneq ($(HOSTOSENV),Darwin)
$(error You have make '$(RUNNING_MAKE_VERSION)' installed. GNU make >= $(MIN_MAKE_VERSION) is required)
else
$(error We need GNU make >= $(MIN_MAKE_VERSION). It can be installed with 'brew install make'. Retry with: 'gmake $(MAKECMDGOALS)')
endif
endif

# Strip quotes and then whitespaces
qstrip = $(strip $(subst ",,$(1)))
#"))

# Variables for use in Make constructs
comma := ,
empty :=
space := $(empty) $(empty)
plus  := $(call qstrip,"+")

# bash prints the name of the directory on 'cd <dir>' if CDPATH is
# set, so unset it here to not cause problems. Notice that the export
# line doesn't affect the environment of $(shell ..) calls, so
# explictly throw away any output from 'cd' here.
export CDPATH :=

# To put more focus on warnings, be less verbose as default
# Use 'make V=1' to see the full commands
# Set KBUILD_VERBOSE and Q to quiet mode
KBUILD_VERBOSE := 0
Q := @

ifeq ("$(origin V)", "command line")
  BUILD_VERBOSE = $(V)
endif
ifndef BUILD_VERBOSE
  BUILD_VERBOSE = 0
endif

ifneq ($(BUILD_VERBOSE),0)
  KBUILD_VERBOSE := 1
  Q :=
endif

# Enable warnings about undefined variables
# with verbosity level 2
ifeq ($(BUILD_VERBOSE),2)
MAKEFLAGS+=--warn-undefined-variables
endif

# Helper that shows an `info` message only
# when verbose mode is on
# verbose_info $verbosemessage
ifneq ($(BUILD_VERBOSE),0)
verbose_info = $(info $(1))
else
verbose_info =
endif

# Use current directory as base
CONFIG_UK_BASE ?= $(CURDIR)
override CONFIG_UK_BASE := $(realpath $(CONFIG_UK_BASE))
ifeq ($(CONFIG_UK_BASE),)
$(error "Invalid base directory (CONFIG_UK_BASE)")
endif

# parameter A: APP_DIR ###
# Set A variable if not already done on the command line;
ifneq ("$(origin A)", "command line")
override A := $(CONFIG_UK_BASE)
else
ifeq ("$(filter /%,$(A))", "")
$(error Path to app directory (A) is not absolute)
endif
endif
# Remove the trailing '/.'
# Also remove the trailing '/' the user can set when on the command line.
override A := $(realpath $(patsubst %/,%,$(patsubst %.,%,$(A))))
ifeq ($(A),)
$(error Invalid app directory (A))
endif
override CONFIG_UK_APP   := $(A)
override APP_DIR  := $(A)
override APP_BASE := $(A)

# parameter O: BUILD_DIR ###
# Use O variable if set on the command line, otherwise use $(A)/build;
ifneq ("$(origin O)", "command line")
_O := $(APP_BASE)/build
else
ifeq ("$(filter /%,$(O))", "")
$(error Path to output directory (O) is not absolute)
endif
_O := $(realpath $(dir $(O)))/$(notdir $(O))
endif
BUILD_DIR := $(shell mkdir -p $(_O) && cd $(_O) >/dev/null && pwd)
$(if $(BUILD_DIR),, $(error could not create directory "$(_O)"))
BUILD_DIR := $(realpath $(patsubst %/,%,$(patsubst %.,%,$(BUILD_DIR))))
override O := $(BUILD_DIR)

# parameter C: UK_CONFIG ###
# Use C variable if set on the command line, otherwise use $(A)/.config;
ifneq ("$(origin C)", "command line")
ifeq ("$(origin C)", "undefined")
override C := $(CONFIG_UK_APP)/.config
endif
else
ifeq ("$(filter /%,$(C))", "")
$(error Path to configuration file (C) is not absolute)
endif
override C := $(realpath $(dir $(C)))/$(notdir $(C))
endif
UK_CONFIG  := $(C)
CONFIG_DIR := $(dir $(C))
# As UK_CONFIG could be different files, always assume it has a newer version
.PHONY: $(UK_CONFIG)

# EPLAT_DIR (list of external platform libraries)
# Retrieved from P variable from the command line (paths separated by colon)
ifeq ("$(origin P)", "command line")
$(foreach E,$(subst :, ,$(P)), \
$(if $(filter /%,$(E)),,$(error Path to external platform "$(E)" (P) is not absolute));\
$(if $(wildcard $(E)), \
	$(eval EPLAT_DIR += $(E)) \
, $(if $(wildcard $(CONFIG_UK_BASE)/$(E)),\
	$(eval EPLAT_DIR += $(CONFIG_UK_BASE)/$(E)), \
	$(error Cannot find platform library: $(E)) \
   ) \
) \
)
endif
EPLAT_DIR := $(realpath $(patsubst %/,%,$(patsubst %.,%,$(EPLAT_DIR))))

# ELIB_DIR (list of external libraries)
# Retrieved from L variable from the command line (paths separated by colon)
ifeq ("$(origin L)", "command line")
# library path exists?
$(foreach E,$(subst :, ,$(L)), \
$(if $(filter /%,$(E)),,$(error Path to external library "$(E)" (L) is not absolute));\
$(if $(wildcard $(E)), \
	$(eval ELIB_DIR += $(E)) \
, $(if $(wildcard $(CONFIG_UK_BASE)/$(E)),\
	$(eval ELIB_DIR += $(CONFIG_UK_BASE)/$(E)), \
	$(error Cannot find library: $(E)) \
   )\
) \
)
endif
ELIB_DIR := $(realpath $(patsubst %/,%,$(patsubst %.,%,$(ELIB_DIR))))

$(call verbose_info,* Unikraft base:      $(CONFIG_UK_BASE))
$(call verbose_info,* Configuration:      $(UK_CONFIG))
$(call verbose_info,* Application base:   $(CONFIG_UK_APP))
$(call verbose_info,* External platforms: [ $(EPLAT_DIR) ])
$(call verbose_info,* External libraries: [ $(ELIB_DIR) ])
$(call verbose_info,* Build output:       $(BUILD_DIR))

build_dir_make  := 0
ifneq ($(BUILD_DIR),$(UK_BASE))
	build_dir_make := 1;
else
	sub_make_exec := 1;
endif

# KConfig settings

CONFIG_UK_PLAT        := $(CONFIG_UK_BASE)/plat/
CONFIG_UK_LIB         := $(CONFIG_UK_BASE)/lib/
CONFIG_UK_DRIV        := $(CONFIG_UK_BASE)/drivers/
CONFIG_CONFIG_IN      := $(CONFIG_UK_BASE)/Config.uk
CONFIG                := $(CONFIG_UK_BASE)/support/kconfig
CONFIGLIB	      := $(CONFIG_UK_BASE)/support/kconfiglib
UK_CONFIG_OUT         := $(BUILD_DIR)/config
UK_GENERATED_INCLUDES := $(BUILD_DIR)/include
KCONFIG_INCLUDES_DIR  := $(UK_GENERATED_INCLUDES)/uk/bits
KCONFIG_DIR           := $(BUILD_DIR)/kconfig
UK_FIXDEP             := $(KCONFIG_DIR)/fixdep
KCONFIG_AUTOCONFIG    := $(KCONFIG_DIR)/auto.conf
KCONFIG_TRISTATE      := $(KCONFIG_DIR)/tristate.config
KCONFIG_AUTOHEADER    := $(KCONFIG_INCLUDES_DIR)/config.h
KCONFIG_LIB_BASE      := $(CONFIG_UK_BASE)/lib
KCONFIG_ELIB_DIRS     := $(L)
KCONFIG_PLAT_BASE     := $(CONFIG_UK_BASE)/plat
KCONFIG_EPLAT_DIRS    := $(P)
KCONFIG_DRIV_BASE     := $(CONFIG_UK_BASE)/drivers
ifneq ($(CONFIG_UK_BASE),$(CONFIG_UK_APP))
KCONFIG_EAPP_DIR      := $(CONFIG_UK_APP)
else
KCONFIG_EAPP_DIR      :=
endif

# Makefile support scripts
SCRIPTS_DIR := $(CONFIG_UK_BASE)/support/scripts

# # Set and export the version string
$(call verbose_info,Including $(CONFIG_UK_BASE)/version.mk...)
include $(CONFIG_UK_BASE)/version.mk

# Compute the full local version string so packages can use it as-is
# Need to export it, so it can be got from environment in children (eg. mconf)
ifdef UK_EXTRAVERSION
export UK_FULLVERSION := $(UK_VERSION).$(UK_SUBVERSION).$(UK_EXTRAVERSION)$(shell cd $(CONFIG_UK_BASE); $(SCRIPTS_DIR)/gitsha1)
else
export UK_FULLVERSION := $(UK_VERSION).$(UK_SUBVERSION)$(shell cd $(CONFIG_UK_BASE); $(SCRIPTS_DIR)/gitsha1)
endif

export DATE := $(shell date +%Y%m%d)

# Makefile targets
null_targets		:= print-version print-vars help
nokconfig_targets       := properclean distclean $(null_targets)
noconfig_targets	:= ukconfig menuconfig nconfig gconfig xconfig config \
			   oldconfig randconfig \
			   defconfig %_defconfig allyesconfig allnoconfig \
			   silentoldconfig \
			   release olddefconfig \
			   scriptconfig iscriptconfig kmenuconfig guiconfig \
			   dumpvarsconfig \
			   $(nokconfig_targets)

# we want bash as shell
SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	 else if [ -x /bin/bash ]; then echo /bin/bash; \
	 else echo sh; fi; fi)

# basic tools
RM    := rm -f
MV    := mv -f
CP    := cp -f
MKDIR := mkdir
TOUCH := touch
XARGS := xargs

# kconfig uses CONFIG_SHELL
CONFIG_SHELL := $(SHELL)
export SHELL CONFIG_SHELL Q KBUILD_VERBOSE

################################################################################
# Create minimal necessary folder structure
################################################################################
# Create them before trying to include a .config
ifeq ($(filter $(nokconfig_targets),$(MAKECMDGOALS)),)
$(if $(shell $(MKDIR) -pv $(KCONFIG_DIR) && cd $(KCONFIG_DIR) >/dev/null && pwd),,\
	$(error Could not create $(KCONFIG_DIR)))
$(if $(shell $(MKDIR) -pv $(UK_GENERATED_INCLUDES) && cd $(KCONFIG_DIR) >/dev/null && pwd),,\
	$(error Could not create $(UK_GENERATED_INCLUDES)))
$(if $(shell $(MKDIR) -pv $(KCONFIG_INCLUDES_DIR) && cd $(KCONFIG_DIR) >/dev/null && pwd),,\
	$(error Could not create $(KCONFIG_INCLUDES_DIR)))
endif

################################################################################
# .config
################################################################################
# Initialize important internal variables
UK_FETCH:=
UK_FETCH-y:=
UK_PREPARE:=
UK_PREPARE-y:=
UK_PREPROCESS:=
UK_PREPROCESS-y:=
UK_PLATS:=
UK_PLATS-y:=
UK_LIBS:=
UK_LIBS-y:=
UK_ALIBS:=
UK_ALIBS-y:=
UK_OLIBS:=
UK_OLIBS-y:=
UK_SRCS:=
UK_SRCS-y:=
UK_DEPS:=
UK_DEPS-y:=
UK_OBJS:=
UK_OBJS-y:=
UK_IMAGES:=
UK_IMAGES-y:=
UK_CLEAN :=
UK_CLEAN-y :=
ARCHFLAGS :=
ARCHFLAGS-y :=
ISR_ARCHFLAGS :=
ISR_ARCHFLAGS-y :=
COMPFLAGS :=
COMPFLAGS-y :=
ASFLAGS :=
ASFLAGS-y :=
ASINCLUDES :=
ASINCLUDES-y :=
CFLAGS :=
CFLAGS-y :=
CINCLUDES :=
CINCLUDES-y :=
CXXFLAGS :=
CXXFLAGS-y :=
CXXINCLUDES :=
CXXINCLUDES-y :=
GOCFLAGS :=
GOCFLAGS-y :=
RUSTCFLAGS :=
RUSTCFLAGS-y :=
GOCINCLUDES :=
GOCINCLUDES-y :=
DBGFLAGS :=
DBGFLAGS-y :=
LDFLAGS :=
LDFLAGS-y :=
IMAGE_LDFLAGS :=
IMAGE_LDFLAGS-y :=
EACHOLIB_SRCS :=
EACHOLIB_SRCS-y :=
EACHOLIB_OBJS :=
EACHOLIB_OBJS-y :=
EACHOLIB_ALIBS :=
EACHOLIB_ALIBS-y :=
EACHOLIB_LOCALS :=
EACHOLIB_LOCALS-y :=

# Pull in the user's configuration file
ifeq ($(filter $(noconfig_targets),$(MAKECMDGOALS)),)
ifneq ("$(wildcard $(UK_CONFIG))","")
$(call verbose_info,Including $(UK_CONFIG)...)
-include $(UK_CONFIG)
UK_HAVE_DOT_CONFIG := y
endif
endif

# parameter N: UK_NAME ###
# # Use N variable if set on the command line, otherwise use directory name
ifeq ("$(origin N)", "command line")
CONFIG_UK_NAME := $(N)
else
CONFIG_UK_NAME ?= $(notdir $(APP_DIR))
endif

# remove quotes from CONFIG_UK_NAME
CONFIG_UK_NAME := $(call qstrip,$(CONFIG_UK_NAME))
export CONFIG_UK_NAME

################################################################################
# Host compiler and linker tools
################################################################################
ifndef HOSTAR
HOSTAR := ar
endif
ifndef HOSTAS
HOSTAS := as
endif
ifndef HOSTCC
HOSTCC := gcc
HOSTCC := $(shell which $(HOSTCC) || type -p $(HOSTCC) || echo gcc)
endif
HOSTCC_NOCCACHE := $(HOSTCC)
ifndef HOSTCXX
HOSTCXX := g++
HOSTCXX := $(shell which $(HOSTCXX) || type -p $(HOSTCXX) || echo g++)
endif
HOSTCXX_NOCCACHE := $(HOSTCXX)
ifndef HOSTCPP
HOSTCPP := cpp
endif
ifndef HOSTLD
HOSTLD := ld
endif
ifndef HOSTLN
HOSTLN := ln
endif
ifndef HOSTNM
HOSTNM := nm
endif
ifndef HOSTOBJCOPY
HOSTOBJCOPY := objcopy
endif
ifndef HOSTRANLIB
HOSTRANLIB := ranlib
endif
HOSTAR		:= $(shell which $(HOSTAR) || type -p $(HOSTAR) || echo ar)
HOSTAS		:= $(shell which $(HOSTAS) || type -p $(HOSTAS) || echo as)
HOSTCPP		:= $(shell which $(HOSTCPP) || type -p $(HOSTCPP) || echo cpp)
HOSTLD		:= $(shell which $(HOSTLD) || type -p $(HOSTLD) || echo ld)
HOSTLN		:= $(shell which $(HOSTLN) || type -p $(HOSTLN) || echo ln)
HOSTNM		:= $(shell which $(HOSTNM) || type -p $(HOSTNM) || echo nm)
HOSTOBJCOPY	:= $(shell which $(HOSTOBJCOPY) || type -p $(HOSTOBJCOPY) || echo objcopy)
HOSTRANLIB	:= $(shell which $(HOSTRANLIB) || type -p $(HOSTRANLIB) || echo ranlib)
HOSTCC_VERSION	:= $(shell $(HOSTCC_NOCCACHE) --version | \
		   $(SED) -n -r 's/^.* ([0-9]*)\.([0-9]*)\.([0-9]*)[ ]*.*/\1 \2/p')

# For gcc >= 5.x, we only need the major version.
ifneq ($(firstword $(HOSTCC_VERSION)),4)
HOSTCC_VERSION	:= $(firstword $(HOSTCC_VERSION))
endif

# Determine the userland we are running on.
#
export HOSTARCH := $(shell LC_ALL=C $(HOSTCC_NOCCACHE) -v 2>&1 | \
		   $(SED) -e '/^Target: \([^-]*\).*/!d' \
		       -e 's//\1/' \
		       -e 's/i.86/x86/' \
		       -e 's/sun4u/sparc64/' \
		       -e 's/arm64.*/arm64/' -e 's/aarch64.*/arm64/' \
		       -e '/arm64/! s/arm.*/arm/' \
		       -e 's/sa110/arm/' \
		       -e 's/ppc64/powerpc64/' \
		       -e 's/ppc/powerpc/' \
		       -e 's/macppc/powerpc/' \
		       -e 's/sh.*/sh/' )
export HOSTAR HOSTAS HOSTCC HOSTCC_VERSION HOSTCXX HOSTLD HOSTARCH
export HOSTCC_NOCCACHE HOSTCXX_NOCCACHE

################################################################################
# Makefile helpers
################################################################################
$(call verbose_info,Including $(CONFIG_UK_BASE)/support/build/Makefile.rules...)
include $(CONFIG_UK_BASE)/support/build/Makefile.rules

# We need to include this file early (before any rule is defined)
# but after we have tried to load a .config and after having our tools defined
$(foreach _M,$(wildcard $(addsuffix Makefile.rules,\
	   $(CONFIG_UK_BASE)/arch/ $(CONFIG_UK_BASE)/arch/*/ \
	   $(CONFIG_UK_BASE)/plat/*/ $(CONFIG_UK_BASE)/lib/*/ \
	   $(CONFIG_UK_BASE)/drivers/*/ $(CONFIG_UK_BASE)/drivers/*/*/ \
	   $(addsuffix /,$(ELIB_DIR)) $(APP_DIR)/)), \
		$(eval $(call verbose_include,$(_M))) \
)

################################################################################
# Clean targets that do not have any dependency on a configuration
################################################################################
# Declare them before we depend on having .config
properclean:
	$(call verbose_cmd,RM,$(notdir $(BUILD_DIR)),$(RM) -r \
		$(BUILD_DIR))

distclean: properclean
	$(call verbose_cmd,RM,config,$(RM) \
		$(UK_CONFIG) $(UK_CONFIG).old \
		$(CONFIG_DIR)/.$(notdir $(UK_CONFIG)).tmp \
		$(CONFIG_DIR)/.auto.deps)

.PHONY: distclean properclean

################################################################################
# Unikraft Architecture
################################################################################
# Set target archicture as set in config
$(eval $(call verbose_include,$(CONFIG_UK_BASE)/arch/Arch.uk))
ifeq ($(CONFIG_UK_ARCH),)
# Set target archicture as set in environment
ifneq ($(ARCH),)
export CONFIG_UK_ARCH	?= $(shell echo "$(call qstrip,$(ARCH))" | \
		   $(SED) -e "s/-.*//" \
		       -e 's//\1/' \
		       -e 's/i.86/x86/' \
		       -e 's/sun4u/sparc64/' \
		       -e 's/arm64.*/arm64/' -e 's/aarch64.*/arm64/' \
		       -e '/arm64/! s/arm.*/arm/' \
		       -e 's/sa110/arm/' \
		       -e 's/ppc64/powerpc64/' \
		       -e 's/ppc/powerpc/' \
		       -e 's/macppc/powerpc/' \
		       -e 's/sh.*/sh/' )
else
# Nothing set, use detected host architecture
export CONFIG_UK_ARCH	?= $(shell echo "$(HOSTARCH)" | \
		   $(SED) -e "s/-.*//" \
		       -e 's//\1/' \
		       -e 's/i.86/x86/' \
		       -e 's/sun4u/sparc64/' \
		       -e 's/arm64.*/arm64/' -e 's/aarch64.*/arm64/' \
		       -e '/arm64/! s/arm.*/arm/' \
		       -e 's/sa110/arm/' \
		       -e 's/ppc64/powerpc64/' \
		       -e 's/ppc/powerpc/' \
		       -e 's/macppc/powerpc/' \
		       -e 's/sh.*/sh/' )
endif
endif
override ARCH := $(CONFIG_UK_ARCH)
export CONFIG_UK_ARCH ARCH

export UK_FAMILY ?= $(shell echo "$(CONFIG_UK_ARCH)" | \
		   $(SED) -e "s/-.*//" \
		       -e 's//\1/' \
		       -e 's/x86.*/x86/' \
		       -e 's/sparc64/sparc/' \
		       -e 's/arm.*/arm/' \
		       -e 's/powerpc.*/powerpc/' \
		       -e 's/sh.*/sh/' )


# Quick-check if architecture exists
ifeq ($(filter $(null_targets),$(MAKECMDGOALS)),)
ifeq ($(wildcard $(CONFIG_UK_BASE)/arch/$(UK_FAMILY)/$(ARCH)/Makefile.uk),)
$(error Target architecture ($(ARCH)) is currently not supported (could not find $(CONFIG_UK_BASE)/arch/$(UK_FAMILY)/$(ARCH)/Makefile.uk).)
endif

ifeq ($(wildcard $(CONFIG_UK_BASE)/arch/$(UK_FAMILY)/$(ARCH)/Compiler.uk),)
$(error Target architecture ($(ARCH)) is currently not supported (could not find $(CONFIG_UK_BASE)/arch/$(UK_FAMILY)/$(ARCH)/Compiler.uk).)
endif
endif

################################################################################
# Compiler and linker tools
################################################################################
ifeq ($(sub_make_exec), 1)
ifeq ($(UK_HAVE_DOT_CONFIG),y)
# Hide troublesome environment variables from sub processes
unexport CONFIG_CROSS_COMPILE
unexport CONFIG_LLVM_TARGET_ARCH
unexport CONFIG_COMPILER
#unexport CC
#unexport LD
#unexport AR
#unexport CXX
#unexport CPP
unexport RANLIB
unexport CFLAGS
unexport CXXFLAGS
unexport ARFLAGS
unexport GREP_OPTIONS
unexport TAR_OPTIONS
unexport CONFIG_SITE
unexport QMAKESPEC
unexport TERMINFO
unexport MACHINE
#unexport O

# CONFIG_CROSS_COMPILE specify the prefix used for all executables used
# during compilation. Only gcc and related bin-utils executables
# are prefixed with $(CONFIG_CROSS_COMPILE).
# CONFIG_CROSS_COMPILE can be set on the command line
# make CROSS_COMPILE=ia64-linux-
# Alternatively CONFIG_CROSS_COMPILE can be set in the environment.
# A third alternative is to store a setting in .config so that plain
# "make" in the configured kernel build directory always uses that.
# Default value for CONFIG_CROSS_COMPILE is not to prefix executables
# Note: Some architectures assign CONFIG_CROSS_COMPILE in their arch/*/Makefile.uk

ifneq ("$(origin CROSS_COMPILE)","undefined")
CONFIG_CROSS_COMPILE := $(CROSS_COMPILE:"%"=%)
endif

ifneq ("$(origin LLVM_TARGET_ARCH)","undefined")
CONFIG_LLVM_TARGET_ARCH := $(LLVM_TARGET_ARCH:"%"=%)
endif

ifneq ("$(origin COMPILER)","undefined")
	CONFIG_COMPILER := $(COMPILER:"%"=%)
else
	CONFIG_COMPILER := gcc
endif


$(eval $(call verbose_include,$(CONFIG_UK_BASE)/arch/$(UK_FAMILY)/Compiler.uk))

# Make variables (CC, etc...)
LD		:= $(CONFIG_CROSS_COMPILE)$(CONFIG_COMPILER)
CC		:= $(CONFIG_CROSS_COMPILE)$(CONFIG_COMPILER)
CPP		:= $(CC)
CXX		:= $(CPP)
GOC		:= $(CONFIG_CROSS_COMPILE)gccgo
# We use rustc because the gcc frontend is experimental and missing features such
# as borrowing checking
ifneq ("$(origin LLVM_TARGET_ARCH)","undefined")
RUSTC		:= rustc --target=$(CONFIG_LLVM_TARGET_ARCH)
else
RUSTC		:= rustc
endif
AS		:= $(CC)
AR		:= $(CONFIG_CROSS_COMPILE)gcc-ar
NM		:= $(CONFIG_CROSS_COMPILE)gcc-nm
READELF		:= $(CONFIG_CROSS_COMPILE)readelf
STRIP		:= $(CONFIG_CROSS_COMPILE)strip
OBJCOPY		:= $(CONFIG_CROSS_COMPILE)objcopy
OBJDUMP		:= $(CONFIG_CROSS_COMPILE)objdump
M4		:= m4
AR		:= ar
CAT		:= cat
# Prefer using GNU AWK because of provided error messages on script errors
ifeq (, $(shell which gawk))
AWK		:= awk
else
AWK		:= gawk --lint
endif
ifeq ($(HOSTOSENV),Darwin)
GREP		:= ggrep
READLINK	:= greadlink
DIRNAME		:= gdirname
else
GREP		:= grep
READLINK	:= readlink
DIRNAME		:= dirname
endif
YACC		:= bison
LEX     	:= flex
PATCH		:= patch
GZIP		:= gzip
TAR		:= tar
UNZIP		:= unzip -qq -u
GIT		:= git
WGET		:= wget
PYTHON          := python3
SHA1SUM		:= sha1sum -b
SHA256SUM	:= sha256sum -b
SHA512SUM	:= sha512sum -b
MD5SUM		:= md5sum -b
DTC		:= dtc
# Time requires the full path so that subarguments are handled correctly
TIME		:= $(shell which time)
LIFTOFF		:= liftoff -e -s
override ARFLAGS:= rcs

CC_INFO := $(shell $(CONFIG_UK_BASE)/support/build/cc-version.sh $(CC))
CC_NAME := $(word 1,$(CC_INFO))

# Retrieve GCC major and minor number from CC_VERSION. They would be used
# to select correct optimization parameters for target CPUs.
CC_VER_MAJOR   := $(word 2,$(subst ., ,$(CC_INFO)))
CC_VER_MINOR   := $(word 3,$(subst ., ,$(CC_INFO)))
CC_VERSION     := $(CC_VER_MAJOR).$(CC_VER_MINOR)

ifeq ($(call have_clang),y)
ifeq ("$(ARCH)", "arm64")
ARCHFLAGS             += --target=aarch64-none-elf
ISR_ARCHFLAGS         += --target=aarch64-none-elf
endif
endif

ASFLAGS		+= -DCC_VERSION=$(CC_VERSION)
CFLAGS		+= -DCC_VERSION=$(CC_VERSION)
CXXFLAGS	+= -DCC_VERSION=$(CC_VERSION)
GOCFLAGS	+= -DCC_VERSION=$(CC_VERSION)

# Add user supplied flags as the last assignments
ASFLAGS  += $(UK_ASFLAGS)
CFLAGS   += $(UK_CFLAGS)
CXXFLAGS += $(UK_CXXFLAGS)
GOCFLAGS += $(UK_GOCFLAGS)
LDFLAGS  += $(UK_LDFLAGS)

ASINCLUDES            += -I$(UK_GENERATED_INCLUDES)
CINCLUDES             += -I$(UK_GENERATED_INCLUDES)
CXXINCLUDES           += -I$(UK_GENERATED_INCLUDES)
GOCINCLUDES           += -I$(UK_GENERATED_INCLUDES)

################################################################################
# Build rules
################################################################################
# external application
ifneq ($(CONFIG_UK_BASE),$(CONFIG_UK_APP))
$(eval $(call _import_lib,$(CONFIG_UK_APP)));
endif

# internal libraries
$(eval $(call verbose_include,$(CONFIG_UK_BASE)/lib/Makefile.uk))

# external libraries
$(foreach E,$(ELIB_DIR), \
	$(eval $(call _import_lib,$(E))); \
)
# architecture library
$(eval $(call _import_lib,$(CONFIG_UK_BASE)/arch/$(UK_FAMILY)))
# drivers
$(eval $(call verbose_include,$(CONFIG_UK_BASE)/drivers/Makefile.uk))
# internal platform libraries
$(eval $(call verbose_include,$(CONFIG_UK_BASE)/plat/Makefile.uk))
# external platform libraries
# NOTE: We include them after internal platform libs so that also base variables
#       provided with /plat/Makefile.uk are populated
$(foreach E,$(EPLAT_DIR), \
	$(eval $(call _import_lib,$(E))); \
)
$(eval $(call verbose_include,$(CONFIG_UK_BASE)/Makefile.uk)) # Unikraft base

ifeq ($(call have_clang),y)
$(call error_if_clang_version_lt,9,0)
endif

ifeq ($(call have_gcc),y)
$(call error_if_gcc_version_lt,7,0)
endif

ifeq ($(call qstrip,$(UK_PLATS) $(UK_PLATS-y)),)
$(warning You did not choose any target platform.)
$(error Please choose at least one target platform in the configuration!)
endif
ifneq ($(CONFIG_HAVE_BOOTENTRY),y)
$(error You did not select a library that handles bootstrapping! (e.g., ukboot))
endif

ifeq ($(CONFIG_OPTIMIZE_LTO), y)
ifeq ($(call have_gcc),y)
ifneq ($(call gcc_version_ge,6,1),y)
$(error Your gcc version does not support incremental link time optimisation)
endif
endif
endif

# Generate build rules
$(eval $(call verbose_include,$(CONFIG_UK_BASE)/support/build/Makefile.build))
$(foreach _M,$(wildcard $(addsuffix Makefile.build,\
	   $(CONFIG_UK_BASE)/lib/*/ $(CONFIG_UK_BASE)/plat/*/ \
	   $(addsuffix /,$(ELIB_DIR)) $(APP_DIR)/)), \
		$(eval $(call verbose_include,$(_M))) \
)

# Include source dependencies
ifneq ($(call qstrip,$(UK_DEPS) $(UK_DEPS-y)),)
$(foreach _D,$(UK_DEPS) $(UK_DEPS-y),\
 $(eval $(call verbose_include_try,$(_D))) \
)
endif

# include Makefile for platform linking (`Linker.uk`)
$(foreach plat,$(UK_PLATS),$(eval $(call _import_linker,$(plat))))

.PHONY: prepare preprocess image libs objs clean

fetch: $(UK_FETCH) $(UK_FETCH-y)

# Copy current configuration in order to detect changes
$(UK_CONFIG_OUT): $(UK_CONFIG)
	$(call verbose_cmd,CP,config,$(CP) \
		$(UK_CONFIG) \
		$(UK_CONFIG_OUT))

prepare: $(KCONFIG_AUTOHEADER) $(UK_CONFIG_OUT) $(UK_PREPARE) $(UK_PREPARE-y)
prepare: $(UK_FIXDEP) | fetch

preprocess: $(UK_PREPROCESS) $(UK_PREPROCESS-y) | prepare

objs: $(UK_OBJS) $(UK_OBJS-y)

libs: $(UK_ALIBS) $(UK_ALIBS-y) $(UK_OLIBS) $(UK_OLIBS-y)

images: $(UK_DEBUG_IMAGES) $(UK_DEBUG_IMAGES-y) $(UK_IMAGES) $(UK_IMAGES-y)

GDB_HELPER_LINKS := $(addsuffix .gdb.py,$(UK_DEBUG_IMAGES) $(UK_DEBUG_IMAGES-y))
$(GDB_HELPER_LINKS):
	$(call verbose_cmd,LN,$(notdir $@),$(HOSTLN) -sf uk-gdb.py $@)

SCRIPTS_DIR_BACKSLASHED = $(subst /,\/,$(SCRIPTS_DIR))
$(BUILD_DIR)/uk-gdb.py: $(SCRIPTS_DIR)/uk-gdb.py
	$(call verbose_cmd,GEN,$(notdir $@), \
	sed '/scripts_dir = / s/os.path.dirname(os.path.realpath(__file__))/"$(SCRIPTS_DIR_BACKSLASHED)"/g' $^ > $@)

gdb_helpers: $(GDB_HELPER_LINKS) $(BUILD_DIR)/uk-gdb.py

all: images gdb_helpers
################################################################################
# Cleanup rules
################################################################################
# Generate cleaning rules
$(eval $(call verbose_include,$(CONFIG_UK_BASE)/support/build/Makefile.clean))

clean-libs: $(addprefix clean-,\
	$(foreach P,$(UK_PLATS) $(UK_PLATS-y),\
	$(if $(call qstrip,$($(call uc,$(P))_LIBS) $($(call uc,$(P))_LIBS-y)),\
	$(foreach L,$($(call uc,$(P))_LIBS) $($(call uc,$(P))_LIBS-y), $(L)))) $(UK_LIBS) $(UK_LIBS-y))

clean: clean-libs
	$(call verbose_cmd,CLEAN,build/,$(RM) \
		$(UK_CONFIG_OUT) \
		$(call build_clean,\
			$(UK_DEBUG_IMAGES) $(UK_DEBUG_IMAGES-y) \
			$(UK_IMAGES) $(UK_IMAGES-y)) \
		$(GDB_HELPER_LINKS) $(BUILD_DIR)/uk-gdb.py \
		$(UK_CLEAN) $(UK_CLEAN-y))

else # !($(UK_HAVE_DOT_CONFIG),y)


$(filter %config,$(MAKECMDGOALS)): $(BUILD_DIR)/Makefile

## ukconfig
ukconfig: $(BUILD_DIR)/Makefile menuconfig

all: ukconfig

.PHONY: prepare image libs objs clean-libs clean ukconfig

fetch: ukconfig

prepare: ukconfig

preprocess: ukconfig

objs: ukconfig

libs: ukconfig

images: ukconfig

clean-libs clean:
	$(error Do not know which files to clean without having a configuration. Did you mean 'properclean' or 'distclean'?)

endif

.PHONY: print-vars print-libs print-objs print-srcs print-loc help outputmakefile list-defconfigs

# Configuration
# ---------------------------------------------------------------------------
HOSTCFLAGS = $(CFLAGS_FOR_BUILD)
export HOSTCFLAGS

ifeq ($(HOSTOSENV),Linux)
KCONFIG_TOOLS = conf mconf gconf nconf qconf
KCONFIG_TOOLS := $(addprefix $(KCONFIG_DIR)/,$(KCONFIG_TOOLS))

$(KCONFIG_TOOLS):
	$(call verbose_cmd,MKDIR,lxdialog,$(MKDIR) -p $(@D)/lxdialog)
	$(call verbose_cmd,MAKE,$(notdir $(CONFIG)),$(MAKE) \
	    --no-print-directory \
	    CC="$(HOSTCC_NOCCACHE)" HOSTCC="$(HOSTCC_NOCCACHE)" \
	    YACC="$(YACC)" LEX="$(LEX)" \
	    obj=$(@D) -C $(CONFIG) -f Makefile.br $(@))
endif

$(UK_FIXDEP):
	$(call verbose_cmd,MAKE,$(notdir $(@)),$(MAKE) \
	    --no-print-directory \
	    CC="$(HOSTCC_NOCCACHE)" HOSTCC="$(HOSTCC_NOCCACHE)" \
	    obj=$(@D) -C $(CONFIG) -f Makefile.br $(@))

DEFCONFIG = $(call qstrip,$(UK_DEFCONFIG))

# We don't want to fully expand UK_DEFCONFIG here, so Kconfig will
# recognize that if it's still at its default $(CONFIG_DIR)/defconfig
COMMON_CONFIG_ENV = \
	CONFIG_="CONFIG_" \
	KCONFIG_CONFIG="$(UK_CONFIG)" \
	KCONFIG_AUTOCONFIG="$(KCONFIG_AUTOCONFIG)" \
	KCONFIG_AUTOHEADER="$(KCONFIG_AUTOHEADER)" \
	KCONFIG_TRISTATE="$(KCONFIG_TRISTATE)" \
	HOST_ARCH="$(HOSTARCH)" \
	BUILD_DIR="$(BUILD_DIR)" \
	UK_BASE="$(CONFIG_UK_BASE)" \
	UK_APP="$(CONFIG_UK_APP)" \
	UK_CONFIG="$(UK_CONFIG)" \
	UK_FULLVERSION="$(UK_FULLVERSION)" \
	UK_CODENAME="$(UK_CODENAME)" \
	UK_ARCH="$(CONFIG_UK_ARCH)" \
	KCONFIG_DIR="$(KCONFIG_DIR)" \
	KCONFIG_LIB_BASE="$(KCONFIG_LIB_BASE)" \
	KCONFIG_ELIB_DIRS="$(KCONFIG_ELIB_DIRS)" \
	KCONFIG_PLAT_BASE="$(KCONFIG_PLAT_BASE)" \
	KCONFIG_EPLAT_DIRS="$(KCONFIG_EPLAT_DIRS)" \
	KCONFIG_DRIV_BASE="$(KCONFIG_DRIV_BASE)" \
	KCONFIG_EAPP_DIR="$(KCONFIG_EAPP_DIR)" \
	UK_NAME="$(CONFIG_UK_NAME)"

PHONY += scriptconfig scriptsyncconfig iscriptconfig kmenuconfig guiconfig \
		 dumpvarsconfig

KPYTHON := PYTHONPATH=$(UK_CONFIGLIB):$$PYTHONPATH $(PYTHON)

ifneq ($(filter scriptconfig,$(MAKECMDGOALS)),)
ifndef SCRIPT
$(error Use "make scriptconfig SCRIPT=<path to script> [SCRIPT_ARG=<argument>]")
endif
endif

scriptconfig:
	$(Q)$(COMMON_CONFIG_ENV) $(KPYTHON) $(SCRIPT) $(Kconfig) $(if $(SCRIPT_ARG),"$(SCRIPT_ARG)")

iscriptconfig:
	$(Q)$(COMMON_CONFIG_ENV) $(KPYTHON) -i -c \
	  "import kconfiglib; \
	   kconf = kconfiglib.Kconfig('$(UK_CONFIG)'); \
	   print('A Kconfig instance \'kconf\' for the architecture $(ARCH) has been created.')"

kmenuconfig:
	@$(COMMON_CONFIG_ENV) $(KPYTHON) $(CONFIGLIB)/menuconfig.py \
		$(CONFIG_CONFIG_IN)
	@$(COMMON_CONFIG_ENV) $(SCRIPTS_DIR)/configupdate $(UK_CONFIG) $(UK_CONFIG_OUT)

scriptsyncconfig:
	@$(COMMON_CONFIG_ENV) $(KPYTHON) $(CONFIGLIB)/genconfig.py \
		--sync-deps=$(BUILD_DIR)/include/config \
		--header-path=$(KCONFIG_AUTOHEADER) $(CONFIG_CONFIG_IN)
	@$(COMMON_CONFIG_ENV) $(SCRIPTS_DIR)/configupdate $(UK_CONFIG) $(UK_CONFIG_OUT)

guiconfig:
	@$(COMMON_CONFIG_ENV) $(KPYTHON) $(CONFIGLIB)/guiconfig.py $(CONFIG_CONFIG_IN)
	@$(SCRIPTS_DIR)/configupdate $(UK_CONFIG) $(UK_CONFIG_OUT)

dumpvarsconfig:
	$(Q)$(COMMON_CONFIG_ENV) $(KPYTHON) $(CONFIGLIB)/examples/dumpvars.py $(CONFIG_CONFIG_IN)
	@$(SCRIPTS_DIR)/configupdate $(UK_CONFIG) $(UK_CONFIG_OUT)

ifneq ($(HOSTOSENV),Linux)
# Use libkconfiglib for non-Linux hosts
# Compatibility wrappers:
menuconfig: kmenuconfig
nconfig: kmenuconfig
gconfig: guiconfig
xconfig: guiconfig

config:
	@$(COMMON_CONFIG_ENV) $(KPYTHON) $(CONFIGLIB)/genconfig.py --header-path $(KCONFIG_AUTOHEADER) $(CONFIG_CONFIG_IN)
	@$(COMMON_CONFIG_ENV) $(SCRIPTS_DIR)/configupdate $(UK_CONFIG) $(UK_CONFIG_OUT)

allyesconfig:
	@$(COMMON_CONFIG_ENV) $(KPYTHON) $(CONFIGLIB)/allyesconfig.py $(CONFIG_CONFIG_IN)
	@$(COMMON_CONFIG_ENV) $(SCRIPTS_DIR)/configupdate $(UK_CONFIG) $(UK_CONFIG_OUT)

allnoconfig:
	@$(COMMON_CONFIG_ENV) $(KPYTHON) $(CONFIGLIB)/allnoconfig.py $(CONFIG_CONFIG_IN)
	@$(COMMON_CONFIG_ENV) $(SCRIPTS_DIR)/configupdate $(UK_CONFIG) $(UK_CONFIG_OUT)

defconfig:
	@$(COMMON_CONFIG_ENV) $(KPYTHON) $(CONFIGLIB)/defconfig.py --kconfig $(CONFIG_CONFIG_IN) $(DEFCONFIG)
	@$(COMMON_CONFIG_ENV) $(SCRIPTS_DIR)/configupdate $(UK_CONFIG) $(UK_CONFIG_OUT)

savedefconfig:
	@$(COMMON_CONFIG_ENV) $(KPYTHON) $(CONFIGLIB)/savedefconfig.py --kconfig $(CONFIG_CONFIG_IN) --out $(DEFCONFIG)
ifeq ($(HOSTARCH),$(CONFIG_UK_ARCH))
	@# Make sure arch is stored in the file even if arch matches between host and config
	@echo "$(call ukarch_str2cfg,$(CONFIG_UK_ARCH))=y" >> $(DEFCONFIG)
endif

oldconfig:
	@$(COMMON_CONFIG_ENV) $(KPYTHON) $(CONFIGLIB)/oldconfig.py $(CONFIG_CONFIG_IN)
	@$(COMMON_CONFIG_ENV) $(SCRIPTS_DIR)/configupdate $(UK_CONFIG) $(UK_CONFIG_OUT)

# Regenerate $(KCONFIG_AUTOHEADER) whenever $(UK_CONFIG) changed
$(KCONFIG_AUTOHEADER): $(UK_CONFIG)
	@$(COMMON_CONFIG_ENV) $(KPYTHON) $(CONFIGLIB)/genconfig.py --header-path $(KCONFIG_AUTOHEADER) $(CONFIG_CONFIG_IN)
else
# Use traditional KConfig system on Linux
xconfig: $(KCONFIG_DIR)/qconf
	@$(COMMON_CONFIG_ENV) $< $(CONFIG_CONFIG_IN)
	@$(COMMON_CONFIG_ENV) $(SCRIPTS_DIR)/configupdate $(UK_CONFIG) $(UK_CONFIG_OUT)

gconfig: $(KCONFIG_DIR)/gconf
	@$(COMMON_CONFIG_ENV) srctree=$(CONFIG_UK_BASE) $< $(CONFIG_CONFIG_IN)
	@$(COMMON_CONFIG_ENV) $(SCRIPTS_DIR)/configupdate $(UK_CONFIG) $(UK_CONFIG_OUT)

menuconfig: $(KCONFIG_DIR)/mconf
	@$(COMMON_CONFIG_ENV) $< $(CONFIG_CONFIG_IN)
	@$(COMMON_CONFIG_ENV) $(SCRIPTS_DIR)/configupdate $(UK_CONFIG) $(UK_CONFIG_OUT)

nconfig: $(KCONFIG_DIR)/nconf
	@$(COMMON_CONFIG_ENV) $< $(CONFIG_CONFIG_IN)
	@$(COMMON_CONFIG_ENV) $(SCRIPTS_DIR)/configupdate $(UK_CONFIG) $(UK_CONFIG_OUT)

config: $(KCONFIG_DIR)/conf
	@$(COMMON_CONFIG_ENV) $< $(CONFIG_CONFIG_IN)
	@$(COMMON_CONFIG_ENV) $(SCRIPTS_DIR)/configupdate $(UK_CONFIG) $(UK_CONFIG_OUT)

# For the config targets that automatically select options, we pass
# SKIP_LEGACY=y to disable the legacy options. However, in that case
# no values are set for the legacy options so a subsequent oldconfig
# will query them. Therefore, run an additional olddefconfig.
oldconfig: $(KCONFIG_DIR)/conf
	@$(COMMON_CONFIG_ENV) $< --oldconfig $(CONFIG_CONFIG_IN)
	@$(COMMON_CONFIG_ENV) $(SCRIPTS_DIR)/configupdate $(UK_CONFIG) $(UK_CONFIG_OUT)

randconfig: $(KCONFIG_DIR)/conf
	@$(COMMON_CONFIG_ENV) SKIP_LEGACY=y $< --randconfig $(CONFIG_CONFIG_IN)
	@$(COMMON_CONFIG_ENV) $< --olddefconfig $(CONFIG_CONFIG_IN) >/dev/null
	@$(COMMON_CONFIG_ENV) $(SCRIPTS_DIR)/configupdate $(UK_CONFIG) $(UK_CONFIG_OUT)

allyesconfig: $(KCONFIG_DIR)/conf
	@$(COMMON_CONFIG_ENV) SKIP_LEGACY=y $< --allyesconfig $(CONFIG_CONFIG_IN)
	@$(COMMON_CONFIG_ENV) $< --olddefconfig $(CONFIG_CONFIG_IN) >/dev/null
	@$(COMMON_CONFIG_ENV) $(SCRIPTS_DIR)/configupdate $(UK_CONFIG) $(UK_CONFIG_OUT)

allnoconfig: $(KCONFIG_DIR)/conf
	@$(COMMON_CONFIG_ENV) SKIP_LEGACY=y $< --allnoconfig $(CONFIG_CONFIG_IN)
	@$(COMMON_CONFIG_ENV) $< --olddefconfig $(CONFIG_CONFIG_IN) >/dev/null
	@$(COMMON_CONFIG_ENV) $(SCRIPTS_DIR)/configupdate $(UK_CONFIG) $(UK_CONFIG_OUT)

syncconfig: $(KCONFIG_DIR)/conf
	@$(COMMON_CONFIG_ENV) $< --syncconfig $(CONFIG_CONFIG_IN)
	@$(COMMON_CONFIG_ENV) $(SCRIPTS_DIR)/configupdate $(UK_CONFIG) $(UK_CONFIG_OUT)

olddefconfig: $(KCONFIG_DIR)/conf
	@$(COMMON_CONFIG_ENV) $< --olddefconfig $(CONFIG_CONFIG_IN)
	@$(COMMON_CONFIG_ENV) $(SCRIPTS_DIR)/configupdate $(UK_CONFIG) $(UK_CONFIG_OUT)

defconfig: $(KCONFIG_DIR)/conf
	@$(COMMON_CONFIG_ENV) $< --defconfig$(if $(DEFCONFIG),=$(DEFCONFIG)) $(CONFIG_CONFIG_IN)
	@$(COMMON_CONFIG_ENV) $(SCRIPTS_DIR)/configupdate $(UK_CONFIG) $(UK_CONFIG_OUT)

# Override the UK_DEFCONFIG from COMMON_CONFIG_ENV with the new defconfig
%_defconfig: $(KCONFIG_DIR)/conf $(A)/configs/%_defconfig
	@$(COMMON_CONFIG_ENV) UK_DEFCONFIG=$(A)/configs/$@ \
		$< --defconfig=$(A)/configs/$@ $(CONFIG_CONFIG_IN)

savedefconfig: $(KCONFIG_DIR)/conf
	@$(COMMON_CONFIG_ENV) $< \
		--savedefconfig=$(if $(DEFCONFIG),$(DEFCONFIG),$(CONFIG_DIR)/defconfig) \
		$(CONFIG_CONFIG_IN)
ifeq ($(HOSTARCH),$(CONFIG_UK_ARCH))
	@# Make sure arch is stored in the file even if arch matches between host and config
	@echo "$(call ukarch_str2cfg,$(CONFIG_UK_ARCH))=y" >> \
		$(if $(DEFCONFIG),$(DEFCONFIG),$(CONFIG_DIR)/defconfig)
endif

.PHONY: defconfig savedefconfig silentoldconfig

# Regenerate $(KCONFIG_AUTOHEADER) whenever $(UK_CONFIG) changed
$(KCONFIG_AUTOHEADER): $(UK_CONFIG) $(KCONFIG_DIR)/conf
	@$(COMMON_CONFIG_ENV) $(KCONFIG_DIR)/conf --syncconfig $(CONFIG_CONFIG_IN)
endif


# Misc stuff
# ---------------------------------------------------------------------------
print-loc: images
	@$(info [LoC stats])
	@$(foreach I,$(UK_DEBUG_IMAGES) $(UK_DEBUG_IMAGES-y),\
		$(info $(shell basename $(I) .dbg) has $(call measure_loc,$(I)) lines of code))

print-vars:
	@$(foreach V, \
		$(sort $(if $(VARS),$(filter $(VARS),$(.VARIABLES)),$(.VARIABLES))), \
		$(if $(filter-out environment% default automatic,$(origin $V)), \
		$(if $(filter simple,$(flavor $V)), \
			$(info [$(origin $V)] $V := $(value $V)), \
			$(info [$(origin $V)] $V = <$(flavor $V)>))))

print-version:
	@echo $(UK_FULLVERSION)

ifeq ($(UK_HAVE_DOT_CONFIG),y)
print-libs:
	@echo 	$(foreach P,$(UK_PLATS) $(UK_PLATS-y),\
		$(if $(call qstrip,$($(call uc,$(P))_LIBS) $($(call uc,$(P))_LIBS-y)),\
		$(foreach L,$($(call uc,$(P))_LIBS) $($(call uc,$(P))_LIBS-y), \
		$(if $(call qstrip,$($(call vprefix_lib,$(L),SRCS)) $($(call vprefix_lib,$(L),SRCS-y))), \
		$(L) \
		)))) \
		$(UK_LIBS) $(UK_LIBS-y)

print-lds:
	@echo -e \
		$(foreach P,$(UK_PLATS) $(UK_PLATS-y),\
		$(if $(call qstrip,$($(call uc,$(P))_LIBS) $($(call uc,$(P))_LIBS-y)),\
		$(foreach L,$($(call uc,$(P))_LIBS) $($(call uc,$(P))_LIBS-y), \
		$(if $(call qstrip,$($(call vprefix_lib,$(L),LDS)) $($(call vprefix_lib,$(L),LDS-y))), \
		'$(L):\n   $($(call vprefix_lib,$(L),LDS)) $($(call vprefix_lib,$(L),LDS-y))\n'\
		))))\
		$(foreach L,$(UK_LIBS) $(UK_LIBS-y),\
		$(if $(call qstrip,$($(call vprefix_lib,$(L),LDS)) $($(call vprefix_lib,$(L),LDS-y))),\
		'$(L):\n   $($(call vprefix_lib,$(L),LDS)) $($(call vprefix_lib,$(L),LDS-y))\n'\
		))

print-objs:
	@echo -e \
		$(foreach P,$(UK_PLATS) $(UK_PLATS-y),\
		$(if $(call qstrip,$($(call uc,$(P))_LIBS) $($(call uc,$(P))_LIBS-y)),\
		$(foreach L,$($(call uc,$(P))_LIBS) $($(call uc,$(P))_LIBS-y), \
		$(if $(call qstrip,$($(call vprefix_lib,$(L),OBJS)) $($(call vprefix_lib,$(L),OBJS-y)) \
		       $(EACHOLIB_OBJS) $(EACHOLIB_OBJS-y)), \
		'$(L):\n   $($(call vprefix_lib,$(L),OBJS)) $($(call vprefix_lib,$(L),OBJS-y)) $(EACHOLIB_OBJS) $(EACHOLIB_OBJS-y)\n'\
		))))\
		$(foreach L,$(UK_LIBS) $(UK_LIBS-y),\
		$(if $(call qstrip,$($(call vprefix_lib,$(L),OBJS)) $($(call vprefix_lib,$(L),OBJS-y)) \
		       $(EACHOLIB_OBJS) $(EACHOLIB_OBJS-y)), \
		'$(L):\n   $($(call vprefix_lib,$(L),OBJS)) $($(call vprefix_lib,$(L),OBJS-y)) $(EACHOLIB_OBJS) $(EACHOLIB_OBJS-y)\n'\
		))

print-srcs:
	@echo -e \
		$(foreach P,$(UK_PLATS) $(UK_PLATS-y),\
		$(if $(call qstrip,$($(call uc,$(P))_LIBS) $($(call uc,$(P))_LIBS-y)),\
		$(foreach L,$($(call uc,$(P))_LIBS) $($(call uc,$(P))_LIBS-y), \
		$(if $(call qstrip,$($(call vprefix_lib,$(L),SRCS)) $($(call vprefix_lib,$(L),SRCS-y)) \
		       $(EACHOLIB_SRCS) $(EACHOLIB_SRCS-y)), \
		'$(L):\n   $($(call vprefix_lib,$(L),SRCS)) $($(call vprefix_lib,$(L),SRCS-y)) $(EACHOLIB_SRCS) $(EACHOLIB_SRCS-y)\n'\
		))))\
		$(foreach L,$(UK_LIBS) $(UK_LIBS-y),\
		$(if $(call qstrip,$($(call vprefix_lib,$(L),SRCS)) $($(call vprefix_lib,$(L),SRCS-y)) \
		       $(EACHOLIB_SRCS) $(EACHOLIB_SRCS-y)), \
		'$(L):\n   $($(call vprefix_lib,$(L),SRCS)) $($(call vprefix_lib,$(L),SRCS-y)) $(EACHOLIB_SRCS) $(EACHOLIB_SRCS-y)\n'\
		))
else
print-libs:
	$(error Do not have a configuration. Please run one of the configuration targets first)

print-objs:
	$(error Do not have a configuration. Please run one of the configuration targets first)

print-srcs:
	$(error Do not have a configuration. Please run one of the configuration targets first)
endif
else #!($(sub_make_exec),)
export sub_make_exec:=1

$(BUILD_DIR)/Makefile:
	$(call verbose_cmd,LN,$(notdir $@),$(HOSTLN) -sf $(CONFIG_UK_BASE)/Makefile $@)

$(filter-out _all $(BUILD_DIR)/Makefile sub-make distclean properclean help $(lastword $(MAKEFILE_LIST)), \
  $(MAKECMDGOALS)) all: sub-make
	@:

sub-make: $(BUILD_DIR)/Makefile
	$(Q)$(MAKE) --no-print-directory CONFIG_UK_BASE=$(CONFIG_UK_BASE) -C $(BUILD_DIR) -f $(BUILD_DIR)/Makefile $(MAKECMDGOALS)

endif

help:
	@echo 'Cleaning:'
	@echo '  clean-[LIBNAME]        - delete all files created by build for a single library'
	@echo '                           (e.g., clean-libfdt)'
	@echo '  clean-libs             - delete all files created by build for all libraries'
	@echo '                           but keep final images and fetched files'
	@echo '  clean                  - delete all files created by build for all libraries'
	@echo '                           including final images, but keep fetched files'
	@echo '  properclean            - delete build directory'
	@echo '  distclean              - delete build directory and configurations (including .config)'
	@echo ''
	@echo 'Building:'
	@echo '* all                    - build everything (default target)'
	@echo '  images                 - build kernel images for selected platforms'
	@echo '  libs                   - build libraries and objects'
	@echo '  [LIBNAME]              - build a single library'
	@echo '  objs                   - build objects only'
	@echo '  preprocess             - run preprocessing steps'
	@echo '  prepare                - run preparation steps'
	@echo '  fetch                  - fetch, extract, and patch remote code'
	@echo ''
	@echo 'Configuration:'
	@echo '* menuconfig             - interactive curses-based configurator'
	@echo '                           (default target when no config exists)'
	@echo '  kmenuconfig            - interactive python based configurator'
	@echo '  guiconfig              - interactive python based configurator'
	@echo '  nconfig                - interactive ncurses-based configurator'
	@echo '  xconfig                - interactive Qt-based configurator'
	@echo '  gconfig                - interactive GTK-based configurator'
	@echo '  oldconfig              - resolve any unresolved symbols in .config'
ifeq ($(HOSTOSENV),Linux)
	@echo '  syncconfig             - Same as oldconfig, but quietly, additionally update deps'
	@echo '  scriptsyncconfig       - Same as oldconfig, but quietly, additionally update deps'
	@echo '  olddefconfig           - Same as silentoldconfig but sets new symbols to their default value'
	@echo '  randconfig             - New config with random answer to all options'
endif
	@echo '  defconfig              - New config with default answer to all options'
	@echo '                             UK_DEFCONFIG, if set, is used as input'
	@echo '  savedefconfig          - Save current config to UK_DEFCONFIG (minimal config)'
	@echo '  allyesconfig           - New config where all options are accepted with yes'
	@echo '  allnoconfig            - New config where all options are answered with no'
	@echo ''
	@echo 'Command-line variables:'
	@echo '  V=0|1|2                - 0 => quiet build (default), 1 => verbose build,'
	@echo '                           2 => like 1 and warn about undefined build variables'
	@echo '  C=[PATH]               - path to .config configuration file'
	@echo '  O=[PATH]               - path to build output (will be created if it does not exist)'
	@echo '  A=[PATH]               - path to Unikraft application'
	@echo '  N=[NAME]               - use NAME as image name instead the one found in the configuration'
	@echo '                           (note: the name in the configuration file is not overwritten)'
	@echo '  L=[PATH]:[PATH]:..     - colon-separated list of paths to external libraries'
	@echo '  P=[PATH]:[PATH]:..     - colon-separated list of paths to external platforms'
	@echo ''
	@echo 'Environment variables:'
	@echo '  UK_ASFLAGS             - explicit Unikraft-specific additions to the assembler flags (the ASFLAGS variable is ignored)'
	@echo '  UK_CFLAGS              - explicit Unikraft-specific additions to the C compiler flags (the CFLAGS variable is ignored)'
	@echo '  UK_CXXFLAGS            - explicit Unikraft-specific additions to the C++ compiler flags (the CXXFLAGS variable is ignored)'
	@echo '  UK_GOCFLAGS            - explicit Unikraft-specific additions to the GO compiler flags (the GOCFLAGS variable is ignored)'
	@echo '  UK_LDFLAGS             - explicit Unikraft-specific additions to the linker flags (the LDFLAGS variable is ignored)'
	@echo '  UK_LDEPS               - explicit, space-seperated link-time file dependencies (changes to these files will trigger relinking on subsequent builds)'
	@echo ''
	@echo 'Miscellaneous:'
	@echo '  print-version          - print Unikraft version'
	@echo '  print-libs             - print library names enabled for build'
	@echo '  print-lds              - print linker script enabled for the build'
	@echo '  print-objs             - print object file names enabled for build'
	@echo '  print-srcs             - print source file names enabled for build'
	@echo '  print-vars             - prints all the variables currently defined in Makefile'
	@echo '  print-loc              - print Lines-of-Code statistics for built unikernel image(s)'
	@echo ''

endif #umask

# This is used to detect if a makefile macro expansion is immediate or deferred.
# As the last statement in the main makefile, this is read in the end of the
# immediate expansion phase. Therefore, UK_DEFERRED_EXPANSION is expanded to a
# empty string in immediate expansion and to 'y' in a deferred expansion.
UK_DEFERRED_EXPANSION := y
