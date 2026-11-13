# nimbledump — design

## 1. Threat model / why this shape

Reading another process's memory in Linux has three conventional doors, and all
three are *auditable*:

1. `ptrace(PTRACE_ATTACH)` + `/proc/pid/mem` — target becomes `PT_PTRACED`,
   `ptrace_attach()` runs an LSM hook (`security_ptrace_access_check`), and the
   target can detect the tracer via `PR_SET_DUMPABLE`, `TracerPid`, or signal
   behaviour.
2. `process_vm_readv` — funnels through `mm_access()` in
   `kernel/fork.c`, same `PTRACE_MODE_ATTACH_REALCREDS` gate.
3. `/dev/mem`, `/dev/kmem` — `CONFIG_STRICT_DEVMEM` restricts to IO regions;
   `CONFIG_DEVKMEM` is gone from modern kernels.

`nimbledump` takes a fourth door: it lives in the kernel, takes
`mmget_not_zero(mm)`, walks the target's page tables itself, and maps the
physical frame behind each present PTE. No task state changes, no LSM hook
fires, no syscall is issued *against the target*.

This is the same access path used by `LiME`, `AVML` (via `/dev/crash`), and
every live-forensics acquisition tool. It is also exactly what an anti-cheat
or EDR driver does internally to inspect memory — which is why user-space
protections are irrelevant to it, not why they are "bypassed".

## 2. Component map

```
+--------------------------------------------------------------+
| user/ndump  (Rust, no_std, x86_64-unknown-none)               |
|   main.rs       CLI parse, orchestration, output              |
|   dev.rs        open /dev/ndump, ioctl, framed read()         |
|   elf.rs        PT_LOAD + PT_NOTE core writer                 |
|   scan.rs       pattern/heap scan over the produced buffer    |
|   sys.rs        freestanding syscall layer (r10/r8/r9 asm)    |
|   abi.rs        repr(C) mirror of ndump_uapi.h + ioctl nums   |
+----------------------------|---------------------------------+
                             | misc device, ioctl + read
+----------------------------v---------------------------------+
| kernel/nimbledump.ko                                         |
|                                                              |
|  ndump_main.c   misc device, fops, ioctl dispatch, lifecycle  |
|  ndump_task.c   pid -> task_struct (init_task list walk)      |
|  ndump_vma.c    mm pin + VMA_ITERATOR reconstruction          |
|  ndump_pgt.c    pgd->p4d->pud->pmd->pte walk, THP handling    |
|  ndump_read.c   pte -> page -> kmap_local_page -> copy        |
|  ndump_ring.c   kfifo page ring, producer/consumer split      |
|                                                              |
|  ndump_uapi.h   shared ABI (kernel <-> user)                  |
|  ndump_internal.h  private module state                       |
+--------------------------------------------------------------+
```

## 3. PID -> task_struct without exported symbols

Two implementations, selected at build time:

**A. Documented path.** `find_get_task_by_vpid()` — exported, PID-namespace
correct, returns a refcounted task.

**B. Raw list walk.** Used when the target may be in a different PID namespace
or when the build must not link against task-lookup symbols:

```
for (task = &init_task; ; task = list_entry(task->tasks.next, ...)) {
        if (task->pid == target_pid) break;
        if (task->tasks.next == &init_task->tasks) return NULL;
}
```

Requires `rcu_read_lock()` around the walk (task list is RCU-protected).
`init_task` itself is exported, so this compiles without `kallsyms_lookup_name`
or any symbol-resolution trickery. Offsets of `tasks` and `pid` in
`task_struct` come from the compiler via `<linux/sched.h>` — no hardcoded
`offsetof` constants, which is why this survives kernel version changes.

Note on PIDs: `task->pid` is namespace-local. For cross-namespace targeting the
walk must compare `task_pid_nr_ns(task, target_ns)` — this is called out in the
header contract.

## 4. mm pinning and VMA reconstruction

```
task = ndump_find_task(pid);
mm   = get_task_mm(task);          /* pins mm, takes mm_users */
if (!mm) -> kernel thread, abort
mmap_read_lock(mm);                /* VMA list is stable under read lock */
VMA_ITERATOR(vmi, mm, 0);
for_each_vma(vmi, vma) { ... }
mmap_read_unlock(mm);
```

`for_each_vma` skips holes automatically. Per VMA we record `vm_start`,
`vm_end`, `vm_flags`. We skip `VM_IO | VM_PFNMAP | VM_DONTDUMP` for the data
copy (there is no struct page to map), but still report them in `--vma-list`.

