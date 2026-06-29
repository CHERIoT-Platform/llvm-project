// RUN: %riscv32_cheri_cc1 "-triple" "riscv32-unknown-cheriotrtos" "-target-abi" "cheriot" -verify %s 

struct MyObj { int val; };

static struct MyObj __attribute__((cheriot_sealed("MyCompartment", "MyKeyName"))) MyObjInitAndSeal  = { 0 };
extern struct MyObj __attribute__((cheriot_sealed("MyCompartment", "MyKeyName"))) MyObjExtSeal;

void useMyObj(struct MyObj x);
void useRefToMyObj(struct MyObj* x);
void useSealedMyObj(struct MyObj* __sealed_capability  x);

struct MyObj returnMyObjInitAndSeal() {
  return MyObjInitAndSeal;  // expected-error{{the only valid operation on a sealed value is to take its address}}
}

struct MyObj returnMyObjExtSeal() {
  return MyObjExtSeal;  // expected-error{{the only valid operation on a sealed value is to take its address}}
}


static int* __attribute__((cheriot_sealed("MyCompartment", "MyKeyName"))) IntPtrInitAndSeal = 0; 

void useIntPtr(int* x);
void useRefToIntPtr(int** x);
void useSealedIntPtr(int** __sealed_capability  x);

int* returnIntPtrInitAndSeal() {
  return IntPtrInitAndSeal;  // expected-error{{the only valid operation on a sealed value is to take its address}}
}

static int __attribute__((cheriot_sealed("MyCompartment", "MyKeyName"))) IntInitAndSeal = 42; 

void useInt(int x);
void useRefToInt(int* x);
void useSealedInt(int* __sealed_capability  x);

int returnIntInitAndSeal() {
  return IntInitAndSeal; // expected-error{{the only valid operation on a sealed value is to take its address}}
}

void useBool(int b);


void func() {
  useMyObj(MyObjInitAndSeal); // expected-error{{the only valid operation on a sealed value is to take its address}}
  useRefToMyObj(&MyObjInitAndSeal); // expected-error{{converting sealed type 'struct MyObj * __sealed_capability' to non-sealed type 'struct MyObj *' without an explicit unsealing}}
  useSealedMyObj(&MyObjInitAndSeal);
  useBool(&MyObjInitAndSeal == &MyObjInitAndSeal);

  useMyObj(MyObjExtSeal); // expected-error{{the only valid operation on a sealed value is to take its address}}
  useRefToMyObj(&MyObjExtSeal); // expected-error{{converting sealed type 'struct MyObj * __sealed_capability' to non-sealed type 'struct MyObj *' without an explicit unsealing}}
  useSealedMyObj(&MyObjExtSeal);
  useBool(&MyObjExtSeal == &MyObjExtSeal);

  useIntPtr(IntPtrInitAndSeal); // expected-error{{the only valid operation on a sealed value is to take its address}}
  useRefToIntPtr(&IntPtrInitAndSeal); // expected-error{{converting sealed type 'int ** __sealed_capability' to non-sealed type 'int **' without an explicit unsealing}}
  int* ShouldFail1 = &(*IntPtrInitAndSeal); // expected-error{{the only valid operation on a sealed value is to take its address}}
  useSealedIntPtr(&IntPtrInitAndSeal);

  useInt(IntInitAndSeal); // expected-error{{the only valid operation on a sealed value is to take its address}}
  useRefToInt(&IntInitAndSeal); // expected-error{{converting sealed type 'int * __sealed_capability' to non-sealed type 'int *' without an explicit unsealing}}
  int ShouldFail2 = IntInitAndSeal + 10; // expected-error{{the only valid operation on a sealed value is to take its address}}
  int ShouldFail3 = (0 == 0) ? IntInitAndSeal : 0; // expected-error{{the only valid operation on a sealed value is to take its address}}
  int ShouldFail4 = (0 == 0) ? 0 : IntInitAndSeal; // expected-error{{the only valid operation on a sealed value is to take its address}}
  int ShouldFail5 = IntInitAndSeal ? 0 : 1; // expected-error{{the only valid operation on a sealed value is to take its address}}
  int ShouldFail6 = IntInitAndSeal < 0; // expected-error{{the only valid operation on a sealed value is to take its address}}
  int ShouldFail7 = 0 < IntInitAndSeal; // expected-error{{the only valid operation on a sealed value is to take its address}}
  IntInitAndSeal = 0; // expected-error{{the only valid operation on a sealed value is to take its address}}  

  useSealedInt(&IntInitAndSeal);
}
