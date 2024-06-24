// RUN: %clang_cc1 %s -o - "-triple" "riscv32-unknown-unknown" "-emit-llvm" "-mframe-pointer=none" "-mcmodel=small" "-target-cpu" "cheriot" "-target-feature" "+xcheri" "-target-feature" "-64bit" "-target-feature" "-relax" "-target-feature" "-xcheri-rvc" "-target-feature" "-save-restore" "-target-abi" "cheriot" "-Oz" "-Werror" "-cheri-compartment=example" -std=c2x | FileCheck %s
unsigned __builtin_strlen(const char *str) __asm__("_Z6strlenPKc");

long dynamic(const char *str) {
  // CHECK: @dynamic
  // CHECK: call cherilibcallcc i32 @_Z6strlenPKc
  return __builtin_strlen(str);
}

long five(const char *str) {
  // CHECK: @five
  // CHECK: ret i32 5
  return __builtin_strlen("hello");
}
