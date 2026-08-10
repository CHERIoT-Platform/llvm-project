// RUN: %clang_cc1 %s -o - -triple riscv32cheriot-unknown-cheriotrtos -emit-llvm -mframe-pointer=none -mcmodel=small -target-abi cheriot -target-feature +xcheriot -Oz -Werror -verify=wrong-compartment -cheri-compartment=wrong

__attribute__((cheriot_compartment("example")))
int shouldBePrototype(void) // libcall-error{{CHERI compartment entry declared for compartment 'example' but implemented in '' (provided with -cheri-compartment=)}} wrong-compartment-error{{CHERI compartment entry declared for compartment 'example' but implemented in 'wrong' (provided with -cheri-compartment=)}}
{
	return 1;
}
