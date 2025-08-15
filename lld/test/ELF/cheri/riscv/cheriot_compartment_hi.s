# REQUIRES: riscv
# RUN: llvm-mc -triple=riscv32cheriot-unknown-cheriotrtos -mcpu=cheriot -mattr=+c,+xcheri,+xcheriot -filetype=obj %s -o %t.o
# RUN: ld.lld %t.o -o %t.exe
# RUN: llvm-objdump -d %t.exe | FileCheck %s

	.attribute	4, 16
	.attribute	5, "rv32e2p0_m2p0_c2p0_zmmul1p0_xcheri0p0_xcheriot1p0"
	.section	.text,"ax",@progbits
	.globl	_start
	.p2align	1
	.type	_start,@function
_start:                              # @_Z5entryv
	ct.auipcc	ct1, %cheriot_compartment_hi(near)
	ct.auipcc	ct1, %cheriot_compartment_hi(mid)

# CHECK:        00012000 <_start>:
# CHECK-NEXT:   12000: 00000317      ct.auipcc       ct1, 0x0
# CHECK-NEXT:   12004: 00001317      ct.auipcc       ct1, 0x1

	.type	near,@function
	.p2align	3, 0x0
near:
	.word 1

# CHECK:      00012008 <near>:
# CHECK-NEXT: 01 00 00 00   .word   0x00000001

	.type	mid,@function
	.p2align	12, 0x0
mid:
	.word 1

# CHECK:      00013000 <mid>:
# CHECK-NEXT: 13000: 01 00 00 00