// RUN: %clang_cc1 %s -o - "-triple" "riscv32cheriot-unknown-cheriotrtos" "-emit-llvm" "-mframe-pointer=none" "-mcmodel=small" "-target-abi" "cheriot" "-O1" "-Werror" "-cheri-compartment=example" -std=c2x | FileCheck %s

// Test that structs that can fit in two registers are correctly handled, both when used as return values and when passed as an argument. 

#define LENGTH 5
static unsigned int dummies[] = {1, 2, 3, 4, 5};
static unsigned int dummy = 0;

volatile static __attribute__((used)) unsigned int* force_use; 

struct TwoIntegers {
  unsigned int one;
  unsigned int two;
};

__attribute__((cheri_compartment("example"), noinline)) unsigned int GetValue(void) {
  return ++dummy; 
}

// CHECK: define dso_local chericcallcce [2 x i32] @_Z8InitIntsv() local_unnamed_addr addrspace(200) #1 {
__attribute__((cheri_compartment("example"), noinline)) struct TwoIntegers InitInts(void) {
  // CHECK: entry:
  // CHECK:   %call = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %call1 = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %.fca.0.insert = insertvalue [2 x i32] poison, i32 %call, 0
  // CHECK:   %.fca.1.insert = insertvalue [2 x i32] %.fca.0.insert, i32 %call1, 1
  struct TwoIntegers Res = {GetValue(), GetValue()};

  // CHECK:   ret [2 x i32] %.fca.1.insert
  return Res;
}

// CHECK: define dso_local chericcallcce [2 x i32] @_Z7ChgInts11TwoIntegers([2 x i32] %x.coerce) local_unnamed_addr addrspace(200) #1 {
__attribute__((cheri_compartment("example"), noinline)) struct TwoIntegers ChgInts(struct TwoIntegers x) {
  // CHECK: entry:
  // CHECK:  %x.coerce.fca.0.extract = extractvalue [2 x i32] %x.coerce, 0
  // CHECK:  %x.coerce.fca.1.extract = extractvalue [2 x i32] %x.coerce, 1
  // CHECK:  %call = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:  %sub = sub i32 %x.coerce.fca.0.extract, %call
  // CHECK:  %call1 = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:  %sub2 = sub i32 %x.coerce.fca.1.extract, %call1
  // CHECK:  %.fca.0.insert = insertvalue [2 x i32] poison, i32 %sub, 0
  // CHECK:  %.fca.1.insert = insertvalue [2 x i32] %.fca.0.insert, i32 %sub2, 1
  x.one -= GetValue();
  x.two -= GetValue();

  // CHECK:   ret [2 x i32] %.fca.1.insert
  return x;
}


// Here we want to check that the initial struct received from the callee is handled as a proper struct, i.e. fields are read using `extractvalue` rather than dereferencing pointers.
// CHECK: define dso_local chericcallcce void @_Z9CheckIntsv() local_unnamed_addr addrspace(200) #2 {
__attribute__((cheri_compartment("example"))) void CheckInts() {

  static struct TwoIntegers __attribute__((used)) x = {}; 

  // CHECK: entry:
  // CHECK:   %call = tail call chericcallcce [2 x i32] @_Z8InitIntsv()
  // CHECK:   %call1 = tail call chericcallcce [2 x i32] @_Z7ChgInts11TwoIntegers([2 x i32] %call)
  // CHECK:   %call1.fca.0.extract = extractvalue [2 x i32] %call1, 0
  // CHECK:   %call1.fca.1.extract = extractvalue [2 x i32] %call1, 1
  // CHECK:   store i32 %call1.fca.0.extract, ptr addrspace(200) @_Z9CheckIntsv.x, align 4, !tbaa !7
  // CHECK:   store i32 %call1.fca.1.extract, ptr addrspace(200) getelementptr inbounds nuw (i8, ptr addrspace(200) @_Z9CheckIntsv.x, i32 4), align 4, !tbaa !7
  x = ChgInts(InitInts());
  
  // CHECK:  ret void
  return;
}


// Do the same but use a struct of two pointers. 

struct TwoPointers {
  unsigned int *one;
  unsigned int *two;
};

// Here we want to check that the result is returned by value.
// CHECK: define dso_local chericcallcce %struct.TwoPointers @_Z8InitPtrsv() local_unnamed_addr addrspace(200) #1
__attribute__((cheri_compartment("example"), noinline)) struct TwoPointers InitPtrs() {
  // CHECK: entry:
  // CHECK:   %call = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %rem = urem i32 %call, 5
  // CHECK:   %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem
  // CHECK:   %call1 = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %rem2 = urem i32 %call1, 5
  // CHECK:   %add.ptr3 = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem2
  struct TwoPointers x = {dummies + (GetValue() % LENGTH), dummies + (GetValue() % LENGTH)};

  // CHECK:   %.fca.0.insert = insertvalue %struct.TwoPointers poison, ptr addrspace(200) %add.ptr, 0
  // CHECK:   %.fca.1.insert = insertvalue %struct.TwoPointers %.fca.0.insert, ptr addrspace(200) %add.ptr3, 1
  // CHECK:   ret %struct.TwoPointers %.fca.1.insert
  return x;
}

