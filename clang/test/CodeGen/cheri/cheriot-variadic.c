// RUN: %clang_cc1 %s -o - "-triple" "riscv32-unknown-unknown" "-emit-llvm" "-mframe-pointer=none" "-mcmodel=small" "-target-cpu" "cheriot" "-target-feature" "+xcheri" "-target-feature" "-64bit" "-target-feature" "-relax" "-target-feature" "-xcheri-rvc" "-target-feature" "-save-restore" "-target-abi" "cheriot" "-Oz" "-Werror" "-cheri-compartment=example" -std=c2x | FileCheck %s

typedef __builtin_va_list va_list;
#define va_start(v, l) __builtin_va_start((v), l)
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg
#define va_copy __builtin_va_copy

typedef struct {
  void *a;
  int b;
  char c[4];
} bar_t;

extern int onward(void *, int, char *);

int foo(va_list ap) {
  // Make sure that we don't see a memcpy in address space zero!
  // CHECK-NOT: p0i8
  bar_t x = va_arg(ap, bar_t);
  return onward(x.a, x.b, x.c);
}
