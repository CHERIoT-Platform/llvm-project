# RUN: llvm-mc -triple riscv32-unknown-unknown --mcpu=cheriot %s -filetype=obj -o %t1.o
# RUN: llvm-mc -triple riscv32-unknown-unknown -mcpu=cheriot %S/Inputs/cheriot-ccall-spacer.s -filetype=obj -o %t2.o
# RUN: ld.lld %t1.o %t2.o -Bstatic -X --no-relax -o %t3.o
# RUN: llvm-objdump -d %t3.o | FileCheck %s

# Test that Cheriot's 11-bit AUIPCC shifts are correctly calculated.
# Originally reported as https://github.com/CHERIoT-Platform/llvm-project/issues/106

# CHECK-LABEL: 000110e0 <start>:
# CHECK-NEXT:    110e0: 00001317      ct.auipcc       t1, 0x1
# CHECK-NEXT:    110e4: 01830067      ct.cjr  0x18(t1)

# CHECK-LABEL:  000110f0 <spacer>:
# CHECK-NEXT:     110f0: 0050006f      j       0x118f4 <spacer+0x804>
# CHECK:          118f4: 00008067      ct.cret

# CHECK-LABEL:  000118f8 <start2>:
# CHECK-NEXT:     118f8: 0050006f      j       0x120fc <start2+0x804>
# CHECK:          120fc: 10500073      wfi
# CHECK-NEXT:     12100: 0020006f      j       0x12102 <start2+0x80a>

  .attribute	4, 16
  .attribute	5, "rv32e2p0_m2p0_c2p0_zmmul1p0_zca1p0_xcheri0p0_xcheriot1p0_xcheripurecap0p0"
  .text
  .p2align 2
start:
	ct.ctail	start2
