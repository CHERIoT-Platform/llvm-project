// RUN: %clang_cc1 -std=c++20 -triple riscv32-none-cheriotrtos -target-feature +xcheriot -target-abi cheriot -cheri-compartment=test -analyze -verify %s \
// RUN:   -analyzer-checker=cheriot.CheriotHeap

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
extern __attribute__((cheri_compartment("other"))) void other_compartment_call();

__attribute__((cheri_compartment("test")))
int test_1(int* p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionStore}>(p, sizeof(int));
    return *p; // expected-warning{{Load through pointer 'p' with missing LD permission. Runtime behavior will depend on the permissions provided by the caller. Use the EnforceStrictPermissions template parameter to check_pointer to enforce consistency across callers}}
}

__attribute__((cheri_compartment("test")))
int test_2(int* p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionStore}, true, true>(p, sizeof(int));
    return *p; // expected-warning {{Load through pointer 'p' with missing LD permission}}
}

__attribute__((cheri_compartment("test")))
void test_3(int* p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}>(p, sizeof(int));
    *p = 1;  // expected-warning{{Store through pointer 'p' with missing SD permission. Runtime behavior will depend on the permissions provided by the caller. Use the EnforceStrictPermissions template parameter to check_pointer to enforce consistency across callers}}
}

__attribute__((cheri_compartment("test")))
void test_4(int* p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}, true, true>(p, sizeof(int));
    *p = 1; // expected-warning {{Store through pointer 'p' with missing SD permission}}
}

__attribute__((cheri_compartment("test")))
void test_5(int** p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}, true, true>(p, sizeof(int*));
    int *q = *p;
    *q = 0;
    // expected-warning@-1 {{Store through pointer 'q' with missing SD permission}}
    // expected-warning@-2 {{Store through pointer 'q' which may be an invalid capability because MC permission was not checked before the pointer was loaded}}
}

__attribute__((cheri_compartment("test")))
void test_6(int** p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad|PermissionLoadStoreCap|PermissionLoadMutable}, true, true>(p, sizeof(int*));
    int *q = *p;
    *q = 0;
}

__attribute__((cheri_compartment("test")))
int test_7(int** p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}, true, true>(p, sizeof(int*));
    int *q = *p;
    return *q; // expected-warning {{Load through pointer 'q' which may be an invalid capability because MC permission was not checked before the pointer was loaded}}
}

__attribute__((cheri_compartment("test")))
int test_8(int** p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad|PermissionLoadStoreCap}, true, true>(p, sizeof(int*));
    int *q = *p;
    return *q;
}

__attribute__((cheri_compartment("test")))
void test_9(int** p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad|PermissionLoadStoreCap}, true, true>(p, sizeof(int*));
    int *q = *p;
    *q = 0; // expected-warning{{Store through pointer 'q' with missing SD permission}}
}

__attribute__((cheri_compartment("test")))
void test_10(int*** p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad|PermissionLoadStoreCap}, true, true>(p, sizeof(int*));
    int **q = *p;
    int *r = *q;
    *r = 0; // expected-warning{{Store through pointer 'r' with missing SD permission}}
}

__attribute__((cheri_compartment("test")))
void test_11(int*** p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad|PermissionLoadStoreCap|PermissionLoadMutable}, true, true>(p, sizeof(int*));
    int **q = *p;
    int *r = *q;
    *r = 0;
}

__attribute__((cheri_compartment("test")))
void test_12(int*** p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}, true, true>(p, sizeof(int*));
    check_pointer<PermissionSet{PermissionStore|PermissionLoadMutable}, true, true>(p, sizeof(int*)); // expected-warning{{check_pointer called multiple times on pointer 'p' with required permissions that were removed by a prior call (SD|LM)}}
}

__attribute__((cheri_compartment("test")))
int** test_13(int*** p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad|PermissionStore}, true, true>(p, sizeof(int*));
    check_pointer<PermissionSet{PermissionStore}, true, true>(p, sizeof(int*));
    return *p; // expected-warning{{Load through pointer 'p' with missing LD permission}}
}

__attribute__((cheri_compartment("test")))
void test_14(int*** p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}, true, false>(p, sizeof(int*));
    check_pointer<PermissionSet{PermissionStore|PermissionLoadMutable|PermissionLoadStoreCap}, true, false>(p, sizeof(int*));
    ***p = 0;
    **p = 0;
    *p = 0;
}

__attribute__((cheri_compartment("test")))
void test_15(int* p, int** q) {
    unknown_call();
    heap_claim_ephemeral(nullptr, q, nullptr);
    check_pointer<PermissionSet{0}, true, true>(p, sizeof(int*));
    check_pointer<PermissionSet{PermissionStore|PermissionLoadStoreCap}, true, true>(q, sizeof(int**));
    *q = p; // expected-warning{{Store of locally-scoped capability 'p' (lacking GL permission) through a globally-scoped pointer 'q' (lacking SL permission). The stored capability will be untagged}}
}

__attribute__((cheri_compartment("test")))
void test_16(int** p, int** q) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, q);
    check_pointer<PermissionSet{PermissionLoad|PermissionLoadStoreCap}, true, true>(p, sizeof(int*));
    check_pointer<PermissionSet{PermissionStore|PermissionLoadStoreCap}, true, true>(q, sizeof(int**));
    int *r = *p;
    *q = r; // expected-warning{{Store of locally-scoped capability (lacking GL permission) through a globally-scoped pointer 'q' (lacking SL permission). The stored capability will be untagged}}
}

__attribute__((cheri_compartment("test")))
void test_17(int** p, int** q) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, q);
    check_pointer<PermissionSet{PermissionLoad|PermissionLoadStoreCap|PermissionLoadGlobal}, true, true>(p, sizeof(int*));
    check_pointer<PermissionSet{PermissionStore|PermissionLoadStoreCap}, true, true>(q, sizeof(int**));
    int *r = *p;
    *q = r;
}

__attribute__((cheri_compartment("test")))
int test_18(int** p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad|PermissionLoadStoreCap|PermissionLoadMutable|PermissionLoadGlobal}, true, true>(p, sizeof(int*));
    int *q = *p;
    check_pointer<PermissionSet{PermissionLoadStoreCap}>(q, sizeof(int));
    int *r = *p;
    return *r; // no warning
}

__attribute__((cheri_compartment("test")))
int test_19(int** p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}, true, true>(p, sizeof(int*));
    int *q = *p;
    check_pointer<PermissionSet{PermissionLoad}>(q, sizeof(int));
    int a = *q; // no warning
    int *r = *p;
    return a + *r; // expected-warning{{Load through pointer 'r' which may be an invalid capability because MC permission was not checked before the pointer was loaded}}
}

__attribute__((cheri_compartment("test")))
int test_20(int* p) {
    unknown_call();
    int *q = p;
    check_pointer<PermissionSet{PermissionLoad}>(p, sizeof(int));
    heap_claim_ephemeral(nullptr, p, nullptr);
    int a = *q; // no warning
    other_compartment_call();
    int b = *q;  // expected-warning{{Read of heap pointer 'q' after its ephemeral claim was released by a cross-compartment call}}
    return b;
}

int test_21(int* p) {
    check_pointer<PermissionSet{PermissionStore}, true, true>(p, sizeof(int));
    return *p; // no warning
}
