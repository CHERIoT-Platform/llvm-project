// RUN: %clang_cc1 -std=c++20 -triple riscv32-none-cheriotrtos -target-feature +xcheriot -target-abi cheriot -cheri-compartment=test -analyze -analyzer-output=text -verify %s \
// RUN:   -analyzer-checker=core.builtin.AssumeModeling,cheriot.CheriotHeap

namespace CHERI {

static constexpr unsigned PermissionGlobal = 1 << 0;
static constexpr unsigned PermissionLoadGlobal = 1 << 1;
static constexpr unsigned PermissionStore = 1 << 2;
static constexpr unsigned PermissionLoadMutable = 1 << 3;
static constexpr unsigned PermissionLoadStoreLocal = 1 << 4;
static constexpr unsigned PermissionLoad = 1 << 5;
static constexpr unsigned PermissionLoadStoreCap = 1 << 6;

struct PermissionSet {
    unsigned rawPermissions;
    constexpr PermissionSet(unsigned rawPermissions)
        : rawPermissions(rawPermissions) { }
};

template<PermissionSet Permissions = PermissionSet{PermissionLoad},
            bool          CheckStack  = true,
            bool          EnforceStrictPermissions = false>
inline bool check_pointer(auto  &ptr, int space) { return true; }
}

using namespace CHERI;

int heap_claim_ephemeral(void* timeout, const void* ptr, const void* ptr2);

extern void unknown_call();

__attribute__((cheri_compartment("test")))
int test_1(int* p) {
    unknown_call(); // #t1_call
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}>(p, 2 * sizeof(int)); // #t1_check
    int a = p[0] + p[1]; // no warning
    return a + p[2]; // expected-warning{{Load through pointer 'p' may access memory outside the 8 bytes checked by check_pointer. Runtime behavior will depend on the bounds provided by the caller. Use the EnforceStrictPermissions template parameter to check_pointer to enforce consistency across callers}}
    // expected-note@-1 {{Load through pointer 'p' may access memory outside the 8 bytes checked by check_pointer}}
    // expected-note@#t1_call {{Externally visible global state potentially mutated by external call here}}
    // expected-note@#t1_check {{check_pointer checked that 'p' has at least 8 accessible bytes here}}
}

__attribute__((cheri_compartment("test")))
int test_2(int* p) {
    unknown_call(); // #t2_call
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}, true, true>(p, sizeof(int)); // #t2_check
    return p[1]; // expected-warning-re{{Load through pointer 'p' may access memory outside the 4 bytes checked by check_pointer{{( \[.*\])?$}}}}
    // expected-note@-1 {{Load through pointer 'p' may access memory outside the 4 bytes checked by check_pointer}}
    // expected-note@#t2_call {{Externally visible global state potentially mutated by external call here}}
    // expected-note@#t2_check {{check_pointer checked that 'p' has at least 4 accessible bytes and set its bounds to exactly 4 bytes here}}
}

__attribute__((cheri_compartment("test")))
int test_3(int* p) {
    unknown_call(); // #t3_call
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}>(p, sizeof(int)); // #t3_check
    return p[-1]; // expected-warning{{Load through pointer 'p' may access memory outside the 4 bytes checked by check_pointer}}
    // expected-note@-1 {{Load through pointer 'p' may access memory outside the 4 bytes checked by check_pointer}}
    // expected-note@#t3_call {{Externally visible global state potentially mutated by external call here}}
    // expected-note@#t3_check {{check_pointer checked that 'p' has at least 4 accessible bytes here}}
}

struct test4_pair { int a; int b; };

__attribute__((cheri_compartment("test")))
void test_4(test4_pair* p) {
    unknown_call(); // #t4_call
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionStore}>(p, sizeof(int)); // #t4_check
    p->a = 1; // no warning
    p->b = 2; // expected-warning{{Store through pointer 'p' may access memory outside the 4 bytes checked by check_pointer}}
    // expected-note@-1 {{Store through pointer 'p' may access memory outside the 4 bytes checked by check_pointer}}
    // expected-note@#t4_call {{Externally visible global state potentially mutated by external call here}}
    // expected-note@#t4_check {{check_pointer checked that 'p' has at least 4 accessible bytes here}}
}

__attribute__((cheri_compartment("test")))
int test_5(int* p, int n) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}>(p, n);
    return p[1000]; // no warning
}

__attribute__((cheri_compartment("test")))
int test_6(int* p, int n) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}>(p, sizeof(int));
    check_pointer<PermissionSet{PermissionLoad}>(p, n);
    return p[1]; // no warning
}

