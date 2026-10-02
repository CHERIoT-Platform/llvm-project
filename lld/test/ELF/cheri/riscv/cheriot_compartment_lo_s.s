# REQUIRES: riscv
# RUN: llvm-mc -triple=riscv32-unknown-cheriotrtos -mcpu=cheriot -mattr=+c,+xcheri,+xcheripurecap,+xcheriot -filetype=obj %s -o %t.o
# RUN: ld.lld %t.o -o %t.exe
# RUN: llvm-objdump -d %t.exe | FileCheck %s

	.attribute	4, 16
	.attribute	5, "rv32e2p0_m2p0_c2p0_zmmul1p0_xcheri0p0_xcheriot1p0_xcheripurecap0p0"
	.section	.text,"ax",@progbits
	.globl	_start
	.align	1
	.type	_start,@function
	.option relax
.CGP_BLOCK:
	ct.auipcc.data	t1, %cheriot_compartment_data_hi(cgp_label+4)
    ct.csw	ra, %cheriot_compartment_lo_s(.CGP_BLOCK)(t1)
.CGP_MID_BLOCK:
	ct.auipcc.data	t1, %cheriot_compartment_data_hi(cgp_mid_label+8)
    ct.csw	ra, %cheriot_compartment_lo_s(.CGP_MID_BLOCK)(t1)
.CGP_FAR_BLOCK:
	ct.auipcc.data	t1, %cheriot_compartment_data_hi(cgp_far_label+12)
    ct.csw	ra, %cheriot_compartment_lo_s(.CGP_FAR_BLOCK)(t1)

# CHECK:      000110f8 <.CGP_BLOCK>:
# CHECK-NEXT: 110f8: 00000317      ct.auipcc t1, 0x0
# CHECK-NEXT: 110fc: 02033303      ct.clc  t1, 0x20(t1)
# CHECK-NEXT: 11100: 00132023      ct.csw  ra, 0x0(t1)

# CHECK:      00011104 <.CGP_MID_BLOCK>:
# CHECK-NEXT: 11104: 0011a323      ct.csw ra, 0x6(gp)

# CHECK:      00011108 <.CGP_FAR_BLOCK>:
# CHECK-NEXT: 11108: 00000317      ct.auipcc t1, 0x0
# CHECK-NEXT: 1110c: 01833303      ct.clc  t1, 0x18(t1)
# CHECK-NEXT: 11110: 00132023      ct.csw  ra, 0x0(t1)

.section        .data,"aw",@progbits
.type   cgp_label,@object
.globl  cgp_label
.align        4
cgp_label:
	.word 2
.zero 4096
cgp_mid_label:
	.word 3
.zero 4096
cgp_far_label:
	.word 4
