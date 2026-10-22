// RUN: %clang_cc1 -triple riscv32-none-cheriotrtos -target-feature +xcheriot -target-abi cheriot -cheri-compartment=test -analyze -verify %s \
// RUN:   -analyzer-checker=core,cheriot.CheriotHeap

// The CHERIoT heap checker only analyzes paths starting at compartment entry
// points with pointer arguments, but must not stop other checkers from
// analyzing everything else.

int heap_claim(void* heapCapability, void* pointer);

// Not a compartment entry point.
int not_entry_point(void) {
    int *p = 0;
    return *p; // expected-warning{{Dereference of null pointer}}
}

// A compartment entry point without pointer arguments.
__attribute__((cheri_compartment("test")))
int entry_point_without_pointers(int x) {
    int *p = 0;
    return *p + x; // expected-warning{{Dereference of null pointer}}
}

// A compartment entry point with pointer arguments: both checkers report.
__attribute__((cheri_compartment("test")))
int entry_point_with_pointer(int *q) {
    heap_claim(0, q);
    int *p = 0;
    return *p; // expected-warning{{Dereference of null pointer}}
}

__attribute__((cheri_compartment("test")))
void entry_point_leak(void *q) {
    heap_claim(0, q); // expected-warning{{Claim on pointer 'q' must be released with heap_free or heap_free_all before returning from a compartment call}}
}
