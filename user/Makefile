# SPDX-License-Identifier: GPL-2.0
# Freestanding user-space CLI: no libc, no dynamic loader, no PLT/GOT.
#
#   make            -> ndump (static ELF)
#
# The result links with ld directly, not gcc's driver, so no crt0 is pulled
# in. entry is _start, provided by user/main.c.

CC      := gcc
LD      := ld
KINC    := /usr/include

CFLAGS  := -std=gnu11 -O2 -g \
           -ffreestanding -fno-stack-protector -fno-pic \
           -fno-asynchronous-unwind-tables -fno-unwind-tables \
           -fno-ident -nostdlib -static \
           -mno-red-zone -mcmodel=small \
           -Wall -Wextra -Werror \
           -I$(KINC) -I../kernel

LDFLAGS := -static -nostdlib -z noexecstack --build-id=none \
           -e _start

OBJS := main.o ndump_dev.o ndump_elf.o ndump_scan.o

.PHONY: all clean

all: ndump

ndump: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c ndump.h ndump_syscall.h ../kernel/ndump_uapi.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) ndump