__attribute__((cheri_compartment("test")))
int test_7(int* p, int n) {
    unknown_call(); // #t7_call
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}, true, true>(p, sizeof(int)); // #t7_check
    check_pointer<PermissionSet{PermissionLoad}>(p, n);
    return p[1]; // expected-warning-re{{Load through pointer 'p' may access memory outside the 4 bytes checked by check_pointer{{( \[.*\])?$}}}}
    // expected-note@-1 {{Load through pointer 'p' may access memory outside the 4 bytes checked by check_pointer}}
    // expected-note@#t7_call {{Externally visible global state potentially mutated by external call here}}
    // expected-note@#t7_check {{check_pointer checked that 'p' has at least 4 accessible bytes and set its bounds to exactly 4 bytes here}}
}

__attribute__((cheri_compartment("test")))
int test_8(int* p) {
    unknown_call(); // #t8_call
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}>(p, 2 * sizeof(int)); // #t8_check
    check_pointer<PermissionSet{PermissionLoad}>(p, sizeof(int));
    int a = p[1]; // no warning
    return a + p[2]; // expected-warning{{Load through pointer 'p' may access memory outside the 8 bytes checked by check_pointer}}
    // expected-note@-1 {{Load through pointer 'p' may access memory outside the 8 bytes checked by check_pointer}}
    // expected-note@#t8_call {{Externally visible global state potentially mutated by external call here}}
    // expected-note@#t8_check {{check_pointer checked that 'p' has at least 8 accessible bytes here}}
}

__attribute__((cheri_compartment("test")))
int test_9(int* p) {
    unknown_call(); // #t9_call
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}>(p, 2 * sizeof(int));
    check_pointer<PermissionSet{PermissionLoad}, true, true>(p, sizeof(int)); // #t9_check
    return p[1]; // expected-warning-re{{Load through pointer 'p' may access memory outside the 4 bytes checked by check_pointer{{( \[.*\])?$}}}}
    // expected-note@-1 {{Load through pointer 'p' may access memory outside the 4 bytes checked by check_pointer}}
    // expected-note@#t9_call {{Externally visible global state potentially mutated by external call here}}
    // expected-note@#t9_check {{check_pointer checked that 'p' has at least 4 accessible bytes and set its bounds to exactly 4 bytes here}}
}

__attribute__((cheri_compartment("test")))
int test_10(int* p) {
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}>(p, sizeof(int));
    return p[1]; // no warning
}

__attribute__((cheri_compartment("test")))
int test_11(int* p, int i) {
    unknown_call(); // #t11_call
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}>(p, sizeof(int)); // #t11_check
    int a = p[i]; // no warning
    __builtin_assume(i == 1); // #t11_assume
    return a + p[i]; // expected-warning{{Load through pointer 'p' may access memory outside the 4 bytes checked by check_pointer. Runtime behavior will depend on the bounds provided by the caller. Use the EnforceStrictPermissions template parameter to check_pointer to enforce consistency across callers}}
    // expected-note@-1 {{Load through pointer 'p' may access memory outside the 4 bytes checked by check_pointer}}
    // expected-note@#t11_call {{Externally visible global state potentially mutated by external call here}}
    // expected-note@#t11_check {{check_pointer checked that 'p' has at least 4 accessible bytes here}}
    // expected-note@#t11_assume {{Assuming 'i' is equal to 1}}
}

__attribute__((cheri_compartment("test")))
int test_12(int* p) {
    unknown_call(); // #t12_call
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}, true, true>(p, sizeof(int)); // #t12_check
    check_pointer<PermissionSet{PermissionLoad}, true, true>(p, 2 * sizeof(int)); // expected-warning{{check_pointer called multiple times on pointer 'p' with a larger space than a prior call set its bounds to (8 > 4 bytes)}}
    // expected-note@-1 {{check_pointer called multiple times on pointer 'p' with a larger space than a prior call set its bounds to (8 > 4 bytes)}}
    // The bounds can't grow, so the first check's 4 bytes still apply.
    return p[1]; // expected-warning-re{{Load through pointer 'p' may access memory outside the 4 bytes checked by check_pointer{{( \[.*\])?$}}}}
    // expected-note@-1 {{Load through pointer 'p' may access memory outside the 4 bytes checked by check_pointer}}
    // expected-note@#t12_call {{Externally visible global state potentially mutated by external call here}}
    // expected-note@#t12_check 2 {{check_pointer checked that 'p' has at least 4 accessible bytes and set its bounds to exactly 4 bytes here}}
}

