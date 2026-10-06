; RUN: llc --filetype=asm --mcpu=cheriot --mtriple=riscv32-unknown-unknown -target-abi cheriot  %s -mattr=+xcheri,+xcheripurecap -o - | FileCheck %s
target datalayout = "e-m:e-p:32:32-pfe200:64:64:64:32-i64:64-n32-S128-A200-P200-G200"
target triple = "riscv32-unknown-cheriotrtos"

@global = external addrspace(200) constant [3 x i8]
@global.1 = external addrspace(200) constant [9 x i8]

define cheriot_compartmentcalleecc i32 @widget(i1 %arg, i1 %arg1, i1 %arg2, i1 %arg3, i1 %arg4, i1 %arg5, ptr addrspace(200) %arg6, ptr addrspace(200) %arg7) addrspace(200) #0 {
; CHECK-LABEL: widget:
; CHECK-NOT:    mov t2, t0
; CHECK-NOT:    mov t0, t2
bb:
  %call = call zeroext addrspace(200) i1 null(ptr addrspace(200) null, i8 0)
  %select = select i1 %arg, ptr addrspace(200) %arg7, ptr addrspace(200) null
  %select8 = select i1 %arg1, ptr addrspace(200) @global, ptr addrspace(200) @global.1
  %select9 = select i1 %arg, ptr addrspace(200) @global, ptr addrspace(200) null
  %select10 = select i1 %arg, ptr addrspace(200) null, ptr addrspace(200) @global.1
  %select11 = select i1 %call, ptr addrspace(200) @global, ptr addrspace(200) null
  call addrspace(200) void (ptr addrspace(200), ...) null(ptr addrspace(200) null, i32 0, ptr addrspace(200) %select, ptr addrspace(200) %select8, ptr addrspace(200) %select9, ptr addrspace(200) %select10, ptr addrspace(200) %select11)
  br i1 %arg, label %bb12, label %bb17

bb12:
  %call13 = call addrspace(200) i1 null(ptr addrspace(200) null, i8 0)
  %select14 = select i1 %arg5, ptr addrspace(200) %arg6, ptr addrspace(200) null
  %select15 = select i1 %arg, ptr addrspace(200) @global, ptr addrspace(200) @global.1
  %select16 = select i1 %arg5, ptr addrspace(200) @global, ptr addrspace(200) null
  call addrspace(200) void (ptr addrspace(200), ...) null(ptr addrspace(200) null, ptr addrspace(200) %select14, ptr addrspace(200) null, ptr addrspace(200) @global.1, ptr addrspace(200) %select15, ptr addrspace(200) %select16)
  ret i32 0

bb17:
  ret i32 0
}

attributes #0 = { minsize }
