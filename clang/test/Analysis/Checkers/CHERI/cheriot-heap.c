// RUN: %clang_cc1 -triple riscv32e-none-cheriotrtos -target-feature +xcheriot -target-abi cheriot -cheri-compartment=test -analyze -verify %s \
// RUN:   -analyzer-checker=cheriot.CheriotHeap

int heap_claim(void* heapCapability, void* pointer);
int heap_free(void* heapCapability, void* ptr);
int heap_free_all(void* heapCapability);
int heap_claim_ephemeral(void* timeout, const void* ptr, const void* ptr2);
char check_pointer(const volatile void* ptr, int space,
                   unsigned int rawPermissions, char checkStackNeeded);
_Bool heap_address_is_valid(void *);

__attribute__((cheri_compartment("bar"))) void crossCompartmentCall(void);

__attribute__((cheri_compartment("test")))
void test_1(void* p) {
    heap_claim(0, p); // expected-warning{{Claim on pointer 'p' must be released with heap_free or heap_free_all before returning from a compartment call}}
}

__attribute__((cheri_compartment("test")))
void test_2(void* p) {
    heap_claim(0, p);
    heap_free(0, p); // no warn
}

__attribute__((cheri_compartment("test")))
void test_3(void* p) {
    heap_claim(0, p);
    heap_free_all(0); // no warn
}

__attribute__((cheri_compartment("test")))
void test_4(void* p1, void *p2) {
    heap_claim(0, p1);
    heap_claim(0, p2);
    heap_free(0, p1); // expected-warning{{Claim on pointer 'p2' must be released with heap_free or heap_free_all before returning from a compartment call}}
}

__attribute__((cheri_compartment("test")))
void test_5(int* p) {
    heap_claim(0, p);
    *p = 0;
    heap_free(0, p);
    *p = 1; // expected-warning{{Store through heap pointer 'p' without a valid claim}}
}

__attribute__((cheri_compartment("test")))
void test_6(int* p) {
    heap_claim(0, p);
    *p = 0;
    heap_free_all(0);
    *p = 1; // expected-warning{{Store through heap pointer 'p' without a valid claim}}
}

__attribute__((cheri_compartment("test")))
void test_7(int* p) {
    heap_claim_ephemeral(0, p, 0);
    *p = 0;
    heap_free(0, p);
    *p = 1; // expected-warning{{Store through heap pointer 'p' without a valid claim}}
}

__attribute__((cheri_compartment("test")))
void test_8(int* p) {
    heap_claim_ephemeral(0, p, 0);
    *p = 0;
    heap_free_all(0);
    *p = 1; // expected-warning{{Store through heap pointer 'p' without a valid claim}}
}

__attribute__((cheri_compartment("test")))
void test_9(int* p) {
    heap_claim_ephemeral(0, p, 0);
    *p = 0; // no warn
}

__attribute__((cheri_compartment("test")))
void test_10(int* p) {
    heap_claim_ephemeral(0, p, 0);
    *p = 0; // no warn
}

__attribute__((cheri_compartment("test")))
void test_11(int* p) {
    heap_claim_ephemeral(0, p, 0);
    crossCompartmentCall();
    *p = 0; // expected-warning{{Store through heap pointer 'p' after its ephemeral claim was released by a cross-compartment call}}
}

static void test_12_helper(int *p) { heap_claim(0, p); }
__attribute__((cheri_compartment("test")))
void test_12(int* p) {
    test_12_helper(p); // expected-warning{{Claim on pointer 'p' must be released with heap_free or heap_free_all before returning from a compartment call}}
}

static void test_13_helper(int *p) { heap_claim(0, p); }
__attribute__((cheri_compartment("test")))
void test_13(int* p) {
    test_13_helper(p);
    heap_free(0, p); // no warn
}

static void test_14_helper(int *p) { heap_claim(0, p); }
__attribute__((cheri_compartment("test")))
void test_14(int* p) {
    test_14_helper(p);
    heap_free_all(0); // no warn
}

__attribute__((cheri_compartment("test")))
void test_15(int* p) {
    check_pointer(p, 0, 0, 0); // expected-warning{{check_pointer called on potential heap pointer 'p' without a valid claim}}
}

__attribute__((cheri_compartment("test")))
void test_16(int* p) {
    int i = 0;
    check_pointer(&i, 0, 0, 0); // no warn
}

int test_17_global = 1;
__attribute__((cheri_compartment("test")))
void test_17(int* p) {
    check_pointer(&test_17_global, 0, 0, 0); // no warn
}

static const int test_18_cst = 1;
__attribute__((cheri_compartment("test")))
void test_18(int* p) {
    check_pointer(&test_18_cst, 0, 0, 0); // no warn
}

__attribute__((cheri_compartment("test")))
void test_20(int* p) {
    if (!heap_address_is_valid(p))
        check_pointer(p, 0, 0, 0); // no warn
}

__attribute__((cheri_compartment("test")))
void test_21(int* p) {
    if (heap_address_is_valid(p))
        check_pointer(p, 0, 0, 0); // expected-warning{{check_pointer called on potential heap pointer 'p' without a valid claim}}
}

__attribute__((cheri_compartment("test")))
void test_22(int* p) {
    *p = 1; // expected-warning{{Store through heap pointer 'p' without a valid claim}}
    heap_claim(0, p);
    *p = 0;
    heap_free(0, p);
}

__attribute__((cheri_compartment("test")))
void test_23(int* p) {
    if (!heap_address_is_valid(p))
        *p = 1; // no warn
    heap_claim(0, p);
    *p = 0;
    heap_free(0, p);
}

__attribute__((cheri_compartment("test")))
void test_24(int* p) {
    if (!heap_address_is_valid(p))
        *p = 1; // no warn
    else
        heap_claim(0, p); // no warn
    check_pointer(p, 0, 0, 0);
    *p = 0;
    heap_free(0, p);
}

__attribute__((cheri_compartment("test")))
void test_25(int* p) {
    if (!heap_address_is_valid(p))
        *p = 1; // no warn
    else
        heap_claim(0, p); // no warn
    check_pointer(p, 0, 0, 0);
    *p = 0; // expected-warning{{Claim on pointer 'p' must be released with heap_free or heap_free_all before returning from a compartment call}}
}

__attribute__((cheri_compartment("test")))
void test_26(int* p) {
    if (!heap_address_is_valid(p))
        *p = 1; // no warn
    else
        heap_claim(0, p); // no warn
    check_pointer(p, 0, 0, 0);
    *p = 0;
    if (heap_address_is_valid(p))
        heap_free(0, p); // no warn
}

struct test27_struct { int i; };
__attribute__((cheri_compartment("test")))
int test_27(struct test27_struct* p) {
    return p->i; // expected-warning{{Read of heap pointer 'p' without a valid claim}}
}

__attribute__((cheri_compartment("test")))
int test_228(int* p) {
    return p[1]; // expected-warning{{Read of heap pointer 'p' without a valid claim}}
}
