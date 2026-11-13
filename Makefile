# SPDX-License-Identifier: GPL-2.0
# Top-level build driver.
#
#   make            build module + CLI
#   make module     kernel side only
#   make user       CLI only
#   make clean
#
# KDIR may be overridden:
#   make KDIR=/path/to/kernel/headers

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

.PHONY: all module user clean check

all: module user

module:
	@test -d $(KDIR) || { \
		echo "error: kernel headers not found at $(KDIR)"; \
		echo "       install them, or pass KDIR=/path/to/headers"; \
		exit 1; }
	$(MAKE) -C $(KDIR) M=$(PWD)/kernel modules

user:
	$(MAKE) -C user

check:
	@echo "verify ABI version consistency:"
	@grep -n 'NDUMP_ABI_VERSION' kernel/ndump_uapi.h

clean:
	$(MAKE) -C $(KDIR) M=$(PWD)/kernel clean 2>/dev/null || true
	$(MAKE) -C user clean
