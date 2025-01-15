// RUN: %clang_cc1 "-triple" "riscv32cheriot-unknown-cheriotrtos" "-mframe-pointer=none" "-mcmodel=small" "-target-abi" "cheriot" "-Oz" "-Werror" "-cheri-compartment=example" -std=c2x -o - %s -fsyntax-only -verify

__attribute__((cheri_compartment("example")))
int readback(void)
{
	return __cheriot_minimum_stack__; // expected-error{{__cheriot_minimum_stack__ can only be used in a function with a minimum stack annotation}}
}
