// SPDX-License-Identifier: GPL-2.0
//
// src/elf.rs — minimal ET_CORE writer, no libc.
//
// Layout emitted:
//
//   [ Elf64_Ehdr ]
//   [ Elf64_Phdr * n ]        PT_NOTE + one PT_LOAD per dumped VMA
//   [ note payload ]          NT_PRSTATUS, NT_PRPSINFO
//   [ segment payloads ]      raw bytes, page-aligned per PT_LOAD
//
// RULES THAT MATTER
//   - e_ident: ELFCLASS64, ELFDATA2LSB, EV_CURRENT, ELFOSABI_NONE.
//   - e_type = ET_CORE, e_machine = EM_X86_64, e_phoff = sizeof(Ehdr),
//     e_phentsize = sizeof(Elf64_Phdr), e_entry = 0.
//   - PT_LOAD must carry p_filesz == dumped length and p_memsz == vma
//     length. p_filesz = 0 makes GDB treat the segment as absent, which
//     silently produces a core with no memory in it.
//   - p_align = PAGE_SIZE; the loader maps with mmap, misalignment is fatal.
//   - Holes inside a VMA are written as real zeros unless --raw sparse is
//     requested; GDB has no per-segment sparse representation.
//   - NT_PRPSINFO needs a plausible pr_fname/pr_psargs or `info proc` looks
//     broken even though the memory is intact.
//
// The writer is streaming: it seeks back to patch e_phnum at the end, so the
// segment count need not be known up front. The output fd must be seekable
// for that; a pipe is rejected with Err(-ND_ERR_IO).
#![allow(dead_code)]

use crate::{NdResult, Session};

pub const ND_ERR_IO: i32 = -1;

// ELF constants we emit — spelled out, no goblin/object crate (no_std).
const ET_CORE: u16 = 4;
const EM_X86_64: u16 = 62;
const EV_CURRENT: u8 = 1;
const ELFCLASS64: u8 = 2;
const ELFDATA2LSB: u8 = 1;
const PT_LOAD: u32 = 1;
const PT_NOTE: u32 = 4;
const NT_PRSTATUS: u32 = 1;
const NT_PRPSINFO: u32 = 3;
const PAGE_SIZE: u64 = 4096;
const EHDR_SIZE: usize = 64;
const PHDR_SIZE: usize = 56;

pub struct ElfWriter {
    // TODO: out fd, current file offset, PT_LOAD count, phdr table offset
    _priv: (),
}

pub fn begin(s: &mut Session) -> NdResult<ElfWriter> {
    // TODO: implement —
    //   write placeholder Elf64_Ehdr;
    //   probe seekability (lseek(0)); if it fails -> Err(ND_ERR_IO);
    //   reserve phdr slots: 1 PT_NOTE + up to s.n_regions PT_LOADs
    let _ = s;
    unimplemented!()
}

pub fn add_segment(
    w: &mut ElfWriter,
    vaddr: u64,
    len: u64,
    file_off: u64,
) -> NdResult<()> {
    // TODO: implement — append Elf64_Phdr:
    //   p_type = PT_LOAD, p_offset = file_off, p_vaddr = vaddr,
    //   p_paddr = 0, p_filesz = len, p_memsz = len, p_flags = PF_R|PF_W|PF_X,
    //   p_align = PAGE_SIZE
    let _ = (w, vaddr, len, file_off);
    unimplemented!()
}

pub fn finish(mut w: ElfWriter) -> NdResult<()> {
    // TODO: implement —
    //   write NT_PRSTATUS + NT_PRPSINFO note payload;
    //   seek back to e_phnum, patch the real segment count;
    //   final lseek to EOF (a subsequent write would desync the patch)
    let _ = &mut w;
    unimplemented!()
}

// Suppress "never used" for the constants until the writer lands —
// they document the exact on-disk contract the implementation must hit.
const _: () = {
    assert!(EHDR_SIZE == 64);
    assert!(PHDR_SIZE == 56);
    assert!(ET_CORE == 4 && EM_X86_64 == 62);
    assert!(ELFCLASS64 == 2 && ELFDATA2LSB == 1 && EV_CURRENT == 1);
    assert!(PT_LOAD == 1 && PT_NOTE == 4);
    assert!(NT_PRSTATUS == 1 && NT_PRPSINFO == 3);
    assert!(PAGE_SIZE == 4096);
};
