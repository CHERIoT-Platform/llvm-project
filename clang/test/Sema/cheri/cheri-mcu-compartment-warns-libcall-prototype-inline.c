// RUN: %clang_cc1 %s -o - -triple riscv32-unknown-cheriotrtos -emit-llvm -mframe-pointer=none -mcmodel=small -target-abi cheriot -target-feature +xcheriot -Oz -Werror -verify=libcall
// RUN: %clang_cc1 %s -o - -triple riscv32-unknown-cheriotrtos -emit-llvm -mframe-pointer=none -mcmodel=small -target-abi cheriot -target-feature +xcheriot -Oz -Werror -verify=wrong-compartment -cheri-compartment=wrong

__attribute__((cheriot_libcall))
inline int inlineDefinition(int a, int b)
// wrong-compartment-error@-1{{CHERI libcall should have a prototype in a header with a matching libcall annotation}}
// libcall-error@-2{{CHERI libcall should have a prototype in a header with a matching libcall annotation}}
{
	return a+b;
}
