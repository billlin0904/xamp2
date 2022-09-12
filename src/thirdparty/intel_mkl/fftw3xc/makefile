#===============================================================================
# Copyright 2006-2019 Intel Corporation.
#
# This software and the related documents are Intel copyrighted  materials,  and
# your use of  them is  governed by the  express license  under which  they were
# provided to you (License).  Unless the License provides otherwise, you may not
# use, modify, copy, publish, distribute,  disclose or transmit this software or
# the related documents without Intel's prior written permission.
#
# This software and the related documents  are provided as  is,  with no express
# or implied  warranties,  other  than those  that are  expressly stated  in the
# License.
#===============================================================================

## Content:
##      Build standalone library of FFTW3 C wrappers to Intel(R) MKL.
##*****************************************************************************

## Usage example:
##
## make libintel64 compiler=gnu
##      Build the library for Intel(R) 64 based applications
##      using GNU C compiler, for use with GNU C compiler.

help:
	@echo "Usage: make libia32|libintel64 [option...]"
	@echo
	@echo "Options:"
	@echo "  compiler=gnu|pgi|intel"
	@echo "        Build the library using GNU gcc, PGI pgcc, or"
	@echo "        Intel(R) C compiler icc."
	@echo "        Default value: intel"
	@echo
	@echo "Additional macros:"
	@echo "  MKLROOT=<path>"
	@echo "        Path to Intel(R) MKL root directory with header files and libraries."
	@echo "        Default value: ../.."
	@echo
	@echo "  INSTALL_DIR=<path>"
	@echo "        Install the library to the specified location."
	@echo "        Default value:  . (that is, the current directory)"
	@echo
	@echo "  INSTALL_LIBNAME=<name>"
	@echo "        Specify the name of the library."
	@echo "        Default value depends on PRECISION and compiler used:"
	@echo "          libfftw3xc_$(compiler).a"
	@echo

##-----------------------------------------------------------------------------
## Default values

MY_MAKEFILE := $(MAKEFILE_LIST)

MKLROOT ?= ../..
compiler = intel

include $(MKLROOT)/interfaces/fftw3xc/fftw3xc.lst

install_to ?= .
INSTALL_DIR ?= $(install_to)
obj_path = $(INSTALL_DIR)/obj_$(compiler)
install_as ?= libfftw3xc_$(compiler).a
INSTALL_LIBNAME ?= $(install_as)

##-----------------------------------------------------------------------------
## Main targets

.PHONY: libia32 lib32 libintel64 libem64t

libia32 lib32:
	$(MAKE) -f $(MY_MAKEFILE) lib _IA=ia32

libintel64 libem64t:
	$(MAKE) -f $(MY_MAKEFILE) lib _IA=intel64

ifdef _IA
##-----------------------------------------------------------------------------
## Supporting _macros

_CC_intel = icc
_CC_gnu = gcc
_CC_pgi = pgcc
CC = $(firstword $(_CC_$(compiler)) icc)

# Define _cflags
_cflags = $(_cflags_$(compiler)_$(_IA))
_cflags += $(_cflags_$(compiler)_diag)
_cflags += $(CFLAGS)

## This option is required by Intel(R) compiler to produce
## Pentium(R) III compatible executables.
_cflags_intel_ia32 = -mia32
_cflags_intel_diag = -Wall -Werror

_cflags_gnu_ia32 = -m32
_cflags_gnu_intel64 = -m64
_cflags_gnu_diag = -Wall -Werror

_cflags_pgi_ia32 = -tp px
_cflags_pgi_intel64 = -tp x64
_cflags_pgi_diag = 

# Define _cppflags
_cppflags = -I$(MKLROOT)/include -I$(MKLROOT)/include/fftw
_cppflags += $(CPPFLAGS) $(TARGET_ARCH)

_srcdir = $(MKLROOT)/interfaces/fftw3xc/wrappers

vpath %.c $(_srcdir)

##-----------------------------------------------------------------------------
## Rules

.PHONY: lib mkobjdir clean

.SUFFIXES:
.SUFFIXES: .c .o

lib: mkobjdir $(INSTALL_DIR)/$(INSTALL_LIBNAME)

$(INSTALL_DIR)/$(INSTALL_LIBNAME): $(WRAP:%=$(obj_path)/%.o)
	ar rs $@ $^
	rm -rf $(obj_path)

$(obj_path)/%.o: %.c
	$(CC) $(_cflags) $(_cppflags) -c $< -o $@

mkobjdir:
	mkdir -p $(obj_path)

clean:
	rm -rf $(obj_path)

##-----------------------------------------------------------------------------
endif
