// RUN: %clang_cc1 %s -o - "-triple" "riscv32-unknown-cheriotrtos" "-emit-llvm" "-mframe-pointer=none" "-mcmodel=small" "-target-abi" "cheriot" "-Oz" "-Werror" "-target-feature" "+xcheriot" -std=c2x | FileCheck %s
// Verify that setjmp is called without cheriot_librarycallcc
struct __jmp_buf
{
	unsigned __cs0;
	unsigned __cs1;
	unsigned __csp;
	unsigned __cra;
};

typedef struct __jmp_buf jmp_buf[1];

__attribute__((returns_twice)) int setjmp(jmp_buf env) __asm__("setjmp");

jmp_buf buf;

// CHECK-LABEL: @test
// CHECK: call addrspace(200) i32 @setjmp
void test() {
    setjmp(buf);
}