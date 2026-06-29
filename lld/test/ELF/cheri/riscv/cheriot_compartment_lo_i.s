# REQUIRES: riscv
# RUN: llvm-mc -triple=riscv32-unknown-cheriotrtos -mcpu=cheriot -mattr=+c,+xcheri,+xcheripurecap,+xcheriot -filetype=obj %s -o %t.o
# RUN: ld.lld %t.o -o %t.exe
# RUN: llvm-objdump -d %t.exe | FileCheck %s

	.attribute	4, 16
	.attribute	5, "rv32e2p0_m2p0_c2p0_zmmul1p0_xcheri0p0_xcheriot1p0_xcheripurecap0p0"
	.section	.text,"ax",@progbits
	.globl	_start
	.p2align	1
	.type	_start,@function
	.option relax
_start:                              # @_Z5entryv
	ct.auipcc	t1, %cheriot_compartment_code_hi(near)
    ct.clc	t1, %cheriot_compartment_lo_i(_start)(t1)
.MID_BLOCK:
	ct.auipcc	t1, %cheriot_compartment_code_hi(mid)
    ct.clc	t1, %cheriot_compartment_lo_i(.MID_BLOCK)(t1)
.CGP_BLOCK:
	ct.auipcc.data	t1, %cheriot_compartment_data_hi(cgp)
    ct.clw	ra, %cheriot_compartment_lo_i(.CGP_BLOCK)(t1)
.CGP_FAR_BLOCK:
	ct.auipcc.data	t1, %cheriot_compartment_data_hi(cgp_far)
    ct.clw	ra, %cheriot_compartment_lo_i(.CGP_FAR_BLOCK)(t1)

# CHECK:        00012000 <_start>:
# CHECK-NEXT:   12000: 00000317      ct.auipcc       t1, 0x0
# CHECK-NEXT:   12004: 02833303      ct.clc  t1, 0x28(t1)

# CHECK:        00012008 <.MID_BLOCK>:
# CHECK-NEXT:   12008: 00001317      ct.auipcc       t1, 0x1
# CHECK-NEXT:   1200c: 7f833303      ct.clc  t1, 0x7f8(t1)

# CHECK:        00012010 <.CGP_BLOCK>:
# CHECK-NEXT:   12010: 00001317      ct.auipcc       t1, 0x1
# CHECK-NEXT:   12014: 7f833303      ct.clc  t1, 0x7f8(t1)
# CHECK-NEXT:   12018: 00032083      ct.clw  ra, 0x0(t1)

# CHECK:        0001201c <.CGP_FAR_BLOCK>:
# CHECK-NEXT:   1201c: 00001317      ct.auipcc       t1, 0x1
# CHECK-NEXT:   12020: 7f433303      ct.clc  t1, 0x7f4(t1)
# CHECK-NEXT:   12024: 00032083      ct.clw  ra, 0x0(t1)

	.type	near,@function
	.align	3
near:
	.word 1

# CHECK:      00012028 <near>:
# CHECK-NEXT: 12028: 01 00 00 00 .word 0x00000001

	.type	mid,@function
	.align	12
mid:
	.word 1

# CHECK:      00013000 <mid>:
# CHECK-NEXT: 13000: 01 00 00 00

.section        .data,"aw",@progbits
.type   cgp,@object
.globl  cgp
.align        4
cgp:
	.word 2
.zero 8192
cgp_far:
	.word 3