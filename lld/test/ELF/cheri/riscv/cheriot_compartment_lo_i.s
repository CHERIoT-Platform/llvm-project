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
    ct.clc	ct1, %cheriot_compartment_lo_i(_start)(ct1)
.MID_BLOCK:
	ct.auipcc	ct1, %cheriot_compartment_hi(mid)
    ct.clc	ct1, %cheriot_compartment_lo_i(.MID_BLOCK)(ct1)
.CGP_BLOCK:
	ct.auipcc	ct1, %cheriot_compartment_hi(cgp)
    ct.clw	ra, %cheriot_compartment_lo_i(.CGP_BLOCK)(ct1)
.CGP_FAR_BLOCK:
	ct.auipcc	ct1, %cheriot_compartment_hi(cgp_far)
    ct.clw	ra, %cheriot_compartment_lo_i(.CGP_FAR_BLOCK)(ct1)

# CHECK:        00012000 <_start>:
# CHECK-NEXT:   12000: 00000317      ct.auipcc       ct1, 0x0
# CHECK-NEXT:   12004: 02033303      ct.clc  ct1, 0x20(ct1)

# CHECK:        00012008 <.MID_BLOCK>:
# CHECK-NEXT:   12008: 00001317      ct.auipcc       ct1, 0x1
# CHECK-NEXT:   1200c: 7f833303      ct.clc  ct1, 0x7f8(ct1)

# CHECK: 00012010 <.CGP_BLOCK>:
# CHECK-NEXT: 12010: ffffe37b      ct.auicgp       ct1, 0xffffe
# CHECK-NEXT: 12014: ffc32083      ct.clw  ra, -0x4(ct1)

# CHECK: 00012018 <.CGP_FAR_BLOCK>:
# CHECK-NEXT: 12018: 0000237b      ct.auicgp       ct1, 0x2
# CHECK-NEXT: 1201c: 00032083      ct.clw  ra, 0x0(ct1)

	.type	near,@function
	.p2align	3, 0x0
near:
	.word 1

# CHECK:      00012020 <near>:
# CHECK-NEXT: 12020: 01 00 00 00 .word 0x00000001

	.type	mid,@function
	.p2align	12, 0x0
mid:
	.word 1

# CHECK:      00013000 <mid>:
# CHECK-NEXT: 13000: 01 00 00 00

.section        .data,"aw",@progbits
.type   cgp,@object
.globl  cgp
.p2align        4, 0x0
cgp:
	.word 2
.zero 8192
cgp_far:
	.word 3