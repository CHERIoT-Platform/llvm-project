// RUN: %clang_cc1 -std=c++11 -triple riscv32cheriot -verify %s

struct test_struct {
    int b;
    int method();
};

int test1(test_struct * __sealed_capability a) {
    return a->b; // expected-error{{cannot access members of sealed type 'test_struct * __sealed_capability'}}
}

int test2(test_struct * __sealed_capability a) {
    return a->method(); // expected-error{{cannot access members of sealed type 'test_struct * __sealed_capability'}}
}

int test3(int& __sealed_capability a) { // expected-error{{reference type 'int & __sealed_capability' cannot be sealed}}
    return a; 
}

int test4(int * __sealed_capability a) {
    return *static_cast<int*>(a); // expected-error{{static_cast from 'int * __sealed_capability' to 'int *' changes sealed qualifier}}
}

int test5(int * __sealed_capability a) {
    return *reinterpret_cast<int*>(a); // expected-error{{cast from  'int * __sealed_capability' to 'int *' changes sealed qualifier}}
}

struct test_struct2 {
    int *a;
    test_struct2(int* __sealed_capability b) : a(b) { } // expected-error{{converting sealed type 'int * __sealed_capability' to non-sealed type 'int *' without an explicit unsealing}}
};


struct test_struct3 {
    int *a;
};

int test6(int * __sealed_capability b) {
    test_struct3 c{b}; // expected-error{{converting sealed type 'int * __sealed_capability' to non-sealed type 'int *' without an explicit unsealing}}
    return *(c.a);
}
