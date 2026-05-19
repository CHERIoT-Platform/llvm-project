; RUN: llc @PURECAP_HARDFLOAT_ARGS@ %s -o - | FileCheck %s --check-prefix=PURECAP
; Test that we can correctly legalise i128 and generate pointer arithmetic that
; doesn't crash the compiler.

%struct.foo = type { i128 }

@x = internal addrspace(200) global %struct.foo zeroinitializer, align 8

; Function Attrs: noinline nounwind
define i32 @main(i32 signext %argc, ptr addrspace(200) %argv) #0 {
entry:
  %retval = alloca i32, align 4, addrspace(200)
  %argc.addr = alloca i32, align 4, addrspace(200)
  %argv.addr = alloca ptr addrspace(200), align 32, addrspace(200)
  store i32 0, ptr addrspace(200) %retval, align 4
  store i32 %argc, ptr addrspace(200) %argc.addr, align 4
  store ptr addrspace(200) %argv, ptr addrspace(200) %argv.addr, align 32
  %bf.load = load i128, ptr addrspace(200) @x, align 4
  %bf.clear = and i128 %bf.load, -4294967295
  %bf.set = or i128 %bf.clear, 2
  store i128 %bf.set, ptr addrspace(200) @x, align 4
  ret i32 0
}

attributes #0 = { noinline nounwind }

!llvm.module.flags = !{!0}

!0 = !{i32 8, !"PIC Level", i32 2}