// Here we want to check that the struct received as parameter is laid out as two different arguments and, again, is returned by value.
// CHECK: define dso_local chericcallcce %struct.TwoPointers @_Z7ChgPtrs11TwoPointers(ptr addrspace(200) %x.coerce0, ptr addrspace(200) %x.coerce1) local_unnamed_addr addrspace(200) #1 {
__attribute__((cheri_compartment("example"), noinline)) struct TwoPointers ChgPtrs(struct TwoPointers x) {

  // CHECK: entry:
  // CHECK:   store ptr addrspace(200) %x.coerce0, ptr addrspace(200) @force_use, align 8, !tbaa !11
  force_use = x.one;

  // CHECK:   %call = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %rem = urem i32 %call, 5
  // CHECK:   %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem
  x.one = dummies + (GetValue() % LENGTH);

  // CHECK:   store ptr addrspace(200) %x.coerce1, ptr addrspace(200) @force_use, align 8, !tbaa !11
  force_use = x.two;

  // CHECK:   %call2 = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %rem3 = urem i32 %call2, 5
  // CHECK:   %add.ptr4 = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem3
  x.two = dummies + (GetValue() % LENGTH);

  // CHECK:   %.fca.0.insert = insertvalue %struct.TwoPointers poison, ptr addrspace(200) %add.ptr, 0
  // CHECK:   %.fca.1.insert = insertvalue %struct.TwoPointers %.fca.0.insert, ptr addrspace(200) %add.ptr4, 1
  // CHECK:   ret %struct.TwoPointers %.fca.1.insert
  return x;
}

// Here we want to check that the struct received from the callee is handled as a proper struct, i.e. fields are read using `extractvalue` rather than dereferencing pointers. Also, we want to check that arguments in composite calls are passed correctly.
// CHECK: define dso_local chericcallcce void @_Z9CheckPtrsv() local_unnamed_addr addrspace(200) #2 {
__attribute__((cheri_compartment("example"))) void CheckPtrs() {

  static struct TwoPointers __attribute__((used)) x = {}; 

  // CHECK: entry:
  // CHECK:   %call = tail call chericcallcce %struct.TwoPointers @_Z8InitPtrsv()
  // CHECK:   %0 = extractvalue %struct.TwoPointers %call, 0
  // CHECK:   %1 = extractvalue %struct.TwoPointers %call, 1
  // CHECK:   %call1 = tail call chericcallcce %struct.TwoPointers @_Z7ChgPtrs11TwoPointers(ptr addrspace(200) %0, ptr addrspace(200) %1)
  // CHECK:   %2 = extractvalue %struct.TwoPointers %call1, 0
  // CHECK:   %3 = extractvalue %struct.TwoPointers %call1, 1
  // CHECK:   store ptr addrspace(200) %2, ptr addrspace(200) @_Z9CheckPtrsv.x, align 8, !tbaa !11
  // CHECK:   store ptr addrspace(200) %3, ptr addrspace(200) getelementptr inbounds nuw (i8, ptr addrspace(200) @_Z9CheckPtrsv.x, i32 8), align 8, !tbaa !11
  x = ChgPtrs(InitPtrs());

  // CHECK:  ret void
  return;
}


// Do the same with a pointer and an integer.

struct PointerAndInt {
  unsigned int *one;
  unsigned int two;
};

// Here we want to check that the result is returned by value.
// CHECK:  define dso_local chericcallcce %struct.PointerAndInt @_Z10InitPtrIntv() local_unnamed_addr addrspace(200) #1 {
__attribute__((cheri_compartment("example"), noinline)) struct PointerAndInt InitPtrInt() {
  // CHECK:  entry:
  // CHECK:    %call = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:    %rem = urem i32 %call, 5
  // CHECK:    %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem
  // CHECK:    %call1 = tail call chericcallcce i32 @_Z8GetValuev()
  struct PointerAndInt x = {dummies + (GetValue() % LENGTH), GetValue()};

  // CHECK:    %.fca.0.insert = insertvalue %struct.PointerAndInt poison, ptr addrspace(200) %add.ptr, 0
  // CHECK:    %.fca.1.insert = insertvalue %struct.PointerAndInt %.fca.0.insert, i32 %call1, 1
  // CHECK:    ret %struct.PointerAndInt %.fca.1.insert
  return x;
}

// Here we want to check that the struct received as parameter is laid out as two different arguments and, again, is returned by value.
// CHECK: define dso_local chericcallcce %struct.PointerAndInt @_Z9ChgPtrInt13PointerAndInt(ptr addrspace(200) %x.coerce0, i32 %x.coerce1) local_unnamed_addr addrspace(200) #1 {
__attribute__((cheri_compartment("example"), noinline)) struct PointerAndInt ChgPtrInt(struct PointerAndInt x) {
  // CHECK: entry:
  // CHECK:   store ptr addrspace(200) %x.coerce0, ptr addrspace(200) @force_use, align 8, !tbaa !11
  force_use = x.one;

  // CHECK:   %call = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %rem = urem i32 %call, 5
  // CHECK:   %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem
  x.one = dummies + (GetValue() % LENGTH);

  // CHECK:   %call2 = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %sub = sub i32 %x.coerce1, %call2
  x.two -= GetValue();

  // CHECK:   %.fca.0.insert = insertvalue %struct.PointerAndInt poison, ptr addrspace(200) %add.ptr, 0
  // CHECK:   %.fca.1.insert = insertvalue %struct.PointerAndInt %.fca.0.insert, i32 %sub, 1
  // CHECK:   ret %struct.PointerAndInt %.fca.1.insert
  return x;
}

// Here we want to check that the struct received from the callee is handled as a proper struct, i.e. fields are read using `extractvalue` rather than dereferencing pointers. Also, we want to check that arguments in composite calls are passed correctly.
// CHECK: define dso_local chericcallcce void @_Z11CheckPtrIntv() local_unnamed_addr addrspace(200) #2 {
__attribute__((cheri_compartment("example"))) void CheckPtrInt() {

  static struct PointerAndInt __attribute__((used)) x = {}; 

  // CHECK: entry:
  // CHECK:   %call = tail call chericcallcce %struct.PointerAndInt @_Z10InitPtrIntv()
  // CHECK:   %0 = extractvalue %struct.PointerAndInt %call, 0
  // CHECK:   %1 = extractvalue %struct.PointerAndInt %call, 1
  // CHECK:   %call1 = tail call chericcallcce %struct.PointerAndInt @_Z9ChgPtrInt13PointerAndInt(ptr addrspace(200) %0, i32 %1)
  // CHECK:   %2 = extractvalue %struct.PointerAndInt %call1, 0
  // CHECK:   %3 = extractvalue %struct.PointerAndInt %call1, 1
  // CHECK:   store ptr addrspace(200) %2, ptr addrspace(200) @_Z11CheckPtrIntv.x, align 8, !tbaa !11
  // CHECK:   store i32 %3, ptr addrspace(200) getelementptr inbounds nuw (i8, ptr addrspace(200) @_Z11CheckPtrIntv.x, i32 8), align 8, !tbaa !7
  x = ChgPtrInt(InitPtrInt());

  // CHECK:   ret void
  return;
}


