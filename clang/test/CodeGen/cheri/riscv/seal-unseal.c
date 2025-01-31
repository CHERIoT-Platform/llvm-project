// RUN: %riscv32_cheri_cc1 "-triple" "riscv32cheriot-unknown-unknown" "-target-abi" "cheriot" -verify %s 

int * unseal_good1(int * __sealed_capability a, void *b) {
    return __builtin_cheri_unseal(a, b);
}

void * unseal_good2(void * __sealed_capability a, void *b) {
    return __builtin_cheri_unseal(a, b);
}

float * unseal_good3(float * __sealed_capability a, void *b) {
    return __builtin_cheri_unseal(a, b);
}

int * __sealed_capability seal_good1(int *a, void *b) {
    return __builtin_cheri_seal(a, b);
}

void * __sealed_capability seal_good2(void *a, void *b) {
    return __builtin_cheri_seal(a, b);
}

float * __sealed_capability seal_good3(float *a, void *b) {
    return __builtin_cheri_seal(a, b);
}

int * __sealed_capability unseal_bad1(int * __sealed_capability a, void *b) {
    return __builtin_cheri_unseal(a, b); // expected-error{{converting unsealed type 'int *' to sealed type 'int * __sealed_capability' without an explicit sealing}}
}

int * unseal_bad2(int *a, void *b) {
    return __builtin_cheri_unseal(a, b); // expected-error{{operand of type 'int *' where sealed capability is required}}
}

int * seal_bad1(int * a, void * b) {
    return __builtin_cheri_seal(a, b); // expected-error{{converting sealed type 'int * __sealed_capability' to non-sealed type 'int *' without an explicit unsealing}}
}

int * __sealed_capability seal_bad2(int * __sealed_capability a, void *b) {
    return __builtin_cheri_seal(a, b); // expected-error{{operand of type 'int * __sealed_capability' where unsealed capability is required}}
}