__attribute__((cheri_compartment("test")))
int test_13(int* p) {
    unknown_call(); // #t13_call
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}, true, true>(p, sizeof(int)); // #t13_check
    check_pointer<PermissionSet{PermissionLoad}>(p, 2 * sizeof(int)); // expected-warning{{check_pointer called multiple times on pointer 'p' with a larger space than a prior call set its bounds to (8 > 4 bytes)}}
    // expected-note@-1 {{check_pointer called multiple times on pointer 'p' with a larger space than a prior call set its bounds to (8 > 4 bytes)}}
    // The strict check's bounds still apply.
    return p[1]; // expected-warning-re{{Load through pointer 'p' may access memory outside the 4 bytes checked by check_pointer{{( \[.*\])?$}}}}
    // expected-note@-1 {{Load through pointer 'p' may access memory outside the 4 bytes checked by check_pointer}}
    // expected-note@#t13_call {{Externally visible global state potentially mutated by external call here}}
    // expected-note@#t13_check 2 {{check_pointer checked that 'p' has at least 4 accessible bytes and set its bounds to exactly 4 bytes here}}
}

__attribute__((cheri_compartment("test")))
int test_14(int* p) {
    unknown_call(); // #t14_call
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}, true, true>(p, 2 * sizeof(int));
    check_pointer<PermissionSet{PermissionLoad}, true, true>(p, sizeof(int)); // #t14_check
    // The narrower bounds apply.
    return p[1]; // expected-warning-re{{Load through pointer 'p' may access memory outside the 4 bytes checked by check_pointer{{( \[.*\])?$}}}}
    // expected-note@-1 {{Load through pointer 'p' may access memory outside the 4 bytes checked by check_pointer}}
    // expected-note@#t14_call {{Externally visible global state potentially mutated by external call here}}
    // expected-note@#t14_check {{check_pointer checked that 'p' has at least 4 accessible bytes and set its bounds to exactly 4 bytes here}}
}

__attribute__((cheri_compartment("test")))
int test_15(int* p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}>(p, sizeof(int));
    check_pointer<PermissionSet{PermissionLoad}, true, true>(p, 2 * sizeof(int)); // no warning
    return p[1]; // no warning
}

__attribute__((cheri_compartment("test")))
int test_16(int* p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}>(p, sizeof(int));
    return *p; // no warning
}

__attribute__((cheri_compartment("test")))
int test_17(int* p) {
    unknown_call(); // #t17_call
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}>(p, sizeof(short)); // #t17_check
    return *p; // expected-warning{{Load through pointer 'p' may access memory outside the 2 bytes checked by check_pointer. Runtime behavior will depend on the bounds provided by the caller. Use the EnforceStrictPermissions template parameter to check_pointer to enforce consistency across callers}}
    // expected-note@-1 {{Load through pointer 'p' may access memory outside the 2 bytes checked by check_pointer}}
    // expected-note@#t17_call {{Externally visible global state potentially mutated by external call here}}
    // expected-note@#t17_check {{check_pointer checked that 'p' has at least 2 accessible bytes here}}
}

__attribute__((cheri_compartment("test")))
long long test_18(int* p) {
    unknown_call(); // #t18_call
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}>(p, sizeof(int)); // #t18_check
    return *(long long*)p; // expected-warning{{Load through pointer 'p' may access memory outside the 4 bytes checked by check_pointer}}
    // expected-note@-1 {{Load through pointer 'p' may access memory outside the 4 bytes checked by check_pointer}}
    // expected-note@#t18_call {{Externally visible global state potentially mutated by external call here}}
    // expected-note@#t18_check {{check_pointer checked that 'p' has at least 4 accessible bytes here}}
}

__attribute__((cheri_compartment("test")))
int test_19(int* p) {
    unknown_call(); // #t19_call
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}>(p, 2 * sizeof(int)); // #t19_check
    int *q = p + 1;
    int a = q[0]; // no warning
    return a + q[1]; // expected-warning{{Load through pointer 'q' may access memory outside the 8 bytes checked by check_pointer}}
    // expected-note@-1 {{Load through pointer 'q' may access memory outside the 8 bytes checked by check_pointer}}
    // expected-note@#t19_call {{Externally visible global state potentially mutated by external call here}}
    // expected-note@#t19_check {{check_pointer checked that 'p' has at least 8 accessible bytes here}}
}

__attribute__((cheri_compartment("test")))
int test_20(int* p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}>(p, sizeof(int));
    check_pointer<PermissionSet{PermissionLoad}>(p, 2 * sizeof(int));
    return p[1]; // no warning
}

__attribute__((cheri_compartment("test")))
int test_21(int* p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}>(p, 0);
    return *p; // no warning
}

__attribute__((cheri_compartment("test")))
int test_22(int* p) {
    unknown_call(); // #t22_call
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}, true, true>(p, 0); // #t22_check
    return *p; // expected-warning-re{{Load through pointer 'p' may access memory outside the 0 bytes checked by check_pointer{{( \[.*\])?$}}}}
    // expected-note@-1 {{Load through pointer 'p' may access memory outside the 0 bytes checked by check_pointer}}
    // expected-note@#t22_call {{Externally visible global state potentially mutated by external call here}}
    // expected-note@#t22_check {{check_pointer checked that 'p' has at least 0 accessible bytes and set its bounds to exactly 0 bytes here}}
}
