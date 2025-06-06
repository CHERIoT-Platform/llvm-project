// RUN: %riscv32_cheri_cc1 "-triple" "riscv32cheriot-unknown-unknown" "-target-abi" "cheriot" -verify %s 
struct Uart {};

__attribute__((cheriot_mmio("uart", "xyz"))) extern struct Uart uart; // expected-error{{the permissions in 'cheriot_mmio' contain unknown permission symbols: 'xyz' (value: 'xyz')}} expected-warning{{global variable definition 'uart' has attribute 'cheriot_mmio' but is not qualified as `volatile`}}
__attribute__((cheriot_mmio("uart", "RR"))) extern volatile struct Uart uart1; // expected-warning{{the permissions in 'cheriot_mmio' contain a duplicate permission symbol: 'R' (value: 'RR')}}
__attribute__((cheriot_mmio("uart", "m"))) extern volatile struct Uart uart2; // expected-error{{the permissions in 'cheriot_mmio' contain ill-formed dependencies: does not contain either read (R) or write (W) (value: 'm')}} 
__attribute__((cheriot_mmio("uart", "R"))) volatile struct Uart uart3; // no warnings or errors, extern is implied 
__attribute__((cheriot_mmio("uart"))) volatile struct Uart uart4 = {};  // expected-error{{global variable definition 'uart4' with attribute 'cheriot_mmio' cannot have an initializer}}
__attribute__((cheriot_mmio("uart"))) volatile struct {int k;} uart5 = {10};  // expected-error{{global variable definition 'uart5' with attribute 'cheriot_mmio' cannot have an initializer}}
__attribute__((cheriot_mmio("uart", "R"))) extern volatile struct Uart *uart6; /* no warnings or errors*/
__attribute__((cheriot_shared_object("exampleK", "RR"))) extern int exampleK; // expected-warning{{the permissions in 'cheriot_shared_object' contain a duplicate permission symbol: 'R' (value: 'RR')}}
__attribute__((cheriot_shared_object("exampleK", "m"))) extern int exampleK; // expected-error{{the permissions in 'cheriot_shared_object' contain ill-formed dependencies: does not contain either read (R) or write (W) (value: 'm')}}
__attribute__((cheriot_shared_object("exampleK", "Wm"))) extern int exampleK; // expected-error{{the permissions in 'cheriot_shared_object' contain ill-formed dependencies: contains mut (m) but does not have both read (R) and cap (c) (value: 'Wm')}}
__attribute__((cheriot_shared_object("exampleK", "R"))) int exampleK; // no warnings or errors, extern is implied 
__attribute__((cheriot_shared_object("exampleK", "R"))) int exampleK2 = 10; // expected-error{{global variable definition 'exampleK2' with attribute 'cheriot_shared_object' cannot have an initializer}}
__attribute__((cheriot_shared_object("exampleK", "R"))) int *exampleK3; /* no warnings or errors */ 