// Do the same with an integer and a pointer (order does not matter).

struct IntAndPointer {
  unsigned int one;
  unsigned int *two;
};

// Here we want to check that the result is returned by value.
// CHECK: define dso_local chericcallcce %struct.IntAndPointer @_Z10InitIntPtrv() local_unnamed_addr addrspace(200) #1 {
__attribute__((cheri_compartment("example"), noinline)) struct IntAndPointer InitIntPtr() {
  // CHECK: entry:
  // CHECK:   %call = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %call1 = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %rem = urem i32 %call1, 5
  // CHECK:   %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem
  struct IntAndPointer x = {GetValue(), dummies + (GetValue() % LENGTH)};

  // CHECK:   %.fca.0.insert = insertvalue %struct.IntAndPointer poison, i32 %call, 0
  // CHECK:   %.fca.1.insert = insertvalue %struct.IntAndPointer %.fca.0.insert, ptr addrspace(200) %add.ptr, 1
  // CHECK:   ret %struct.IntAndPointer %.fca.1.insert
  return x;
}

// Here we want to check that the struct received as parameter is laid out as two different arguments and, again, is returned by value.
// CHECK: define dso_local chericcallcce %struct.IntAndPointer @_Z9ChgIntPtr13IntAndPointer(i32 %x.coerce0, ptr addrspace(200) %x.coerce1) local_unnamed_addr addrspace(200) #1 {
__attribute__((cheri_compartment("example"), noinline)) struct IntAndPointer ChgIntPtr(struct IntAndPointer x) {

  // CHECK: entry:
  // CHECK:   %call = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %sub = sub i32 %x.coerce0, %call
  x.one -= GetValue();

  // CHECK:   store ptr addrspace(200) %x.coerce1, ptr addrspace(200) @force_use, align 8, !tbaa !11
  force_use = x.two;

  // CHECK:   %call1 = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %rem = urem i32 %call1, 5
  // CHECK:   %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem
  x.two = dummies + (GetValue() % LENGTH);

  // CHECK:   %.fca.0.insert = insertvalue %struct.IntAndPointer poison, i32 %sub, 0
  // CHECK:   %.fca.1.insert = insertvalue %struct.IntAndPointer %.fca.0.insert, ptr addrspace(200) %add.ptr, 1
  // CHECK:   ret %struct.IntAndPointer %.fca.1.insert
  return x;
}

// Here we want to check that the struct received from the callee is handled as a proper struct, i.e. fields are read using `extractvalue` rather than dereferencing pointers. Also, we want to check that arguments in composite calls are passed correctly.
// CHECK: define dso_local chericcallcce void @_Z11CheckIntPtrv() local_unnamed_addr addrspace(200) #2 {
__attribute__((cheri_compartment("example"))) void CheckIntPtr() {

  static struct IntAndPointer __attribute__((used)) x = {}; 

  // CHECK: entry:
  // CHECK:   %call = tail call chericcallcce %struct.IntAndPointer @_Z10InitIntPtrv()
  // CHECK:   %0 = extractvalue %struct.IntAndPointer %call, 0
  // CHECK:   %1 = extractvalue %struct.IntAndPointer %call, 1
  // CHECK:   %call1 = tail call chericcallcce %struct.IntAndPointer @_Z9ChgIntPtr13IntAndPointer(i32 %0, ptr addrspace(200) %1)
  // CHECK:   %2 = extractvalue %struct.IntAndPointer %call1, 0
  // CHECK:   %3 = extractvalue %struct.IntAndPointer %call1, 1
  // CHECK:   store i32 %2, ptr addrspace(200) @_Z11CheckIntPtrv.x, align 8, !tbaa !7
  // CHECK:   store ptr addrspace(200) %3, ptr addrspace(200) getelementptr inbounds nuw (i8, ptr addrspace(200) @_Z11CheckIntPtrv.x, i32 8), align 8, !tbaa !11
  x = ChgIntPtr(InitIntPtr());

  // CHECK:   ret void
  return;
}


// What happens in this case? 

struct Inner {
  unsigned int z;
};

struct Parent {
  unsigned int x; 
  struct Inner y;
};


// Here we want to check that the result is returned by value.
// CHECK: define dso_local chericcallcce [2 x i32] @_Z10InitParentv() local_unnamed_addr addrspace(200) #1 {
__attribute__((cheri_compartment("example"), noinline)) struct Parent InitParent() {
  // CHECK: entry:
  // CHECK:   %call = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %call1 = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %.fca.0.insert = insertvalue [2 x i32] poison, i32 %call, 0
  // CHECK:   %.fca.1.insert = insertvalue [2 x i32] %.fca.0.insert, i32 %call1, 1
  struct Parent x = {GetValue(), {GetValue()}};

  // CHECK:   ret [2 x i32] %.fca.1.insert
  return x;
}

