// RUN: %clang_cc1 %s -o - "-triple" "riscv32cheriot-unknown-unknown" "-emit-llvm" "-mframe-pointer=none" "-mcmodel=small" "-target-abi" "cheriot" "-Oz" "-Werror" -std=c2x | FileCheck %s
// Verify that setjmp is called without cherilibcallcc
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
// CHECK: call i32 @setjmp
void test() {
    setjmp(buf);
}