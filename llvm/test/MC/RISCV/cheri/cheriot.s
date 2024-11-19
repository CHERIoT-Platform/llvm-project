# RUN: llvm-mc %s -triple=riscv32cheriot -mcpu=cheriot -mattr=+xcheri -riscv-no-aliases -show-encoding \
# RUN:     | FileCheck %s

csetboundsrounddown cra, cra, zero
# CHECK: encoding: [0xdb,0x80,0x00,0x14]
csetboundsrounddown cra, ca5, zero
# CHECK: encoding: [0xdb,0x80,0x07,0x14]
csetboundsrounddown cra, cra, a5
# CHECK: encoding: [0xdb,0x80,0xf0,0x14]
csetboundsrounddown cra, ca5, a5
# CHECK: encoding: [0xdb,0x80,0xf7,0x14]
csetboundsrounddown ca5, cra, zero
# CHECK: [0xdb,0x87,0x00,0x14]
csetboundsrounddown ca5, ca5, zero
# CHECK: [0xdb,0x87,0x07,0x14]
csetboundsrounddown ca5, cra, a5
# CHECK: [0xdb,0x87,0xf0,0x14]
csetboundsrounddown ca5, ca5, a5
# CHECK: [0xdb,0x87,0xf7,0x14]