// Here we want to check that the struct received as parameter is laid out as two different arguments and, again, is returned by value.
// CHECK: define dso_local chericcallcce [2 x i32] @_Z9ChgParent6Parent([2 x i32] %x.coerce) local_unnamed_addr addrspace(200) #1 {
__attribute__((cheri_compartment("example"), noinline)) struct Parent ChgParent(struct Parent x) {




  // CHECK: entry:
  // CHECK:   %x.coerce.fca.0.extract = extractvalue [2 x i32] %x.coerce, 0
  // CHECK:   %x.coerce.fca.1.extract = extractvalue [2 x i32] %x.coerce, 1
  // CHECK:   %call = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %sub = sub i32 %x.coerce.fca.0.extract, %call
  x.x -= GetValue();

  // CHECK:   %call2 = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %sub3 = sub i32 %x.coerce.fca.1.extract, %call2
  x.y.z -= GetValue();

  // CHECK:   %.fca.0.insert = insertvalue [2 x i32] poison, i32 %sub, 0
  // CHECK:   %.fca.1.insert = insertvalue [2 x i32] %.fca.0.insert, i32 %sub3, 1
  // CHECK:   ret [2 x i32] %.fca.1.insert
  return x;
}

// Here we want to check that the struct received from the callee is handled as a proper struct, i.e. fields are read using `extractvalue` rather than dereferencing pointers. Also, we want to check that arguments in composite calls are passed correctly.
// CHECK: define dso_local chericcallcce void @_Z11CheckParentv() local_unnamed_addr addrspace(200) #2 {
__attribute__((cheri_compartment("example"))) void CheckParent() {

  static struct Parent __attribute__((used)) x = {}; 

  // CHECK: entry:
  // CHECK:   %call = tail call chericcallcce [2 x i32] @_Z10InitParentv()
  // CHECK:   %call1 = tail call chericcallcce [2 x i32] @_Z9ChgParent6Parent([2 x i32] %call)
  // CHECK:   %call1.fca.0.extract = extractvalue [2 x i32] %call1, 0
  // CHECK:   %call1.fca.1.extract = extractvalue [2 x i32] %call1, 1
  // CHECK:   store i32 %call1.fca.0.extract, ptr addrspace(200) @_Z11CheckParentv.x, align 4, !tbaa !7
  // CHECK:   store i32 %call1.fca.1.extract, ptr addrspace(200) getelementptr inbounds nuw (i8, ptr addrspace(200) @_Z11CheckParentv.x, i32 4), align 4, !tbaa !7
  x = ChgParent(InitParent());

  // CHECK:   ret void
  return;
}

struct InnerPtr {
  unsigned int* z;
};

struct ParentPtr {
  unsigned int* x; 
  struct InnerPtr y;
};


// Here we want to check that the result is returned by value.
// CHECK: define dso_local chericcallcce %struct.ParentPtr @_Z13InitParentPtrv() local_unnamed_addr addrspace(200) #1 {
__attribute__((cheri_compartment("example"), noinline)) struct ParentPtr InitParentPtr() {
  // CHECK: entry:
  // CHECK:   %call = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %rem = urem i32 %call, 5
  // CHECK:   %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem
  // CHECK:   %call1 = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %rem2 = urem i32 %call1, 5
  // CHECK:   %add.ptr3 = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem2
  // CHECK:   %.fca.0.insert = insertvalue %struct.ParentPtr poison, ptr addrspace(200) %add.ptr, 0
  // CHECK:   %.fca.1.0.insert = insertvalue %struct.ParentPtr %.fca.0.insert, ptr addrspace(200) %add.ptr3, 1, 0
  struct ParentPtr x = {dummies + (GetValue() % LENGTH), {dummies + (GetValue() % LENGTH)}};

  // CHECK:   ret %struct.ParentPtr %.fca.1.0.insert
  return x;
}

// Here we want to check that the struct received as parameter is laid out as two different arguments and, again, is returned by value.
// CHECK: define dso_local chericcallcce %struct.ParentPtr @_Z12ChgParentPtr9ParentPtr(ptr addrspace(200) %x.coerce0, %struct.InnerPtr %x.coerce1) local_unnamed_addr addrspace(200) #1 {
__attribute__((cheri_compartment("example"), noinline)) struct ParentPtr ChgParentPtr(struct ParentPtr x) {
  // CHECK: entry:
  // CHECK:   %x.coerce1.fca.0.extract = extractvalue %struct.InnerPtr %x.coerce1, 0

  // CHECK:   store ptr addrspace(200) %x.coerce0, ptr addrspace(200) @force_use, align 8, !tbaa !11
  force_use = x.x;

  // CHECK:   %call = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %rem = urem i32 %call, 5
  // CHECK:   %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem
  x.x = dummies + (GetValue() % LENGTH);

  // CHECK:   store ptr addrspace(200) %x.coerce1.fca.0.extract, ptr addrspace(200) @force_use, align 8, !tbaa !11
  force_use = x.y.z;

  // CHECK:   %call3 = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %rem4 = urem i32 %call3, 5
  // CHECK:   %add.ptr5 = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem4
  x.y.z = dummies + (GetValue() % LENGTH);

  // CHECK:   %.fca.0.insert = insertvalue %struct.ParentPtr poison, ptr addrspace(200) %add.ptr, 0
  // CHECK:   %.fca.1.0.insert = insertvalue %struct.ParentPtr %.fca.0.insert, ptr addrspace(200) %add.ptr5, 1, 0
  // CHECK:   ret %struct.ParentPtr %.fca.1.0.insert
  return x;
}

