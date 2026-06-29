# REQUIRES: riscv
# RUN: llvm-mc -triple=riscv32-unknown-cheriotrtos -mcpu=cheriot -mattr=+c,+xcheri,+xcheriot -filetype=obj %s -o %t.o
# RUN: ld.lld %t.o -o %t.exe
# RUN: llvm-objdump -d %t.exe | FileCheck %s

	.attribute	4, 16
	.attribute	5, "rv32e2p0_m2p0_c2p0_zmmul1p0_xcheri0p0_xcheriot1p0"
	.section	.text,"ax",@progbits
	.globl	_start
	.p2align	1
	.type	_start,@function
	.option relax
_start:                              # @_Z5entryv
	ct.auipcc	t1, %cheriot_compartment_hi(near)
	ct.auipcc	t1, %cheriot_compartment_hi(mid)

# CHECK:        00012000 <_start>:
# CHECK-NEXT:   12000: 00000317      ct.auipcc       t1, 0x0
# CHECK-NEXT:   12004: 00001317      ct.auipcc       t1, 0x1

	.type	near,@function
	.align 3
near:
	.word 1

# CHECK:      00012008 <near>:
# CHECK-NEXT: 01 00 00 00   .word   0x00000001

	.type	mid,@function
	.align 12
mid:
	.word 1

# CHECK:      00013000 <mid>:
# CHECK-NEXT: 13000: 01 00 00 00