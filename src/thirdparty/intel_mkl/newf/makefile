#===============================================================================
# Copyright 2006-2022 Intel Corporation.
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
##      Build standalone library of FFTW3 Fortran wrappers to Intel(R) oneMKL.
##*****************************************************************************

## Usage example:
##
## make libintel64 compiler=gnu
##      Build the library for Intel(R) 64 based applications
##      using GNU C compiler, for use with GNU Fortran compiler.

help:
	@echo "Usage: make libintel64 [option...]"
	@echo
	@echo "Options:"
	@echo "  compiler=gnu|pgi|intel"
	@echo "        Build the library using GNU gcc, PGI pgcc, or"
	@echo "        Intel(R) C compiler icc."
	@echo "        Default value: intel"
	@echo
	@echo "  INTERFACE=lp64|ilp64 selects kind of MKL_INT type for %intel64 targets:"
	@echo "        INTERFACE=lp64  - 4-byte integers (DEFAULT)"
	@echo "        INTERFACE=ilp64 - 8-byte integers"
	@echo
	@echo "  fname=a_name|a_name_|a_name__|A_NAME"
	@echo "        Select the pattern to decorate wrapper names for"
	@echo "        Fortran. For example, with no special options"
	@echo "            g77 uses pattern a_name__"
	@echo "            ifort and gfortran use pattern a_name_"
	@echo "            ifort on Windows uses pattern A_NAME"
	@echo "        Default value: a_name_"
	@echo
	@echo "Additional macros:"
	@echo "  MKLROOT=<path>"
	@echo "        Path to Intel(R) oneMKL root directory with header files and libraries."
	@echo "        Default value: ../.."
	@echo
	@echo "  INSTALL_DIR=<path>"
	@echo "        Install the library to the specified location."
	@echo "        Default value: . (that is, the current directory)"
	@echo
	@echo "  INSTALL_LIBNAME=<name>"
	@echo "        Specify the name of the library."
	@echo "        Default value depends on PRECISION and compiler used:"
	@echo "          libfftw3xf_$(compiler).a"
	@echo

##-----------------------------------------------------------------------------
## Default values

MY_MAKEFILE := $(MAKEFILE_LIST)

MKLROOT ?= ../..

include $(MKLROOT)/interfaces/fftw3xf/fftw3xf.lst

compiler = intel
INTERFACE = lp64
fname = a_name_

install_to ?= .
INSTALL_DIR ?= $(install_to)
obj_path = $(INSTALL_DIR)/obj_$(compiler)
install_as ?= libfftw3xf_$(compiler).a
INSTALL_LIBNAME ?= $(install_as)

##-----------------------------------------------------------------------------
## Main targets

.PHONY: libintel64 libem64t

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

_cflags_intel_diag = -Wall -Werror

_cflags_gnu_intel64 = -m64
_cflags_gnu_diag = -Wall -Werror

# Define _cppflags
_cppflags = -I$(MKLROOT)/include -I$(MKLROOT)/include/fftw
_cppflags += $(_cppflags_interface_$(INTERFACE))
_cppflags += $(_cppflags_$(fname))
_cppflags += $(CPPFLAGS) $(TARGET_ARCH)

_cppflags_a_name   = -D_FNAME_NOUNDERSCORE
_cppflags_a_name_  =
_cppflags_a_name__ = -D_FNAME_SECOND_UNDERSCORE
_cppflags_A_NAME   = -D_FNAME_UPPERCASE

_cppflags_interface_ilp64   = -DMKL_ILP64

_srcdir = $(MKLROOT)/interfaces/fftw3xf/wrappers

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
