// RUN: %clang_cc1 -std=c++20 -triple riscv32-none-cheriotrtos -target-feature +xcheriot -target-abi cheriot -cheri-compartment=test -analyze -verify %s \
// RUN:   -analyzer-checker=cheriot.CheriotHeap

namespace CHERI {

static constexpr unsigned PermissionStore = 1 << 2;
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
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionStore}>(p, sizeof(int));
    return *p; // expected-warning {{Load through heap pointer 'p' without passing the appropriate permission (LD) to check_pointer. Runtime behavior will depend on the permissions provided by the caller. Use the EnforceStrictPermissions template parameter to check_pointer to enforce consistency across callers}}
}

__attribute__((cheri_compartment("test")))
int test_2(int* p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionStore}, true, true>(p, sizeof(int));
    return *p; // expected-warning {{Load through heap pointer 'p' without passing the appropriate permission (LD) to check_pointer}}
}

__attribute__((cheri_compartment("test")))
void test_3(int* p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}>(p, sizeof(int));
    *p = 1; // expected-warning {{Store through heap pointer 'p' without passing the appropriate permission (SD) to check_pointer. Runtime behavior will depend on the permissions provided by the caller. Use the EnforceStrictPermissions template parameter to check_pointer to enforce consistency across callers}}
}

__attribute__((cheri_compartment("test")))
void test_4(int* p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}, true, true>(p, sizeof(int));
    *p = 1; // expected-warning {{Store through heap pointer 'p' without passing the appropriate permission (SD) to check_pointer}}
}

__attribute__((cheri_compartment("test")))
void test_5(int** p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}, true, true>(p, sizeof(int*));
    int *q = *p;
    *q = 0; // expected-warning {{Store through pointer which may be an invalid capability because MC permission was not checked before it was loaded}}
}

__attribute__((cheri_compartment("test")))
void test_6(int** p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad|PermissionLoadStoreCap}, true, true>(p, sizeof(int*));
    int *q = *p;
    *q = 0;
}

__attribute__((cheri_compartment("test")))
int test_7(int** p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad}, true, true>(p, sizeof(int*));
    int *q = *p;
    return *q; // expected-warning {{Load through pointer which may be an invalid capability because MC permission was not checked before it was loaded}}
}

__attribute__((cheri_compartment("test")))
int test_8(int** p) {
    unknown_call();
    heap_claim_ephemeral(nullptr, p, nullptr);
    check_pointer<PermissionSet{PermissionLoad|PermissionLoadStoreCap}, true, true>(p, sizeof(int*));
    int *q = *p;
    return *q;
}