The read lock is held only for VMA enumeration. The page walk happens per-VMA
with a short re-acquire, so a long dump does not stall the target's page faults
for its whole duration.

## 5. Page-table walk

```
pgd_t *pgd = mm->pgd + pgd_index(addr);
if (pgd_none(*pgd) || pgd_bad(*pgd))  -> hole
p4d = p4d_offset(pgd, addr);          /* folded away on 4-level */
pud = pud_offset(p4d, addr);
if (pud_none(*pud))                   -> hole
if (pud_leaf(*pud))                   -> 1G huge page (pud_page)
pmd = pmd_offset(pud, addr);
if (pmd_none(*pmd))                   -> hole
if (pmd_leaf(*pmd) || pmd_trans_huge(*pmd)) -> 2M THP (pmd_page)
pte  = pte_offset_map_lock(mm, pmd, addr, &ptl);
pte_t pteval = *pte;
present = pte_present(pteval);
special = pte_special(pteval);
page = pte_page(pteval);
pte_unmap_unlock(pte, ptl);
```

Key details that make this correct across configs:

* Use `p4d_offset` even if `CONFIG_PGTABLE_LEVELS == 4` — it folds to a no-op
  and the code stays 5-level clean. Never index `pgd` manually.
* `pgd_bad`/`pud_bad` checks, not just `*none`, or a corrupted table crashes
  the host.
* THP: `pmd_trans_huge` requires `pmd_lock` for the read on some paths;
  `pmd_leaf` is the config-safe predicate in 6.x.
* `pte_special` -> skip (no struct page, e.g. PFNMAP/device).
* Tail pages of a THP: only the head carries `PageCompound`; when copying an
  interior address use `page + pte_index(addr) - thp_nr_pages` carefully, or
  copy the whole PMD at once and slice. The latter is what `ndump_pgt.c` does.

### Reading the frame

```c
void *kaddr = kmap_local_page(page);
memcpy(buf, kaddr + offset_in_page(addr), n);
kunmap_local(kaddr);
```

`kmap_local_page` is the correct API for arbitrary (possibly highmem, on 32-bit
targets) pages and does not disable preemption beyond the current local scope.
For pages backed by slab or vmalloc, `copy_from_kernel_nofault()` is the
fallback when a `struct page` cannot be resolved.

## 6. Ring buffer and user-space drain

`ndump_ring.c` is a `kfifo` of fixed 4 KiB slots. The walker is the producer;
`read()` on the misc device is the consumer. Framing:

```
struct ndump_rec { __u64 vaddr; __u32 len; __u32 flags; };  /* then len bytes */
NDUMP_F_DATA  real bytes follow
NDUMP_F_ZERO  hole; user space emits a zero-filled region
NDUMP_F_SKIP  unmappable; metadata only
```

The producer runs from a kernel work item, not from the `read()` path, so the
consumer never blocks the page walk and vice versa. Backpressure: when the
fifo fills, the walker sleeps on the fifo wait queue. This bounds kernel memory
to the slot count regardless of target size (a 40 GB process does not need 40
GB of kernel RAM).

`read()` returns only complete records; a partial record is re-queued by the
consumer side, never split mid-frame.

## 7. ioctl ABI

Defined once in `kernel/ndump_uapi.h`, included by both sides. See that file
for the struct layout. Sequence:

```
open("/dev/ndump")
NDUMP_IOC_ATTACH   { pid }              -> cookie, validates task+mm
NDUMP_IOC_VMA_LIST                      -> iterator of ndump_vma_info
NDUMP_IOC_ADD_REGION { start, end }     -> filter (repeatable)
NDUMP_IOC_START                         -> spawns producer work
loop: read()                            -> ndump_rec stream
NDUMP_IOC_STAT                          -> pages_ok / skipped / faults
NDUMP_IOC_STOP / DETACH
```

All addresses are `__u64` and validated against `TASK_SIZE_MAX` before use.
The module keeps one target per file descriptor (not per module), so multiple
concurrent dumps are possible and cleanup is tied to `release()`.

## 8. User-space: direct syscalls, no libc (Rust)

The CLI targets `x86_64-unknown-none`: `#![no_std]`, no allocator, no loader —
the target triple has no std/libc by construction, so the glibc-free property
is enforced by the toolchain rather than by build flags. All syscalls go
through inline `syscall` instructions (`core::arch::asm!`) rather than any
wrapping layer:

