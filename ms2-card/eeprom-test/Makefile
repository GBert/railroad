#
# Copyright (C) 2005-2017 Darron Broad
# All rights reserved.
# 
# This file is part of Pickle Microchip PIC ICSP.
#
# Pickle Microchip PIC ICSP is free software: you can redistribute it and/or
# modify it under the terms of the GNU General Public License as published
# by the Free Software Foundation.
#
# Pickle Microchip PIC ICSP is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
# Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with Pickle Microchip PIC ICSP. If not, see http://www.gnu.org/licenses/
#
#

#FreeBSD IBMPC FreeBSD 9.0-RELEASE   amd64
#Debian  IBMPC Linux   3.2.0-4-amd64 x86_64
#Debian  RPi   Linux   3.2.27+       armv6l
SYSTEM:=$(shell uname -s)
RELEASE:=$(shell uname -r)
PLATFORM:=$(shell uname -m)

CC=gcc
CFLAGS=-Os -std=gnu99 -pedantic-errors -I. -Wall
ifeq "$(GDB)" "1"
CFLAGS+=-ggdb
endif
ifneq (,$(findstring CYGWIN,$(SYSTEM)))
CFLAGS+=-D__USE_LINUX_IOCTL_DEFS
endif

CTARGET=mcp24
CSOURCE=mcp24.c
COBJECT=$(CSOURCE:.c=.o)
CHEADER=$(CSOURCE:.c=.h)
CLIB=-lm

build:$(COBJECT) $(CTARGET)

$(AOBJECT):$(AHEADER) Makefile

$(CTARGET):$(COBJECT)
	@echo -n "[CC] "
	$(CC) $(CFLAGS) $(COBJECT) -o $(CTARGET) $(CLIB)

$(COBJECT):$(CHEADER) Makefile

%.o:%.c
	@echo -n "[CC] "
	$(CC) $(CFLAGS) -c $< -o $@

install:build

uninstall:clean

clean:
	rm -f *.lst *.hex *.cod *.log *.o *~ $(CTARGET)
