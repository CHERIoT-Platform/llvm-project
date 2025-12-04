# REQUIRES: riscv
# RUN: llvm-mc -triple=riscv32cheriot-unknown-cheriotrtos -mcpu=cheriot -mattr=+c,+xcheri,+xcheripurecap,+xcheriot -filetype=obj %s -o %t.o
# RUN: ld.lld %t.o -o %t.exe
# RUN: llvm-objdump -d %t.exe | FileCheck %s

	.attribute	4, 16
	.attribute	5, "rv32e2p0_m2p0_c2p0_zmmul1p0_xcheri0p0_xcheriot1p0_xcheripurecap0p0"
	.section	.text,"ax",@progbits
	.globl	_start
	.p2align	1
	.type	_start,@function
.CGP_BLOCK:
	ct.auipcc	t1, %cheriot_compartment_hi(cgp_label)
    ct.csw	ra, %cheriot_compartment_lo_s(.CGP_BLOCK)(t1)
.CGP_FAR_BLOCK:
	ct.auipcc	t1, %cheriot_compartment_hi(cgp_far_label)
    ct.csw	ra, %cheriot_compartment_lo_s(.CGP_FAR_BLOCK)(t1)

# CHECK: 000110f4 <.CGP_BLOCK>:
# CHECK-NEXT: 110f4: ffffe37b      ct.auicgp       t1, 0xffffe
# CHECK-NEXT: 110f8: fe132e23      ct.csw  ra, -0x4(t1)

# CHECK: 000110fc <.CGP_FAR_BLOCK>:
# CHECK-NEXT: 110fc: 0000237b      ct.auicgp       t1, 0x2
# CHECK-NEXT: 11100: 00132023      ct.csw  ra, 0x0(t1)

.section        .data,"aw",@progbits
.type   cgp_label,@object
.globl  cgp_label
.p2align        4, 0x0
cgp_label:
	.word 2
.zero 8192
cgp_far_label:
	.word 3