```rust
pub unsafe fn raw(n: u64, a: u64, b: u64, c: u64,
                  d: u64, e: u64, f: u64) -> i64 {
    let ret: i64;
    core::arch::asm!(
        "syscall",
        inlateout("rax") n as i64 => ret,
        in("rdi") a, in("rsi") b, in("rdx") c,
        in("r10") d, in("r8") e, in("r9") f,
        lateout("rcx") _, lateout("r11") _,
        options(nostack)
    );
    ret
}
```

The register choices are mandatory: 4th/5th/6th args go in `r10`/`r8`/`r9`
per the x86_64 SysV *kernel* convention (not the userspace one), and `rcx`/
`r11` are clobbered by `syscall` itself (return RIP and RFLAGS) — omitting
either silently corrupts the caller.

Consequences worth stating plainly: an LD_PRELOAD shim cannot intercept these
calls, and the binary has no dynamic symbol table for a user-space hook to
patch. That is a *dependency* property (no libc, no loader), useful for running
from a minimal initramfs — it is not a detection-evasion feature.

`SYS_ioctl`, `SYS_read`, `SYS_openat`, `SYS_mmap`, `SYS_munmap`, `SYS_write`,
`SYS_exit_group` are the only numbers needed. The `abi.rs` ioctl numbers are
computed with `const fn` from struct sizes (mirroring `_IOWR`/`_IOR`/`_IOW`),
and `NdumpRec` carries a compile-time `assert!(size_of::<NdumpRec>() == 16)`
so an ABI drift in either direction fails the build instead of desyncing the
record parser.

## 9. ELF core output

`elf.rs` emits a standard `ET_CORE`:

* `PT_NOTE` with `NT_PRSTATUS` (pid, signal 0, no registers) and `NT_PRPSINFO`
  (comm, args from the VMA that spans the stack), so GDB can `attach` the file
  and walk the mappings.
* one `PT_LOAD` per dumped VMA: `p_vaddr = vm_start`, `p_filesz = 0`,
  `p_memsz = vm_end - vm_start`, `p_offset` into the payload appended after the
  header. GDB treats `filesz == 0` as "not present in file", so the layout must
  instead use `p_filesz = dumped_len` with the data written in place. Sparse
  holes are written as real zeros for GDB compatibility, or kept sparse in
  `--raw` mode.

## 10. Failure modes to handle (the parts that actually bite)

* **Target exits mid-dump.** `mm` is refcounted via `get_task_mm()`, so the
  `mm_struct` and page tables survive, but page contents may be freed. Detect
  via `mmget_not_zero()` recheck and mark the run `NDUMP_STAT_PARTIAL`.
* **`vmalloc` / stack guard guard pages.** `pte_none` -> emit `NDUMP_F_ZERO`,
  never dereference.
* **Huge zero page.** `pte_devmap` and the zero page have no useful contents;
  check `is_zero_pfn(pte_pfn(pteval))` and emit zeroes without mapping.
* **PID reuse.** Validate by capturing `task->start_time` (or `mm` pointer)
  at ATTACH and rechecking on every region; mismatch aborts.
* **Lock ordering.** Never take `mmap_read_lock` while holding the fifo lock.
  Producer order is: read lock -> walk -> unlock -> enqueue.
* **CONFIG_PREEMPT_RT.** `kmap_local_page` is sleeping-safe there; the walker
  must not be in a `spin_lock` region when copying. The design uses a work
  item, not an atomic context, specifically for this.

## 11. Build matrix

| Config | Concern |
|---|---|
| `CONFIG_PGTABLE_LEVELS=5` | `p4d_offset` must not be folded manually |
| `CONFIG_TRANSPARENT_HUGEPAGE` | `pmd_leaf` path |
| `CONFIG_HUGETLBFS` | `pud_leaf` 1G path |
| `CONFIG_HIGHMEM` (32-bit host) | `kmap_local_page` required |
| `CONFIG_PREEMPT_RT` | work item, not softirq/timer |
| `CONFIG_KASAN` | `copy_from_kernel_nofault` for untrusted frames |

## 12. Validation plan

Not against `/proc/pid/mem` inside the tool — that is a hard constraint. Instead
`tools/validate.sh` compares output against a reference produced by independent
means (`gcore`, or a self-test target that `mmap`s known patterns and writes its
own map to a file). Diff must be byte-exact for `MAP_ANONYMOUS` regions.
