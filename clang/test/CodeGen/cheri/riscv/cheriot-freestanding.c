// RUN: %clang_cc1 %s -o - "-triple" "riscv32cheriot-unknown-unknown" "-emit-llvm" "-mframe-pointer=none" "-mcmodel=small" "-target-abi" "cheriot" "-Oz" "-Werror" -std=c2x | FileCheck %s


// CHECK: @memcpy
void* __attribute__((cheri_libcall)) memcpy(void*, const void*, unsigned int) {
    return 0;
}

// CHECK: @memmove
void* __attribute__((cheri_libcall)) memmove(void*, const void*, unsigned int) {
    return 0;
}

// CHECK: @memset
void* __attribute__((cheri_libcall)) memset(void*, int, unsigned int) {
    return 0;
}

// CHECK: @memcmp
int __attribute__((cheri_libcall)) memcmp(const void*, const void*, unsigned int) {
    return 0;
}
