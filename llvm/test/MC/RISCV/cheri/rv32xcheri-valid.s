# RUN: llvm-mc %s -triple=riscv32 -mattr=+xcheri -riscv-no-aliases -show-encoding \
# RUN:     | FileCheck -check-prefixes=CHECK,CHECK-INST %s
# RUN: llvm-mc %s -triple=riscv64 -mattr=+xcheri -riscv-no-aliases -show-encoding \
# RUN:     | FileCheck -check-prefixes=CHECK,CHECK-INST %s
# RUN: llvm-mc -filetype=obj -triple riscv32 -mattr=+xcheri < %s \
# RUN:     | llvm-objdump --no-print-imm-hex -M no-aliases --mattr=+xcheri -d - \
# RUN:     | FileCheck -check-prefix=CHECK-INST %s
# RUN: llvm-mc -filetype=obj -triple riscv64 -mattr=+xcheri < %s \
# RUN:     | llvm-objdump --no-print-imm-hex -M no-aliases --mattr=+xcheri -d - \
# RUN:     | FileCheck -check-prefix=CHECK-INST %s

# CHECK-INST: cgetperm ra, sp
# CHECK: encoding: [0xdb,0x00,0x01,0xfe]
cgetperm x1, c2
# CHECK-INST: cgettype ra, sp
# CHECK: encoding: [0xdb,0x00,0x11,0xfe]
cgettype x1, c2
# CHECK-INST: cgetbase ra, sp
# CHECK: encoding: [0xdb,0x00,0x21,0xfe]
cgetbase x1, c2
# CHECK-INST: cgetlen ra, sp
# CHECK: encoding: [0xdb,0x00,0x31,0xfe]
cgetlen x1, c2
# CHECK-INST: cgettag ra, sp
# CHECK: encoding: [0xdb,0x00,0x41,0xfe]
cgettag x1, c2
# CHECK-INST: cgetsealed ra, sp
# CHECK: encoding: [0xdb,0x00,0x51,0xfe]
cgetsealed x1, c2
# CHECK-INST: cgetoffset ra, sp
# CHECK: encoding: [0xdb,0x00,0x61,0xfe]
cgetoffset x1, c2
# CHECK-INST: cgetflags ra, sp
# CHECK: encoding: [0xdb,0x00,0x71,0xfe]
cgetflags x1, c2
# CHECK-INST: addi ra, sp, 0
# CHECK: encoding: [0x93,0x00,0x01,0x00]
cgetaddr x1, c2
# CHECK-INST: cgethigh ra, sp
# CHECK: encoding: [0xdb,0x00,0x71,0xff]
cgethigh x1, c2