// Here we want to check that the struct received from the callee is handled as a proper struct, i.e. fields are read using `extractvalue` rather than dereferencing pointers. Also, we want to check that arguments in composite calls are passed correctly.
// CHECK: define dso_local chericcallcce void @_Z14CheckParentPtrv() local_unnamed_addr addrspace(200) #2 {
__attribute__((cheri_compartment("example"))) void CheckParentPtr() {

  static struct ParentPtr __attribute__((used)) x = {}; 

  // CHECK: entry:
  // CHECK:   %call = tail call chericcallcce %struct.ParentPtr @_Z13InitParentPtrv()
  // CHECK:   %0 = extractvalue %struct.ParentPtr %call, 0
  // CHECK:   %1 = extractvalue %struct.ParentPtr %call, 1
  // CHECK:   %call1 = tail call chericcallcce %struct.ParentPtr @_Z12ChgParentPtr9ParentPtr(ptr addrspace(200) %0, %struct.InnerPtr %1)
  // CHECK:   %2 = extractvalue %struct.ParentPtr %call1, 0
  // CHECK:   %3 = extractvalue %struct.ParentPtr %call1, 1
  // CHECK:   %.fca.0.extract3 = extractvalue %struct.InnerPtr %3, 0
  // CHECK:   store ptr addrspace(200) %2, ptr addrspace(200) @_Z14CheckParentPtrv.x, align 8, !tbaa !11
  // CHECK:   store ptr addrspace(200) %.fca.0.extract3, ptr addrspace(200) getelementptr inbounds nuw (i8, ptr addrspace(200) @_Z14CheckParentPtrv.x, i32 8), align 8, !tbaa !11
  x = ChgParentPtr(InitParentPtr());

  // CHECK:   ret void
  return;
}

// For arguments, does it work when the struct is placed in an odd-numbered position? 

// CHECK: define dso_local chericcallcce [2 x i32] @_Z8ChgInts2i11TwoIntegers(i32 noundef %new_int, [2 x i32] %x.coerce) local_unnamed_addr addrspace(200) #4 {
__attribute__((cheri_compartment("example"), noinline)) struct TwoIntegers ChgInts2(int new_int, struct TwoIntegers x) {
  // CHECK: %x.coerce.fca.0.extract = extractvalue [2 x i32] %x.coerce, 0
  // CHECK: %x.coerce.fca.1.extract = extractvalue [2 x i32] %x.coerce, 1
  // CHECK: %add = add i32 %x.coerce.fca.0.extract, %new_int
  x.one += new_int;

  // CHECK: %add1 = add i32 %x.coerce.fca.1.extract, %new_int
  x.two += new_int;

  // CHECK: %.fca.0.insert = insertvalue [2 x i32] poison, i32 %add, 0
  // CHECK: %.fca.1.insert = insertvalue [2 x i32] %.fca.0.insert, i32 %add1, 1
  // CHECK:  ret [2 x i32] %.fca.1.insert
  return x;
}

// CHECK: define dso_local chericcallcce %struct.TwoPointers @_Z8ChgPtrs2i11TwoPointers(i32 noundef %new_int, ptr addrspace(200) %x.coerce0, ptr addrspace(200) %x.coerce1) local_unnamed_addr addrspace(200) #1 {
__attribute__((cheri_compartment("example"), noinline)) struct TwoPointers ChgPtrs2(int new_int, struct TwoPointers x) {
  // CHECK: entry:
  // CHECK:   store ptr addrspace(200) %x.coerce0, ptr addrspace(200) @force_use, align 8, !tbaa !11
  force_use = x.one;

  // CHECK:   %call = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %add = add i32 %call, %new_int
  // CHECK:   %rem = urem i32 %add, 5
  // CHECK:   %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem
  x.one = dummies + ((GetValue() + new_int) % LENGTH);

  // CHECK:   store ptr addrspace(200) %x.coerce1, ptr addrspace(200) @force_use, align 8, !tbaa !11
  force_use = x.two;

  // CHECK:   %call2 = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %add3 = add i32 %call2, %new_int
  // CHECK:   %rem4 = urem i32 %add3, 5
  // CHECK:   %add.ptr5 = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem4
  x.two = dummies + ((GetValue() + new_int) % LENGTH);

  // CHECK:   %.fca.0.insert = insertvalue %struct.TwoPointers poison, ptr addrspace(200) %add.ptr, 0
  // CHECK:   %.fca.1.insert = insertvalue %struct.TwoPointers %.fca.0.insert, ptr addrspace(200) %add.ptr5, 1
  // CHECK:   ret %struct.TwoPointers %.fca.1.insert
  return x;
}

// CHECK: define dso_local chericcallcce %struct.ParentPtr @_Z13ChgParentPtr2i9ParentPtr(i32 noundef %new_int, ptr addrspace(200) %x.coerce0, %struct.InnerPtr %x.coerce1) local_unnamed_addr addrspace(200) #1 {
__attribute__((cheri_compartment("example"), noinline)) struct ParentPtr ChgParentPtr2(int new_int, struct ParentPtr x) {

  // CHECK: entry:
  // CHECK:   %x.coerce1.fca.0.extract = extractvalue %struct.InnerPtr %x.coerce1, 0

  // CHECK:   store ptr addrspace(200) %x.coerce0, ptr addrspace(200) @force_use, align 8, !tbaa !11
  force_use = x.x;

  // CHECK:   %call = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %add = add i32 %call, %new_int
  // CHECK:   %rem = urem i32 %add, 5
  // CHECK:   %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem
  x.x = dummies + ((GetValue() + new_int) % LENGTH);

  // CHECK:   store ptr addrspace(200) %x.coerce1.fca.0.extract, ptr addrspace(200) @force_use, align 8, !tbaa !11
  force_use = x.y.z;

  // CHECK:   %call3 = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %add4 = add i32 %call3, %new_int
  // CHECK:   %rem5 = urem i32 %add4, 5
  // CHECK:   %add.ptr6 = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem5
  x.y.z = dummies + ((GetValue() + new_int) % LENGTH);

  // CHECK:   %.fca.0.insert = insertvalue %struct.ParentPtr poison, ptr addrspace(200) %add.ptr, 0
  // CHECK:   %.fca.1.0.insert = insertvalue %struct.ParentPtr %.fca.0.insert, ptr addrspace(200) %add.ptr6, 1, 0
  // CHECK:   ret %struct.ParentPtr %.fca.1.0.insert
  return x;
}


