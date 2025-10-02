# RUN: llvm-mc %s -triple=riscv32 -mattr=+xcheri -riscv-no-aliases -show-encoding \
# RUN:     | FileCheck -check-prefixes=CHECK,CHECK-INST %s
# RUN: llvm-mc -filetype=obj -triple riscv32 -mattr=+xcheri < %s \
# RUN:     | llvm-objdump --no-print-imm-hex -M no-aliases --mattr=+xcheri -d - \
# RUN:     | FileCheck -check-prefix=CHECK-INST %s

# CHECK-INST: lc ra, 3(sp)
# CHECK: encoding: [0x83,0x30,0x31,0x00]
lc c1, 3(x2)

# CHECK-INST: sc ra, 3(sp)
# CHECK: encoding: [0xa3,0x31,0x11,0x00]
sc c1, 3(x2)

# CHECK-INST: lc.ddc ra, (sp)
# CHECK: encoding: [0xdb,0x00,0x31,0xfa]
lc.ddc c1, (x2)

# CHECK-INST: sc.ddc ra, (sp)
# CHECK: encoding: [0xdb,0x01,0x11,0xf8]
sc.ddc c1, (x2)

# CHECK-INST: lc.cap ra, (sp)
# CHECK: encoding: [0xdb,0x00,0xb1,0xfa]
lc.cap c1, (c2)

# CHECK-INST: sc.cap ra, (sp)
# CHECK: encoding: [0xdb,0x05,0x11,0xf8]
sc.cap c1, (c2)
