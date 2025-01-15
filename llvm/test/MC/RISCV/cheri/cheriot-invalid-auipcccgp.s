# RUN: not llvm-mc %s -triple=riscv32 -mcpu=cheriot -mattr=+xcheri -riscv-no-aliases -show-encoding \
# RUN:   2>&1  | FileCheck %s
hello:
	auipcc csp, 1<<20 # CHECK: :[[@LINE]]:14: error: operand must be a symbol with
	# CHECK-SAME: cheri_compartment_pccrel_hi
	# CHECK-SAME: or an integer in the range [0, 1048575]
	auicgp csp, 1<<20 # CHECK: :[[@LINE]]:14: error: operand must be a symbol with
	# CHECK-SAME: cheri_compartment_cgprel_hi
	# CHECK-SAME: or an integer in the range [0, 1048575]
