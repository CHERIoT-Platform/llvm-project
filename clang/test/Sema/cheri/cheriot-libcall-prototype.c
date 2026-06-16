// RUN: %riscv32_cheri_cc1 "-triple" "riscv32cheriot-unknown-cheriotrtos" "-target-abi" "cheriot" -verify %s

int a() __attribute__((cheriot_libcall)); // expected-warning{{CHERI libcall should have a prototype in a header with a matching libcall annotation}}
int a() __attribute__((cheriot_libcall)) { return 0; }

int b() __attribute__((cheriot_libcall)) { return 0; } // expected-warning{{CHERI libcall should have a prototype in a header with a matching libcall annotation}}

int c(); // expected-note{{previous declaration is here}}
int c() __attribute__((cheriot_libcall)) { return 0; }
// expected-error@-1{{function declared 'cheri_libcall' here was previously declared without calling convention}}
// expected-warning@-2{{CHERI libcall should have a prototype in a header with a matching libcall annotation}}
