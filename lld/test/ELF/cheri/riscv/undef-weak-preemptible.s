# REQUIRES: riscv
## Undefined weak capability references that are preemptible, because the
## executable links against a shared library. For -no-pie they resolve
## statically to NULL-derived capabilities with no dynamic relocations, like
## non-preemptible ones (see undef-weak.s). For -pie they need dynamic
## relocations.

# RUN: rm -rf %t && split-file %s %t && cd %t
# RUN: %riscv64_cheri_purecap_llvm-mc -filetype=obj main.s -o main.o
# RUN: %riscv64_cheri_purecap_llvm-mc -filetype=obj lib.s -o lib.o
# RUN: ld.lld -shared lib.o -o lib.so

# RUN: ld.lld -no-pie main.o lib.so -o nopie
# RUN: llvm-readelf --dyn-syms nopie | FileCheck --check-prefix=DYNSYM %s
# RUN: llvm-readobj -r --cap-relocs nopie | FileCheck --check-prefix=NOPIE-RELOC %s
# RUN: llvm-readobj -x .data nopie | FileCheck --check-prefix=NOPIE-DATA %s

# RUN: ld.lld -pie main.o lib.so -o pie
# RUN: llvm-readelf --dyn-syms pie | FileCheck --check-prefix=DYNSYM %s
# RUN: llvm-readobj -r --cap-relocs pie | FileCheck --check-prefix=PIE-RELOC %s

## The symbol is preemptible in both links.
# DYNSYM: WEAK   DEFAULT   UND undef_weak

# NOPIE-RELOC:      Relocations [
# NOPIE-RELOC-NEXT: ]
# NOPIE-RELOC-NEXT: CHERI Capability Relocations [
# NOPIE-RELOC-NEXT: ]

## NULL-derived capabilities with addresses 0 and 3.
# NOPIE-DATA:      00000000 00000000 00000000 00000000
# NOPIE-DATA-NEXT: 03000000 00000000 00000000 00000000

# PIE-RELOC:      Relocations [
# PIE-RELOC-NEXT:   Section ({{.+}}) .rela.dyn {
# PIE-RELOC-NEXT:     0x{{[0-9A-Fa-f]+}} R_RISCV_CHERI_CAPABILITY undef_weak 0x0
# PIE-RELOC-NEXT:     0x{{[0-9A-Fa-f]+}} R_RISCV_CHERI_CAPABILITY undef_weak 0x3
# PIE-RELOC-NEXT:   }
# PIE-RELOC-NEXT: ]
# PIE-RELOC-NEXT: CHERI Capability Relocations [
# PIE-RELOC-NEXT: ]

#--- main.s
.global _start
_start:
  cret

.weak undef_weak
.data
.balign 16
.chericap undef_weak
.chericap undef_weak + 3

#--- lib.s
.global libfn
libfn:
  cret
