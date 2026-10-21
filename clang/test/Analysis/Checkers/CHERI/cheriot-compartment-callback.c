// RUN: %clang_cc1 -triple riscv32-none-cheriotrtos -target-feature +xcheriot -target-abi cheriot -cheri-compartment=test \
// RUN:   -Wno-cheriot-compartment-fn-ptrs -analyze -verify %s \
// RUN:   -analyzer-checker=cheriot.CheriotCompartmentCallback

typedef void (*FnPtr)(void);
typedef int (*IntFnPtr)(int);
typedef __attribute__((cheriot_ccallback)) void (*Callback)(void);

void local_function(void);
void take_ptr(void *);

__attribute__((cheri_compartment("bar"))) void bar_entry(FnPtr f);

struct Holder {
  int other;
  FnPtr fn;
};

FnPtr global_fn;

// Positive tests

__attribute__((cheri_compartment("test")))
void test_direct(FnPtr f) { // #direct
  f(); // expected-warning{{Calling function pointer that was passed into compartment entry point 'test_direct' as parameter 'f' without the cheriot_ccallback attribute}}
  // expected-note@#direct {{Function pointer received from another compartment here}}
}

__attribute__((cheri_compartment("test")))
void test_explicit_deref(FnPtr f) { // #deref
  (*f)(); // expected-warning{{Calling function pointer that was passed into compartment entry point 'test_explicit_deref' as parameter 'f' without the cheriot_ccallback attribute}}
  // expected-note@#deref {{Function pointer received from another compartment here}}
}

__attribute__((cheri_compartment("test")))
int test_with_args_and_result(IntFnPtr f, int x) { // #args
  return f(x) + 1; // expected-warning{{Calling function pointer that was passed into compartment entry point 'test_with_args_and_result' as parameter 'f' without the cheriot_ccallback attribute}}
  // expected-note@#args {{Function pointer received from another compartment here}}
}

__attribute__((cheri_compartment("test")))
void test_no_typedef(void (*f)(void)) { // #rawparam
  f(); // expected-warning{{Calling function pointer that was passed into compartment entry point 'test_no_typedef' as parameter 'f' without the cheriot_ccallback attribute}}
  // expected-note@#rawparam {{Function pointer received from another compartment here}}
}

__attribute__((cheri_compartment("test")))
void test_local_copy(FnPtr f) { // #copy
  FnPtr g = f;
  g(); // expected-warning{{Calling function pointer that was passed into compartment entry point 'test_local_copy' as parameter 'f' without the cheriot_ccallback attribute}}
  // expected-note@#copy {{Function pointer received from another compartment here}}
}

__attribute__((cheri_compartment("test")))
void test_void_ptr_round_trip(FnPtr f) { // #voidptr
  void *p = (void *)f;
  FnPtr g = (FnPtr)p;
  g(); // expected-warning{{Calling function pointer that was passed into compartment entry point 'test_void_ptr_round_trip' as parameter 'f' without the cheriot_ccallback attribute}}
  // expected-note@#voidptr {{Function pointer received from another compartment here}}
}

__attribute__((cheri_compartment("test")))
void test_struct_field(FnPtr f) { // #field
  struct Holder h;
  h.other = 0;
  h.fn = f;
  h.fn(); // expected-warning{{Calling function pointer that was passed into compartment entry point 'test_struct_field' as parameter 'f' without the cheriot_ccallback attribute}}
  // expected-note@#field {{Function pointer received from another compartment here}}
}

__attribute__((cheri_compartment("test")))
void test_global(FnPtr f) { // #global
  global_fn = f;
  global_fn(); // expected-warning{{Calling function pointer that was passed into compartment entry point 'test_global' as parameter 'f' without the cheriot_ccallback attribute}}
  // expected-note@#global {{Function pointer received from another compartment here}}
}

static void call_it(FnPtr cb) {
  cb(); // expected-warning{{Calling function pointer that was passed into compartment entry point 'test_via_helper' as parameter 'f' without the cheriot_ccallback attribute}}
}
__attribute__((cheri_compartment("test")))
void test_via_helper(FnPtr f) { // #helper
  call_it(f);
  // expected-note@#helper {{Function pointer received from another compartment here}}
}

__attribute__((cheri_compartment("test")))
void test_mixed(Callback good, FnPtr bad) { // #mixed
  good();
  bad(); // expected-warning{{Calling function pointer that was passed into compartment entry point 'test_mixed' as parameter 'bad' without the cheriot_ccallback attribute}}
  // expected-note@#mixed {{Function pointer received from another compartment here}}
}

__attribute__((cheri_compartment("test")))
void test_conditional(FnPtr f, int x) { // #cond
  FnPtr g = x ? f : local_function;
  g(); // expected-warning{{Calling function pointer that was passed into compartment entry point 'test_conditional' as parameter 'f' without the cheriot_ccallback attribute}}
  // expected-note@#cond {{Function pointer received from another compartment here}}
}

__attribute__((cheri_compartment("test")))
void test_two_calls(FnPtr f) { // #twice
  f(); // expected-warning{{Calling function pointer that was passed into compartment entry point 'test_two_calls' as parameter 'f' without the cheriot_ccallback attribute}}
  f(); // expected-warning{{Calling function pointer that was passed into compartment entry point 'test_two_calls' as parameter 'f' without the cheriot_ccallback attribute}}
  // expected-note@#twice 2 {{Function pointer received from another compartment here}}
}

__attribute__((cheri_compartment("test"))) void test_attr_on_prototype(FnPtr f);
void test_attr_on_prototype(FnPtr f) { // #proto
  f(); // expected-warning{{Calling function pointer that was passed into compartment entry point 'test_attr_on_prototype' as parameter 'f' without the cheriot_ccallback attribute}}
  // expected-note@#proto {{Function pointer received from another compartment here}}
}

__attribute__((cheri_compartment("test")))
void test_entry_also_called_locally(FnPtr f) { // expected-note{{Function pointer received from another compartment here}}
  f(); // expected-warning{{Calling function pointer that was passed into compartment entry point 'test_entry_also_called_locally' as parameter 'f' without the cheriot_ccallback attribute}}
}

void same_compartment_caller(void) {
  test_entry_also_called_locally(local_function);
}

// Negative tests

__attribute__((cheri_compartment("test")))
void test_callback_param(Callback cb) {
  cb(); // no-warning
}

void test_not_entry_point(FnPtr f) {
  f(); // no-warning
}

__attribute__((cheri_compartment("test")))
void test_reassigned(FnPtr f) {
  f = local_function;
  f(); // no-warning
}

__attribute__((cheri_compartment("test")))
void test_pass_along(FnPtr f) {
  bar_entry(f); // no-warning
}

__attribute__((cheri_compartment("test")))
void test_escaped(FnPtr f) {
  FnPtr g = f;
  take_ptr(&g);
  g(); // no-warning
}
