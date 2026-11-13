/* SPDX-License-Identifier: GPL-2.0 */
/*
 * user/ndump_elf.c — minimal ET_CORE writer, no libc.
 *
 * Layout emitted:
 *
 *   [ Elf64_Ehdr ]
 *   [ Elf64_Phdr * n ]        PT_NOTE + one PT_LOAD per dumped VMA
 *   [ note payload ]          NT_PRSTATUS, NT_PRPSINFO
 *   [ segment payloads ]      raw bytes, page-aligned per PT_LOAD
 *
 * RULES THAT MATTER
 *   - e_ident: ELFCLASS64, ELFDATA2LSB, EV_CURRENT, ELFOSABI_NONE.
 *   - e_type = ET_CORE, e_machine = EM_X86_64, e_phoff = sizeof(Ehdr),
 *     e_phentsize = sizeof(Elf64_Phdr), e_entry = 0.
 *   - PT_LOAD must carry p_filesz == dumped length and p_memsz == vma
 *     length. p_filesz = 0 makes GDB treat the segment as absent, which
 *     silently produces a core with no memory in it.
 *   - p_align = PAGE_SIZE; the loader maps with mmap, misalignment is fatal.
 *   - Holes inside a VMA are written as real zeros unless --raw sparse is
 *     requested; GDB has no per-segment sparse representation.
 *   - NT_PRPSINFO needs a plausible pr_fname/pr_psargs or `info proc` looks
 *     broken even though the memory is intact.
 *
 * The writer is streaming: it seeks back to patch e_phnum at the end, so the
 * segment count need not be known up front. The output fd must be seekable
 * for that; a pipe is rejected with ND_ERR_IO.
 *
 * TODO: implement.
 */