// For arguments, does it work when the function is variadic? 
// CHECK: define dso_local chericcallcce [2 x i32] @_Z8ChgInts3i11TwoIntegersz(i32 noundef %n, [2 x i32] %x.coerce, ...) local_unnamed_addr addrspace(200) #5 {
__attribute__((cheri_compartment("example"), noinline)) struct TwoIntegers ChgInts3(int n, struct TwoIntegers x, ...) {
  __builtin_va_list args;

  // CHECK: call void @llvm.va_start.p200(ptr addrspace(200) nonnull %args)
  __builtin_va_start(args, x);
  
  for (int i = 0; i < n; i++)  {

	int v = __builtin_va_arg(args, int);

	// CHECK: for.cond.cleanup:                                 ; preds = %for.cond.cleanup.loopexit, %entry
	// CHECK:   %.fca.1.insert.merged = phi [2 x i32] [ %x.coerce, %entry ], [ %1, %for.cond.cleanup.loopexit ]
	// CHECK:   call void @llvm.va_end.p200(ptr addrspace(200) %args)
	// CHECK:   call void @llvm.lifetime.end.p200(i64 8, ptr addrspace(200) nonnull %args) #8
	// CHECK:   ret [2 x i32] %.fca.1.insert.merged

    x.one += v;
    x.two += v;
  }
  
  __builtin_va_end(args);
  
  return x;
}

// CHECK: define dso_local chericcallcce %struct.TwoPointers @_Z8ChgPtrs3i11TwoPointersz(i32 noundef %n, ptr addrspace(200) %x.coerce0, ptr addrspace(200) %x.coerce1, ...) local_unnamed_addr addrspace(200) #7 {
__attribute__((cheri_compartment("example"), noinline)) struct TwoPointers ChgPtrs3(int n, struct TwoPointers x, ...) {

  __builtin_va_list args;

  __builtin_va_start(args, x);
  
  // CHECK: store ptr addrspace(200) %x.coerce0, ptr addrspace(200) @force_use, align 8, !tbaa !11
  force_use = x.one;

  // -- just to force the line above -- 
  volatile int _ = GetValue();

  // CHECK:  store ptr addrspace(200) %x.coerce1, ptr addrspace(200) @force_use, align 8, !tbaa !11
  force_use = x.two;

  for (int i = 0; i < n; i++)  {

	int v = __builtin_va_arg(args, int);

	// CHECK: for.cond.cleanup:                                 ; preds = %for.cond.for.cond.cleanup_crit_edge, %entry
	// CHECK:  	%x.sroa.0.0.lcssa = phi ptr addrspace(200) [ %add.ptr.le, %for.cond.for.cond.cleanup_crit_edge ], [ %x.coerce0, %entry ]
	// CHECK:  	%x.sroa.4.0.lcssa = phi ptr addrspace(200) [ %add.ptr6.le, %for.cond.for.cond.cleanup_crit_edge ], [ %x.coerce1, %entry ]
	// CHECK:  	call void @llvm.va_end.p200(ptr addrspace(200) %args)
	// CHECK:  	call void @llvm.lifetime.end.p200(i64 4, ptr addrspace(200) nonnull %_)
	// CHECK:  	call void @llvm.lifetime.end.p200(i64 8, ptr addrspace(200) nonnull %args) #8
	// CHECK:  	%.fca.0.insert = insertvalue %struct.TwoPointers poison, ptr addrspace(200) %x.sroa.0.0.lcssa, 0
	// CHECK:  	%.fca.1.insert = insertvalue %struct.TwoPointers %.fca.0.insert, ptr addrspace(200) %x.sroa.4.0.lcssa, 1
	// CHECK:  	ret %struct.TwoPointers %.fca.1.insert


    x.one = dummies + ((GetValue() + v) % LENGTH);
    x.two = dummies + ((GetValue() + v) % LENGTH);
  }
  
  __builtin_va_end(args);
  
  return x;
}

// CHECK: define dso_local chericcallcce %struct.ParentPtr @_Z13ChgParentPtr3i9ParentPtrz(i32 noundef %n, ptr addrspace(200) %x.coerce0, %struct.InnerPtr %x.coerce1, ...) local_unnamed_addr addrspace(200) #7 {
__attribute__((cheri_compartment("example"), noinline)) struct ParentPtr ChgParentPtr3(int n, struct ParentPtr x, ...) {

  __builtin_va_list args;

  __builtin_va_start(args, x);
  
  // CHECK: store ptr addrspace(200) %x.coerce0, ptr addrspace(200) @force_use, align 8, !tbaa !11
  force_use = x.x;

  // -- just to force the line above -- 
  volatile int _ = GetValue();

  // CHECK: store ptr addrspace(200) %x.coerce1.fca.0.extract, ptr addrspace(200) @force_use, align 8, !tbaa !11
  force_use = x.y.z;

  for (int i = 0; i < n; i++)  {

	int v = __builtin_va_arg(args, int);

	// CHECK: for.cond.cleanup:                                 ; preds = %for.cond.for.cond.cleanup_crit_edge, %entry
	// CHECK:   %x.sroa.0.0.lcssa = phi ptr addrspace(200) [ %add.ptr.le, %for.cond.for.cond.cleanup_crit_edge ], [ %x.coerce0, %entry ]
	// CHECK:   %x.sroa.4.0.lcssa = phi ptr addrspace(200) [ %add.ptr7.le, %for.cond.for.cond.cleanup_crit_edge ], [ %x.coerce1.fca.0.extract, %entry ]
	// CHECK:   call void @llvm.va_end.p200(ptr addrspace(200) %args)
	// CHECK:   call void @llvm.lifetime.end.p200(i64 4, ptr addrspace(200) nonnull %_)
	// CHECK:   call void @llvm.lifetime.end.p200(i64 8, ptr addrspace(200) nonnull %args) #8
	// CHECK:   %.fca.0.insert = insertvalue %struct.ParentPtr poison, ptr addrspace(200) %x.sroa.0.0.lcssa, 0
	// CHECK:   %.fca.1.0.insert = insertvalue %struct.ParentPtr %.fca.0.insert, ptr addrspace(200) %x.sroa.4.0.lcssa, 1, 0
	// CHECK:   ret %struct.ParentPtr %.fca.1.0.insert

    x.x = dummies + ((GetValue() + v) % LENGTH);
    x.y.z = dummies + ((GetValue() + v) % LENGTH);
  }
  
  __builtin_va_end(args);
  
  return x;
}


