// RUN: %riscv32_cheri_cc1 "-triple" "riscv32cheriot-unknown-unknown" "-target-abi" "cheriot" -verify %s 

void func() {
  int *k1 = __builtin_cheriot_sealing_type(MySealingType); // expected-error{{use of undeclared identifier 'MySealingType'}}
  int *k2 = __builtin_cheriot_sealing_type("This is my key"); // expected-error{{the sealing key type name 'This is my key' is not a valid identifier}}
  int *k3 = __builtin_cheriot_sealing_type("αβγδ"); // expected-error{{the sealing key type name '\CE\B1\CE\B2\CE\B3\CE\B4' is not a valid identifier}}
  int *k4 = __builtin_cheriot_sealing_type("a\0b"); // expected-error{{the sealing key type name 'a\00b' is not a valid identifier}}
  int *k5 = __builtin_cheriot_sealing_type("a\"b"); // expected-error{{the sealing key type name 'a\22b' is not a valid identifier}}
  int *k6 = __builtin_cheriot_sealing_type("a\nb"); // expected-error{{the sealing key type name 'a\0Ab' is not a valid identifier}}
  int *k7 = __builtin_cheriot_sealing_type("a\bb"); // expected-error{{the sealing key type name 'a\08b' is not a valid identifier}}
  int *k8 = __builtin_cheriot_sealing_type("ab"); // expected-error{{__builtin_cheriot_sealing_type used, but no compartment name given}}
}
