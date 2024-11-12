# RUN: llvm-mc -triple riscv32-unknown-unknown --mcpu=cheriot %s -filetype=obj -o %t1.o
# RUN: llvm-mc -triple riscv32-unknown-unknown -mcpu=cheriot %S/Inputs/cheriot-ccall-spacer.s -filetype=obj -o %t2.o
# RUN: ld.lld %t1.o %t2.o -Bstatic -X --no-relax -o %t3.o
# RUN: llvm-objdump -d %t3.o | FileCheck %s

# Test that Cheriot's 11-bit AUIPCC shifts are correctly calculated.
# Originally reported as https://github.com/CHERIoT-Platform/llvm-project/issues/106

# CHECK-LABEL: 00011100 <start>:
# CHECK-NEXT:    11100: 00001317      ct.auipcc       t1, 0x1
# CHECK-NEXT:    11104: 01830067      ct.cjr  0x18(t1)

# CHECK-LABEL:  00011110 <spacer>:
# CHECK-NEXT:     11110: 0050006f      j       0x11914 <spacer+0x804>
# CHECK:          11914: 00008067      ct.cret

# CHECK-LABEL:  00011918 <start2>:
# CHECK-NEXT:     11918: 0050006f      j       0x1211c <start2+0x804>
# CHECK:          1211c: 10500073      wfi
# CHECK-NEXT:     12120: 0020006f      j       0x12122 <start2+0x80a>

  .attribute	4, 16
  .attribute	5, "rv32e2p0_m2p0_c2p0_zmmul1p0_zca1p0_xcheri0p0_xcheriot1p0_xcheripurecap0p0"
  .text
  .p2align 2
start:
	ct.ctail	start2