// For arguments, does it work correctly when the "optimizable" argument sits across the "put in registers"  and "spill to stack" boundary? 

// CHECK: define dso_local chericcallcce [2 x i32] @_Z8ChgInts4iiiii11TwoIntegers(i32 noundef %n0, i32 noundef %n1, i32 noundef %n2, i32 noundef %n3, i32 noundef %n4, [2 x i32] %x.coerce) local_unnamed_addr addrspace(200) #4 {
__attribute__((cheri_compartment("example"), noinline)) struct TwoIntegers ChgInts4(int n0, int n1, int n2, int n3, int n4, struct TwoIntegers x) {

  // CHECK: entry:
  // CHECK:   %x.coerce.fca.0.extract = extractvalue [2 x i32] %x.coerce, 0
  // CHECK:   %x.coerce.fca.1.extract = extractvalue [2 x i32] %x.coerce, 1

  // CHECK:   %add = add nsw i32 %n1, %n0
  // CHECK:   %add1 = add nsw i32 %add, %n2
  // CHECK:   %add2 = add nsw i32 %add1, %n3
  // CHECK:   %add3 = add nsw i32 %add2, %n4
  // CHECK:   %add4 = add i32 %x.coerce.fca.0.extract, %add3
  x.one += n0 + n1 + n2 + n3 + n4;

  // CHECK:   %add9 = add i32 %x.coerce.fca.1.extract, %add3
  x.two += n0 + n1 + n2 + n3 + n4;

  // CHECK:   %.fca.0.insert = insertvalue [2 x i32] poison, i32 %add4, 0
  // CHECK:   %.fca.1.insert = insertvalue [2 x i32] %.fca.0.insert, i32 %add9, 1
  // CHECK:   ret [2 x i32] %.fca.1.insert
  return x;
}

// CHECK: define dso_local chericcallcce %struct.TwoPointers @_Z8ChgPtrs4iiiii11TwoPointers(i32 noundef %n0, i32 noundef %n1, i32 noundef %n2, i32 noundef %n3, i32 noundef %n4, ptr addrspace(200) %x.coerce0, ptr addrspace(200) %x.coerce1) local_unnamed_addr addrspace(200) #1 {
__attribute__((cheri_compartment("example"), noinline)) struct TwoPointers ChgPtrs4(int n0, int n1, int n2, int n3, int n4, struct TwoPointers x) {

  // CHECK: entry:
  // CHECK:   store ptr addrspace(200) %x.coerce0, ptr addrspace(200) @force_use, align 8, !tbaa !11
  force_use = x.one;

  // CHECK:   %call = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %add = add i32 %n1, %n0
  // CHECK:   %add1 = add i32 %add, %n2
  // CHECK:   %add2 = add i32 %add1, %n3
  // CHECK:   %add3 = add i32 %add2, %n4
  // CHECK:   %add4 = add i32 %add3, %call
  // CHECK:   %rem = urem i32 %add4, 5
  // CHECK:   %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem
  x.one = dummies + ((GetValue() + n0 + n1 + n2 + n3 + n4) % LENGTH);

  // CHECK:   store ptr addrspace(200) %x.coerce1, ptr addrspace(200) @force_use, align 8, !tbaa !11
  force_use = x.two;

  // CHECK:   %call6 = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %add7 = add i32 %n1, %n0
  // CHECK:   %add8 = add i32 %add7, %n2
  // CHECK:   %add9 = add i32 %add8, %n3
  // CHECK:   %add10 = add i32 %add9, %n4
  // CHECK:   %add11 = add i32 %add10, %call6
  // CHECK:   %rem12 = urem i32 %add11, 5
  // CHECK:   %add.ptr13 = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem12
  x.two = dummies + ((GetValue() + n0 + n1 + n2 + n3 + n4) % LENGTH);

  // CHECK:   %.fca.0.insert = insertvalue %struct.TwoPointers poison, ptr addrspace(200) %add.ptr, 0
  // CHECK:   %.fca.1.insert = insertvalue %struct.TwoPointers %.fca.0.insert, ptr addrspace(200) %add.ptr13, 1
  // CHECK:   ret %struct.TwoPointers %.fca.1.insert
  return x;
}

// CHECK: define dso_local chericcallcce %struct.ParentPtr @_Z13ChgParentPtr4iiiii9ParentPtr(i32 noundef %n0, i32 noundef %n1, i32 noundef %n2, i32 noundef %n3, i32 noundef %n4, ptr addrspace(200) %x.coerce0, %struct.InnerPtr %x.coerce1) local_unnamed_addr addrspace(200) #1 {
__attribute__((cheri_compartment("example"), noinline)) struct ParentPtr ChgParentPtr4(int n0, int n1, int n2, int n3, int n4, struct ParentPtr x) {

  // CHECK: entry:
  // CHECK:   %x.coerce1.fca.0.extract = extractvalue %struct.InnerPtr %x.coerce1, 0


  // CHECK:   store ptr addrspace(200) %x.coerce0, ptr addrspace(200) @force_use, align 8, !tbaa !11
  force_use = x.x;

  // CHECK:   %call = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %add = add i32 %n1, %n0
  // CHECK:   %add2 = add i32 %add, %n2
  // CHECK:   %add3 = add i32 %add2, %n3
  // CHECK:   %add4 = add i32 %add3, %n4
  // CHECK:   %add5 = add i32 %add4, %call
  // CHECK:   %rem = urem i32 %add5, 5
  // CHECK:   %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem


  x.x= dummies + ((GetValue() + n0 + n1 + n2 + n3 + n4) % LENGTH);

  // CHECK:   store ptr addrspace(200) %x.coerce1.fca.0.extract, ptr addrspace(200) @force_use, align 8, !tbaa !11
  force_use = x.y.z;

  // CHECK:   %call7 = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %add8 = add i32 %n1, %n0
  // CHECK:   %add9 = add i32 %add8, %n2
  // CHECK:   %add10 = add i32 %add9, %n3
  // CHECK:   %add11 = add i32 %add10, %n4
  // CHECK:   %add12 = add i32 %add11, %call7
  // CHECK:   %rem13 = urem i32 %add12, 5
  // CHECK:   %add.ptr14 = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem13
  x.y.z = dummies + ((GetValue() + n0 + n1 + n2 + n3 + n4) % LENGTH);

  // CHECK:   %.fca.0.insert = insertvalue %struct.ParentPtr poison, ptr addrspace(200) %add.ptr, 0
  // CHECK:   %.fca.1.0.insert = insertvalue %struct.ParentPtr %.fca.0.insert, ptr addrspace(200) %add.ptr14, 1, 0


  // CHECK:   ret %struct.ParentPtr %.fca.1.0.insert
  return x;
}

