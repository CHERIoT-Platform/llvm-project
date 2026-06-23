# RUN: llvm-mc %s -filetype=obj -triple=riscv64 -mattr=+xcheri -o %t.o
# RUN: llvm-readobj -r %t.o | FileCheck %s

## Check that we can create R_RISCV_CHERI_CAPABILITY relocations for a range of
## operands: a negative addend, an external/undefined symbol, and references to
## defined symbols (sized and unsized) in both .data and a custom section.
## See https://github.com/CTSRD-CHERI/llvm-project/issues/353
	.type	lmp_head,@object
	.data
	.globl	lmp_head
lmp_head:
	.chericap	lmp_head-2
	.size	lmp_head, 16

	.globl  external_symbol
	.globl  lmp_head_extern
lmp_head_extern:
	.chericap	external_symbol+1
	.size	lmp_head_extern, 16

.Llmp_head_local:
	.chericap	.Llmp_head_local+1
	.size	.Llmp_head_local, 16

.Llmp_head_local_unsized:
	.chericap	.Llmp_head_local_unsized+1

.section .otherdata, "aw", %progbits
.Lreloc_text_local:
.chericap .Lfoo_start + 4
.Lreloc_text_global:
.chericap foo + 4

.text
.global foo
foo:
  nop
.Lfoo_start:
  nop

# CHECK-LABEL: Relocations [
# CHECK-NEXT:   Section ({{.*}}) .rela.data {
# CHECK-NEXT:     0x0 R_RISCV_CHERI_CAPABILITY lmp_head 0xFFFFFFFFFFFFFFFE
# CHECK-NEXT:     0x10 R_RISCV_CHERI_CAPABILITY external_symbol 0x1
# CHECK-NEXT:     0x20 R_RISCV_CHERI_CAPABILITY .Llmp_head_local 0x1
# CHECK-NEXT:     0x30 R_RISCV_CHERI_CAPABILITY .Llmp_head_local_unsized 0x1
# CHECK-NEXT:   }
# CHECK-NEXT:   Section ({{.*}}) .rela.otherdata {
# CHECK-NEXT:     0x0 R_RISCV_CHERI_CAPABILITY .Lfoo_start 0x4
# CHECK-NEXT:     0x10 R_RISCV_CHERI_CAPABILITY foo 0x4
# CHECK-NEXT:   }
