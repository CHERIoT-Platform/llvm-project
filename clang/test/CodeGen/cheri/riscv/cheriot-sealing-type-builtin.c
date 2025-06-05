// RUN: %clang_cc1 %s -o - "-triple" "riscv32cheriot-unknown-unknown-cheriotrtos" "-emit-llvm" "-cheri-compartment=static_sealing_test" "-mframe-pointer=none" "-mcmodel=small" "-target-abi" "cheriot" "-O0" "-Werror" -std=c2x | FileCheck %s


struct StructSealingKey { };
enum EnumSealingKey { SEALING_KEY_KIND1, SEALING_KEY_KIND2 };
typedef enum EnumSealingKey TypeDefSealingKey;

// CHECK: %struct.__CHERIoT__OpaqueSealingKeyType = type opaque
// CHECK: @__import.sealing_type.static_sealing_test.StructSealingKey = external addrspace(200) constant %struct.__CHERIoT__OpaqueSealingKeyType #0
// CHECK: @__import.sealing_type.static_sealing_test.EnumSealingKey = external addrspace(200) constant %struct.__CHERIoT__OpaqueSealingKeyType #1
// CHECK: @__import.sealing_type.static_sealing_test.TypeDefSealingKey = external addrspace(200) constant %struct.__CHERIoT__OpaqueSealingKeyType #2
// CHECK: @__import.sealing_type.static_sealing_test.int = external addrspace(200) constant %struct.__CHERIoT__OpaqueSealingKeyType #3

// CHECK: define dso_local void @func() addrspace(200) #4 {
void func() {

// CHECK: entry:
// CHECK:  %SealingKey1 = alloca ptr addrspace(200), align 8, addrspace(200)
// CHECK:  %SealingKey2 = alloca ptr addrspace(200), align 8, addrspace(200)
// CHECK:  %SealingKey3 = alloca ptr addrspace(200), align 8, addrspace(200)
// CHECK:  %SealingKey4 = alloca ptr addrspace(200), align 8, addrspace(200)
// CHECK:  %SealingKey5 = alloca ptr addrspace(200), align 8, addrspace(200)
// CHECK:  %SealingKey6 = alloca ptr addrspace(200), align 8, addrspace(200)
// CHECK:  %SealingKey7 = alloca ptr addrspace(200), align 8, addrspace(200)

// CHECK:  store ptr addrspace(200) @__import.sealing_type.static_sealing_test.StructSealingKey, ptr addrspace(200) %SealingKey1, align 8
  struct StructSealingKey *SealingKey1 = __builtin_cheriot_sealing_type("StructSealingKey");

// CHECK:  store ptr addrspace(200) @__import.sealing_type.static_sealing_test.EnumSealingKey, ptr addrspace(200) %SealingKey2, align 8
  enum EnumSealingKey *SealingKey2 = __builtin_cheriot_sealing_type("EnumSealingKey");

// CHECK:  store ptr addrspace(200) @__import.sealing_type.static_sealing_test.TypeDefSealingKey, ptr addrspace(200) %SealingKey3, align 8
  TypeDefSealingKey *SealingKey3 = __builtin_cheriot_sealing_type("TypeDefSealingKey");

// CHECK:  store ptr addrspace(200) @__import.sealing_type.static_sealing_test.int, ptr addrspace(200) %SealingKey4, align 8
  int *SealingKey4 = __builtin_cheriot_sealing_type("int");

// CHECK:  store ptr addrspace(200) @__import.sealing_type.static_sealing_test.int, ptr addrspace(200) %SealingKey5, align 8
  int *SealingKey5 = __builtin_cheriot_sealing_type("int");
// CHECK:  store ptr addrspace(200) @__import.sealing_type.static_sealing_test.int, ptr addrspace(200) %SealingKey6, align 8
  int *SealingKey6 = __builtin_cheriot_sealing_type("int");
// CHECK:  store ptr addrspace(200) @__import.sealing_type.static_sealing_test.int, ptr addrspace(200) %SealingKey7, align 8
  int *SealingKey7 = __builtin_cheriot_sealing_type("int");

// CHECK: ret void
}

// CHECK: attributes #0 = { "cheri-compartment"="static_sealing_test" "cheriot_sealing_key"="sealing_type.static_sealing_test.StructSealingKey" }
// CHECK: attributes #1 = { "cheri-compartment"="static_sealing_test" "cheriot_sealing_key"="sealing_type.static_sealing_test.EnumSealingKey" }
// CHECK: attributes #2 = { "cheri-compartment"="static_sealing_test" "cheriot_sealing_key"="sealing_type.static_sealing_test.TypeDefSealingKey" }
// CHECK: attributes #3 = { "cheri-compartment"="static_sealing_test" "cheriot_sealing_key"="sealing_type.static_sealing_test.int" }