// One-sized checks.

struct OneInt {
  unsigned int x;
};

// CHECK: }

// CHECK: define dso_local chericcallcce i32 @_Z10InitOneIntv() local_unnamed_addr addrspace(200) #1 {
__attribute__((cheri_compartment("example"), noinline)) struct OneInt InitOneInt(void) {

  // CHECK: entry:
  // CHECK:   %call = tail call chericcallcce i32 @_Z8GetValuev()
  struct OneInt Res = {GetValue()};

  // CHECK:   ret i32 %call
  return Res;
}

// CHECK: define dso_local chericcallcce i32 @_Z9ChgOneInt6OneInt(i32 %x.coerce) local_unnamed_addr addrspace(200) #1 {
__attribute__((cheri_compartment("example"), noinline)) struct OneInt ChgOneInt(struct OneInt x) {
  // CHECK: entry:
  // CHECK:   %call = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %sub = sub i32 %x.coerce, %call
  x.x -= GetValue();

  // CHECK:   ret i32 %sub
  return x;
}


// Here we want to check that the initial struct received from the callee is handled as a proper struct, i.e. fields are read using `extractvalue` rather than dereferencing pointers.
// CHECK: define dso_local chericcallcce void @_Z11CheckOneIntv() local_unnamed_addr addrspace(200) #2 {
__attribute__((cheri_compartment("example"))) void CheckOneInt() {

  static struct OneInt __attribute__((used)) x = {}; 

  // CHECK: entry:
  // CHECK:   %call = tail call chericcallcce i32 @_Z10InitOneIntv()
  // CHECK:   %call2 = tail call chericcallcce i32 @_Z9ChgOneInt6OneInt(i32 %call)
  // CHECK:   store i32 %call2, ptr addrspace(200) @_Z11CheckOneIntv.x, align 4, !tbaa !7
  x = ChgOneInt(InitOneInt());
  
  // CHECK:   ret void
  return;
}

struct OnePtr {
  unsigned int* x;
};

// CHECK: define dso_local chericcallcce %struct.OnePtr @_Z10InitOnePtrv() local_unnamed_addr addrspace(200) #1 {
__attribute__((cheri_compartment("example"), noinline)) struct OnePtr InitOnePtr(void) {
  // CHECK: entry:
  // CHECK:   %call = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %rem = urem i32 %call, 5
  // CHECK:   %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem
  // CHECK:   %.fca.0.insert = insertvalue %struct.OnePtr poison, ptr addrspace(200) %add.ptr, 0
  struct OnePtr Res = {dummies + (GetValue() % LENGTH)};

  // CHECK:   ret %struct.OnePtr %.fca.0.insert
  return Res;
}

// CHECK: define dso_local chericcallcce %struct.OnePtr @_Z9ChgOnePtr6OnePtr(ptr addrspace(200) %x.coerce) local_unnamed_addr addrspace(200) #1 {
__attribute__((cheri_compartment("example"), noinline)) struct OnePtr ChgOnePtr(struct OnePtr x) {
  // CHECK: entry:
  // CHECK:   store ptr addrspace(200) %x.coerce, ptr addrspace(200) @force_use, align 8, !tbaa !11
  force_use = x.x;

  // CHECK:   %call = tail call chericcallcce i32 @_Z8GetValuev()
  // CHECK:   %rem = urem i32 %call, 5
  // CHECK:   %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem
  // CHECK:   %.fca.0.insert = insertvalue %struct.OnePtr poison, ptr addrspace(200) %add.ptr, 0
  x.x = dummies + (GetValue() % LENGTH);

  // CHECK:   ret %struct.OnePtr %.fca.0.insert
  return x;
}

// CHECK: define dso_local chericcallcce void @_Z11CheckOnePtrv() local_unnamed_addr addrspace(200) #2 {
__attribute__((cheri_compartment("example"))) void CheckOnePtr () {

  static struct OnePtr __attribute__((used)) x = {}; 

  // CHECK: entry:
  // CHECK:   %call = tail call chericcallcce %struct.OnePtr @_Z10InitOnePtrv()
  // CHECK:   %0 = extractvalue %struct.OnePtr %call, 0
  // CHECK:   %call1 = tail call chericcallcce %struct.OnePtr @_Z9ChgOnePtr6OnePtr(ptr addrspace(200) %0)
  // CHECK:   %1 = extractvalue %struct.OnePtr %call1, 0
  // CHECK:   store ptr addrspace(200) %1, ptr addrspace(200) @_Z11CheckOnePtrv.x, align 8, !tbaa !11
  x = ChgOnePtr(InitOnePtr());
  
  // CHECK:   ret void
  return;
}
