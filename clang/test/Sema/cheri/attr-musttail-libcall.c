// RUN: %riscv32_cheri_cc1 "-triple" "riscv32-unknown-unknown" "-target-abi" "cheriot" -verify -fsyntax-only %s 

// expected-no-diagnostics

__attribute__((cheri_libcall))
extern int bar(void);

int foo(void) {
    [[clang::musttail]] return bar();
}
