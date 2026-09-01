// RUN: %riscv32_cheri_cc1 "-triple" "riscv32-unknown-cheriotrtos" "-target-abi" "cheriot" -verify %s

typedef void (*FnPtr)();
typedef __attribute__((cheriot_ccallback)) void (*FnPtrWithAttribute)();


void foo();
__attribute__((cheriot_ccallback)) void bar();

__attribute__((cheri_compartment("test")))
FnPtr test_1() { // expected-warning{{a function pointer without the cheriot_ccallback attribute as the return value from a compartment call is error-prone}}
    return foo;
}

__attribute__((cheri_compartment("test")))
FnPtrWithAttribute test_2() {
    return bar;
}

__attribute__((cheri_compartment("test")))
void test_3(FnPtr f) {} // expected-warning{{a function pointer without the cheriot_ccallback attribute as a parameter to a compartment call is error-prone}}

__attribute__((cheri_compartment("test")))
void test_4(FnPtrWithAttribute f) {}

__attribute__((cheri_compartment("test")))
void test_5(FnPtr f); // expected-warning{{a function pointer without the cheriot_ccallback attribute as a parameter to a compartment call is error-prone}}
__attribute__((cheri_compartment("test")))
void test_5(FnPtr f) { }
