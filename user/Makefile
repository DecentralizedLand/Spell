# SPDX-License-Identifier: GPL-2.0
# Freestanding user-space CLI (Rust): no libc, no dynamic loader, no PLT/GOT.
#
#   make            -> ndump (static ELF, x86_64-unknown-none)
#
# Requires: rustup target add x86_64-unknown-none
# The target has no std/libc by construction, so the glibc-free property
# is enforced by the target triple, not by build flags.

TARGET  := x86_64-unknown-none
OUT     := target/$(TARGET)/release/ndump

.PHONY: all clean

all: ndump

ndump:
	cargo build --release --target $(TARGET)
	cp $(OUT) ndump

clean:
	cargo clean
	rm -f ndump
