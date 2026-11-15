# RUN: llvm-mc %s -triple=riscv32 -mcpu=cheriot -mattr=+xcheri,+xcheriot -show-encoding \
# RUN:   | FileCheck %s
# RUN: not llvm-mc %s -triple=riscv32 -mcpu=cheriot -mattr=+xcheri,+xcheriot --defsym=ERR=1 \
# RUN:   2>&1 | FileCheck %s --check-prefix=ERR
# RUN: llvm-mc %s -triple=riscv32 -mcpu=cheriot -mattr=+xcheri,+xcheriot -filetype=obj \
# RUN:   | llvm-objdump -d --mattr=+xcheri,+xcheriot - | FileCheck %s --check-prefix=DIS

## CHERIoT has no DDC: the rs1 operand of ct.ctestsubset is an ordinary
## capability register, so x0 is the null capability (unlike CHERI ISAv9,
## where an rs1 of x0 means DDC).

# CHECK: ct.ctestsubset t2, zero, a3 # encoding: [0xdb,0x03,0xd0,0x40]
# DIS:   ct.ctestsubset t2, zero, a3
ct.ctestsubset t2, cnull, a3
# CHECK: ct.ctestsubset t2, zero, a3 # encoding: [0xdb,0x03,0xd0,0x40]
# DIS:   ct.ctestsubset t2, zero, a3
ctestsubset t2, cnull, a3

.ifdef ERR
# ERR: :[[@LINE+1]]:20: error: register must be a GPR
ct.ctestsubset t2, ddc, a3
.endif
