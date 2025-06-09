// RUN: %riscv32_cheri_cc1 "-triple" "riscv32cheriot-unknown-unknown" "-target-abi" "cheriot" -verify %s 

struct PlainObj {int val;};
struct __attribute__((cheriot_sealed("MyCompartment", "MyKeyName"))) MyObj { int val; }; // expected-error{{the only valid operation on a sealed value is to take its address}}

struct MyObj Obj = {10};

typedef int MyObj2 [[cheriot::sealed("MyCompartment", "MyKeyName")]];
typedef int MyObj3 __attribute__((cheriot_sealed("MyCompartment", "MyKeyName")));
MyObj2 Obj2 = 10;
MyObj3 Obj3 = 10;

typedef int MyObj4[4] __attribute__((cheriot_sealed("MyCompartment", "MyKeyName")));
typedef struct PlainObj *MyObj5 __attribute__((cheriot_sealed("MyCompartment", "MyKeyName")));

MyObj4 Obj4 = {0, 0, 0, 0};

struct PlainObj plainObj = {10};
MyObj5 Obj5 = &plainObj;
extern MyObj5 Obj6; // No warnings here, it is allowed to declare it `extern`. 
					
void useStruct(struct MyObj x);
void useStructSealedCap(struct MyObj *__sealed_capability x);
void useInt(int x);
void useIntSealedCap(int *__sealed_capability x);

void func() {
  struct MyObj* __sealed_capability ptr = &Obj;
  int eq = (&Obj == &Obj);
  int _eq = (Obj == Obj); // expected-error{{the only valid operation on a sealed value is to take its address}}
  int leq = (&Obj < &Obj);
  int _or = ((int) &Obj | (int) &Obj);
  int lor = ((int) &Obj || (int) &Obj);
  Obj.val; // expected-error{{the only valid operation on a sealed value is to take its address}}
  (Obj2 + 1); // expected-error{{the only valid operation on a sealed value is to take its address}}
  (Obj3 - 1); // expected-error{{the only valid operation on a sealed value is to take its address}}
  (Obj2 * 10); // expected-error{{the only valid operation on a sealed value is to take its address}}
  (Obj3 * 10); // expected-error{{the only valid operation on a sealed value is to take its address}}
  (Obj4[0]); // expected-error{{the only valid operation on a sealed value is to take its address}}
  (Obj5->val); // expected-error{{the only valid operation on a sealed value is to take its address}}
  useStruct(Obj);  // expected-note{{in implicit copy constructor for 'MyObj' first required here}}
  useInt(Obj2); // expected-error{{the only valid operation on a sealed value is to take its address}}
  useStructSealedCap(&Obj); 
  useIntSealedCap(&Obj2);
  auto _ = sizeof(*(&Obj)); 
  int x = 10;
  decltype(*(&Obj2)) y = x;

  // Valid in unevaluated contexts.
  auto size = sizeof(Obj5->val); 

  Obj2++; // expected-error{{the only valid operation on a sealed value is to take its address}}
  -Obj2; // expected-error{{the only valid operation on a sealed value is to take its address}}
  Obj2 += 1; // expected-error{{the only valid operation on a sealed value is to take its address}}
  float f = (float) Obj2; // expected-error{{the only valid operation on a sealed value is to take its address}}

  int k1 = Obj3 ; // expected-error{{the only valid operation on a sealed value is to take its address}}
  int k = 1 ? Obj3 : Obj2; // expected-error{{the only valid operation on a sealed value is to take its address}}
}