# CHECK-INST: cseal ra, sp, gp
# CHECK: encoding: [0xdb,0x00,0x31,0x16]
cseal c1, c2, c3
# CHECK-INST: cunseal ra, sp, gp
# CHECK: encoding: [0xdb,0x00,0x31,0x18]
cunseal c1, c2, c3
# CHECK-INST: candperm ra, sp, gp
# CHECK: encoding: [0xdb,0x00,0x31,0x1a]
candperm c1, c2, x3
# CHECK-INST: csetflags ra, sp, gp
# CHECK: encoding: [0xdb,0x00,0x31,0x1c]
csetflags c1, c2, x3
# CHECK-INST: csetoffset ra, sp, gp
# CHECK: encoding: [0xdb,0x00,0x31,0x1e]
csetoffset c1, c2, x3
# CHECK-INST: csetaddr ra, sp, gp
# CHECK: encoding: [0xdb,0x00,0x31,0x20]
csetaddr c1, c2, x3
# CHECK-INST: csethigh ra, sp, gp
# CHECK: encoding: [0xdb,0x00,0x31,0x2c]
csethigh c1, c2, x3
# CHECK-INST: cincoffset ra, sp, gp
# CHECK: encoding: [0xdb,0x00,0x31,0x22]
cincoffset c1, c2, x3
# CHECK-INST: cincoffset ra, sp, -173
# CHECK: encoding: [0xdb,0x10,0x31,0xf5]
cincoffset c1, c2, -173
# CHECK-INST: cincoffset ra, sp, -173
# CHECK: encoding: [0xdb,0x10,0x31,0xf5]
cincoffsetimm c1, c2, -173
# CHECK-INST: csetbounds ra, sp, gp
# CHECK: encoding: [0xdb,0x00,0x31,0x10]
csetbounds c1, c2, x3
# CHECK-INST: csetboundsexact ra, sp, gp
# CHECK: encoding: [0xdb,0x00,0x31,0x12]
csetboundsexact c1, c2, x3
# CHECK-INST: csetbounds ra, sp, 3029
# CHECK: encoding: [0xdb,0x20,0x51,0xbd]
csetbounds c1, c2, 0xbd5
# CHECK-INST: csetbounds ra, sp, 3029
# CHECK: encoding: [0xdb,0x20,0x51,0xbd]
csetboundsimm c1, c2, 0xbd5
# CHECK-INST: ccleartag ra, sp
# CHECK: encoding: [0xdb,0x00,0xb1,0xfe]
ccleartag c1, c2
# CHECK-INST: cbuildcap ra, sp, gp
# CHECK: encoding: [0xdb,0x00,0x31,0x3a]
cbuildcap c1, c2, c3
# CHECK-INST: cbuildcap ra, ddc, gp
# CHECK: encoding: [0xdb,0x00,0x30,0x3a]
cbuildcap c1, ddc, c3
# CHECK-INST: ccopytype ra, sp, gp
# CHECK: encoding: [0xdb,0x00,0x31,0x3c]
ccopytype c1, c2, c3
# CHECK-INST: ccseal ra, sp, gp
# CHECK: encoding: [0xdb,0x00,0x31,0x3e]
ccseal c1, c2, c3
# CHECK-INST: csealentry ra, sp
# CHECK: encoding: [0xdb,0x00,0x11,0xff]
csealentry c1, c2

# CHECK-INST: sub ra, sp, gp
# CHECK: encoding: [0xb3,0x00,0x31,0x40]
csub x1, c2, c3
# CHECK-INST: cmove ra, sp
# CHECK: encoding: [0xdb,0x00,0xa1,0xfe]
cmove c1, c2

# CHECK-INST: jalr.cap ra, sp
# CHECK: encoding: [0xdb,0x00,0xc1,0xfe]
jalr.cap c1, c2
# CHECK-INST: jalr.cap ra, sp
# CHECK: encoding: [0xdb,0x00,0xc1,0xfe]
jalr.cap c2
# CHECK-INST: jalr.cap zero, ra
# CHECK: encoding: [0x5b,0x80,0xc0,0xfe]
jr.cap c1
# CHECK-INST: jalr.cap zero, ra
# CHECK: encoding: [0x5b,0x80,0xc0,0xfe]
ret.cap
# CHECK-INST: jalr.pcc ra, sp
# CHECK: encoding: [0xdb,0x00,0x41,0xff]
jalr.pcc x1, x2
# CHECK-INST: jalr.pcc ra, sp
# CHECK: encoding: [0xdb,0x00,0x41,0xff]
jalr.pcc x2
# CHECK-INST: jalr.pcc zero, ra
# CHECK: encoding: [0x5b,0x80,0x40,0xff]
jr.pcc x1
# CHECK-INST: jalr.pcc zero, ra
# CHECK: encoding: [0x5b,0x80,0x40,0xff]
ret.pcc
# CHECK-INST: cinvoke ra, sp
# CHECK: encoding: [0xdb,0x80,0x20,0xfc]
cinvoke c1, c2

# CHECK-INST: ctestsubset ra, sp, gp
# CHECK: encoding: [0xdb,0x00,0x31,0x40]
ctestsubset x1, c2, c3
# CHECK-INST: ctestsubset ra, ddc, gp
# CHECK: encoding: [0xdb,0x00,0x30,0x40]
ctestsubset x1, ddc, c3

# CHECK-INST: csetequalexact ra, sp, gp
# CHECK: encoding: [0xdb,0x00,0x31,0x42]
csetequalexact x1, c2, c3
# CHECK-INST: csetequalexact ra, sp, gp
# CHECK: encoding: [0xdb,0x00,0x31,0x42]
cseqx x1, c2, c3

