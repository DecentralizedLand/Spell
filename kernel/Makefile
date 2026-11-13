# SPDX-License-Identifier: GPL-2.0
# External (out-of-tree) kernel module build.
#
#   make -C /lib/modules/$(uname -r)/build M=$(PWD) modules
#
# Linux 5.8+ defines a few macros that modules must not rely on across
# versions; keep the flag list minimal on purpose so this builds on 5.x, 6.x
# and mainline without per-version #ifdef forests.

obj-m += nimbledump.o

nimbledump-y := \
	ndump_main.o \
	ndump_task.o \
	ndump_vma.o \
	ndump_pgt.o \
	ndump_read.o \
	ndump_ring.o

# 1 = manual init_task list walk, 0 = find_get_task_by_vpid (default)
ccflags-y += -DNDUMP_TASK_WALK_RAW=1
