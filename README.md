# SPELL

**S**pooky **P**rocess **E**xtraction, **L**ayer by **L**ayer

Out-of-band process memory extractor for Linux x86_64.

Reads a target process's address space from a kernel module by walking the
target's own page tables (`mm_struct -> pgd -> p4d -> pud -> pmd -> pte`) and
copying the backing pages into a kernel ring buffer that user space drains over
a misc device. The target process is never stopped, never ptraced, and no
monitored user-space interface is involved.

## What it actually avoids (and why that is a design property, not stealth)

| Interface | Used? | Why |
|---|---|---|
| `/proc/<pid>/mem` | no | permission-gated, `PTRACE_MODE_ATTACH` check, LSM-visible |
| `ptrace(2)` | no | stops target, `PT_PTRACED`, trivially detected |
| `process_vm_readv` | no | same `mm_access()` gate as ptrace, auditable |
| `/dev/mem`, `/dev/kmem` | no | `CONFIG_STRICT_DEVMEM`, usually absent |

The target's memory is reached through the kernel's own page-table structures,
which requires neither a syscall surface nor a stop-the-world event. A
user-space anti-debug check cannot observe this because there is nothing in the
target's own address space or task state to observe.

## Layout

```
kernel/   Linux kernel module (external build, out-of-tree)
user/     Rust CLI (no_std), direct syscalls, ELF core writer
tools/    test target + validation helpers
docs/     design notes
```

## Build

Kernel side needs headers for the *running* kernel:

```
make -C /lib/modules/$(uname -r)/build M=$PWD/nimbledump/kernel modules
```

User side is freestanding Rust (`x86_64-unknown-none`, no std/libc):

```
rustup target add x86_64-unknown-none
make -C nimbledump/user
```

## Usage

```
ndump -p <pid> -o target.core            # full ELF core dump
ndump -p <pid> -r 0x400000-0x401000 -o region.bin
ndump -p <pid> --vma-list                # address space map, no data
```

## Status

Skeleton only. See `docs/DESIGN.md` for the full architecture and the
per-file contract.