# CHECK-INST: cspecialrw ra, uscratchc, sp
# CHECK: encoding: [0xdb,0x00,0x61,0x02]
cspecialrw c1, uscratchc, c2
# CHECK-INST: cspecialrw ra, uscratchc, sp
# CHECK: encoding: [0xdb,0x00,0x61,0x02]
cspecialrw c1, 0x6, c2
# CHECK-INST: cspecialrw ra, uscratchc, zero
# CHECK: encoding: [0xdb,0x00,0x60,0x02]
cspecialr c1, uscratchc
# CHECK-INST: cspecialrw zero, uscratchc, sp
# CHECK: encoding: [0x5b,0x00,0x61,0x02]
cspecialw uscratchc, c2

# CHECK-INST: cclear 1, 66
# CHECK: encoding: [0x5b,0x01,0xe5,0xfe]
cclear 1, 0x42
# CHECK-INST: fpclear 1, 66
# CHECK: encoding: [0x5b,0x01,0x05,0xff]
fpclear 1, 0x42

# CHECK-INST: croundrepresentablelength ra, sp
# CHECK: encoding: [0xdb,0x00,0x81,0xfe]
croundrepresentablelength x1, x2
# CHECK-INST: croundrepresentablelength ra, sp
# CHECK: encoding: [0xdb,0x00,0x81,0xfe]
crrl x1, x2
# CHECK-INST: crepresentablealignmentmask ra, sp
# CHECK: encoding: [0xdb,0x00,0x91,0xfe]
crepresentablealignmentmask x1, x2
# CHECK-INST: crepresentablealignmentmask ra, sp
# CHECK: encoding: [0xdb,0x00,0x91,0xfe]
cram x1, x2

# CHECK-INST: cloadtags ra, (sp)
# CHECK: encoding: [0xdb,0x00,0x21,0xff]
cloadtags x1, (c2)

# CHECK-INST: lb.ddc ra, (sp)
# CHECK: encoding: [0xdb,0x00,0x01,0xfa]
lb.ddc x1, (x2)
# CHECK-INST: lh.ddc ra, (sp)
# CHECK: encoding: [0xdb,0x00,0x11,0xfa]
lh.ddc x1, (x2)
# CHECK-INST: lw.ddc ra, (sp)
# CHECK: encoding: [0xdb,0x00,0x21,0xfa]
lw.ddc x1, (x2)
# CHECK-INST: lbu.ddc ra, (sp)
# CHECK: encoding: [0xdb,0x00,0x41,0xfa]
lbu.ddc x1, (x2)
# CHECK-INST: lhu.ddc ra, (sp)
# CHECK: encoding: [0xdb,0x00,0x51,0xfa]
lhu.ddc x1, (x2)

# CHECK-INST: sb.ddc ra, (sp)
# CHECK: encoding: [0x5b,0x00,0x11,0xf8]
sb.ddc x1, (x2)
# CHECK-INST: sh.ddc ra, (sp)
# CHECK: encoding: [0xdb,0x00,0x11,0xf8]
sh.ddc x1, (x2)
# CHECK-INST: sw.ddc ra, (sp)
# CHECK: encoding: [0x5b,0x01,0x11,0xf8]
sw.ddc x1, (x2)

# CHECK-INST: lb.cap ra, (sp)
# CHECK: encoding: [0xdb,0x00,0x81,0xfa]
lb.cap x1, (c2)
# CHECK-INST: lh.cap ra, (sp)
# CHECK: encoding: [0xdb,0x00,0x91,0xfa]
lh.cap x1, (c2)
# CHECK-INST: lw.cap ra, (sp)
# CHECK: encoding: [0xdb,0x00,0xa1,0xfa]
lw.cap x1, (c2)
# CHECK-INST: lbu.cap ra, (sp)
# CHECK: encoding: [0xdb,0x00,0xc1,0xfa]
lbu.cap x1, (c2)
# CHECK-INST: lhu.cap ra, (sp)
# CHECK: encoding: [0xdb,0x00,0xd1,0xfa]
lhu.cap x1, (c2)

# CHECK-INST: sb.cap ra, (sp)
# CHECK: encoding: [0x5b,0x04,0x11,0xf8]
sb.cap x1, (c2)
# CHECK-INST: sh.cap ra, (sp)
# CHECK: encoding: [0xdb,0x04,0x11,0xf8]
sh.cap x1, (c2)
# CHECK-INST: sw.cap ra, (sp)
# CHECK: encoding: [0x5b,0x05,0x11,0xf8]
sw.cap x1, (c2)
