// RUN: %clang_cc1 %s -o - "-triple" "riscv32-unknown-unknown" "-emit-llvm" "-mframe-pointer=none" "-mcmodel=small" "-target-cpu" "cheriot" "-target-feature" "+xcheri" "-target-feature" "-64bit" "-target-feature" "-relax" "-target-feature" "-xcheri-rvc" "-target-feature" "-save-restore" "-target-abi" "cheriot" "-Oz" "-Werror" "-cheri-compartment=example" -std=c2x | FileCheck %s
int foo(void);

[[cheriot::minimum_stack(256)]]
// CHECK: _Z8disabledv() 
// CHECK-SAME: #0
__attribute__((cheri_compartment("example")))
int disabled(void)
{
	return foo();
}

// CHECK: attributes #0 =
// CHECK-SAME: "minimum-stack-size"="256"
