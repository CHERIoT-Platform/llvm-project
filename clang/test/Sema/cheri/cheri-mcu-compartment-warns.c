// RUN: %clang_cc1 %s -o - -triple riscv32cheriot-unknown-cheriotrtos -emit-llvm -mframe-pointer=none -mcmodel=small -target-abi cheriot -Oz -Werror -verify=libcall
// RUN: %clang_cc1 %s -o - -triple riscv32cheriot-unknown-cheriotrtos -emit-llvm -mframe-pointer=none -mcmodel=small -target-abi cheriot -Oz -Werror -verify=wrong-compartment -cheri-compartment=wrong

__attribute__((cheri_libcall))
int add(int a, int b) // wrong-compartment-error{{CHERI libcall exported from compilation unit for compartment 'wrong' (provided with -cheri-compartment=)}}
{
	return a+b;
}

__attribute__((cheri_compartment("example")))
int shouldBePrototype(void) // libcall-error{{CHERI compartment entry declared for compartment 'example' but implemented in '' (provided with -cheri-compartment=)}} wrong-compartment-error{{CHERI compartment entry declared for compartment 'example' but implemented in 'wrong' (provided with -cheri-compartment=)}}
{
	return 1;
}
