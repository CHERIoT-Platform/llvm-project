// RUN: %clang_cc1 -triple riscv32cheriot -target-abi cheriot-baremetal -verify %s

int * __sealed_capability test1(int * __sealed_capability a) {
    return a; // no error
}

int test2(int * __sealed_capability a) {
    return *a; // expected-error{{invalid operator applied to sealed type 'int * __sealed_capability'}}
}

int test3(int * __sealed_capability a) {
    return a[0]; // expected-error{{invalid operator applied to sealed type 'int * __sealed_capability'}}
}

int * __sealed_capability test4(int * __sealed_capability a) {
    return a + 1; // expected-error{{invalid operands to binary expression ('int * __sealed_capability' and 'int'}}
}

typedef struct { int b; } test_struct;

int test5(test_struct * __sealed_capability a) {
    return a->b; // expected-error{{cannot access members of sealed type 'test_struct * __sealed_capability'}}
}

int test6(int (* __sealed_capability myfun)()) {
    return myfun(); // expected-error{{sealed type 'int (* __sealed_capability)(void)' cannot be called}}
}

int test7(int * __sealed_capability a) {
    int *b = a; // expected-error{{converting sealed type 'int * __sealed_capability' to non-sealed type 'int *' without an explicit unsealing}}
    return *b;
}

int test8(int * __sealed_capability a) {
    int *b = (int*)a; // no error (*explicit* cast allowed)
    return *b;
}

extern int deref(int *);
extern void opaque(void*);

int test9(int * __sealed_capability a) {
    return deref(a); // expected-error{{converting sealed type 'int * __sealed_capability' to non-sealed type 'int *' without an explicit unsealing}}
}

extern int test9(int * __sealed_capability);
int test10(int *a) {
    return test9(a); // expected-error{{converting unsealed type 'int *' to sealed type 'int * __sealed_capability' without an explicit sealing}}
}

void test11(int * __sealed_capability x) {
	opaque(x); // No error (cast to void permitted)
}

int test12(int * __sealed_capability a) {
    return sizeof(*a);
}
