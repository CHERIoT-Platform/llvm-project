# RUN: llvm-mc %s -triple=riscv32 -mcpu=cheriot -mattr=+xcheri -riscv-no-aliases -show-encoding \
# RUN:     | FileCheck %s
hello:
	auipcc ca0, 12345
	# CHECK: auipcc  ca0, 12345
	# CHECK: encoding: [0x17,0x95,0x03,0x03]
	auicgp cra, 12345
	# CHECK:        auicgp  cra, 12345
	# CHECK: encoding: [0xfb,0x90,0x03,0x03]
	auipcc csp, 12345
	# CHECK:        auipcc  csp, 12345
	# CHECK: encoding: [0x17,0x91,0x03,0x03]
	auicgp csp, 12345
	# CHECK:        auicgp  csp, 12345
	# CHECK: encoding: [0x7b,0x91,0x03,0x03]
