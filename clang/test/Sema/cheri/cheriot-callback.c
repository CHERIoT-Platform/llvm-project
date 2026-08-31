// RUN: %riscv32_cheri_cc1 "-triple" "riscv32-unknown-cheriotrtos" "-target-abi" "cheriot" -verify %s


typedef __attribute__((cheriot_ccallback)) void (*Callback)();
void doesNotHaveAttribute() { }
void __attribute__((cheriot_ccallback)) hasAttribute() { }

__attribute__((cheri_compartment("bar"))) void crossCompartmentCall(Callback f);
// expected-note@-1{{passing argument to parameter 'f' here}}

__attribute__((cheri_compartment("test")))
void test_1() {
    crossCompartmentCall(&doesNotHaveAttribute); // expected-error{{passing 'void (*)()' to parameter of incompatible type 'Callback' (aka 'void (*)(void) __attribute__((cheri_ccallback))')}}
}

__attribute__((cheri_compartment("test")))
void test_2() {
    crossCompartmentCall(&hasAttribute);
}

__attribute__((cheri_compartment("test")))
void test_3(int i) {
    Callback f = i ? &doesNotHaveAttribute : &hasAttribute;
    // expected-warning@-1{{pointer type mismatch ('void (*)()' and 'void (*)() __attribute__((cheri_ccallback))')}}
    // expected-error@-2{{initializing 'Callback' (aka 'void (*)(void) __attribute__((cheri_ccallback))') with an expression of incompatible type 'void *'}}
    crossCompartmentCall(f);
}
