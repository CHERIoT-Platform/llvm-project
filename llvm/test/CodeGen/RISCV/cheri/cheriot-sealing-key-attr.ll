; RUN: llc -O0 --filetype=asm --mcpu=cheriot --mtriple=riscv32-unknown-unknown-cheriotrtos -target-abi cheriot -mattr=+xcheri,+cap-mode < %s | FileCheck %s

target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32-unknown-cheriotrtos-unknown"

%struct.OpaqueSealingKeyType = type opaque

@__import.sealing_type.static_sealing_test.StructSealingKey = external addrspace(200) constant %struct.OpaqueSealingKeyType #0
@__import.sealing_type.static_sealing_test.EnumSealingKey = external addrspace(200) constant %struct.OpaqueSealingKeyType #1
@__import.sealing_type.static_sealing_test.TypeDefSealingKey = external addrspace(200) constant %struct.OpaqueSealingKeyType #2
@__import.sealing_type.static_sealing_test.int = external addrspace(200) constant %struct.OpaqueSealingKeyType #3
;; Test that repeated sealing keys do not generate two separate entries.
@__import.sealing_type.static_sealing_test.int2 = external addrspace(200) constant %struct.OpaqueSealingKeyType #3


; Function Attrs: noinline nounwind optnone
define dso_local void @func() #4 {
entry:

  %SealingKey1 = alloca ptr, align 8, addrspace(200)
  %SealingKey2 = alloca ptr, align 8, addrspace(200)
  %SealingKey3 = alloca ptr, align 8, addrspace(200)
  %SealingKey4 = alloca ptr, align 8, addrspace(200)
  %SealingKey5 = alloca ptr, align 8, addrspace(200)
; CHECK: .LBB0_1:                                # %entry
; CHECK:                                         # Label of block must be emitted
; CHECK:         auipcc  ca0, %cheriot_compartment_hi(__import.sealing_type.static_sealing_test.StructSealingKey)
; CHECK:         clc     ca0, %cheriot_compartment_lo_i(.LBB0_1)(ca0)
  store ptr addrspace(200) @__import.sealing_type.static_sealing_test.StructSealingKey, ptr addrspace(200) %SealingKey1, align 8
; CHECK: .LBB0_2:                                # %entry
; CHECK:                                         # Label of block must be emitted
; CHECK:         auipcc  ca0, %cheriot_compartment_hi(__import.sealing_type.static_sealing_test.EnumSealingKey)
; CHECK:         clc     ca0, %cheriot_compartment_lo_i(.LBB0_2)(ca0)
  store ptr addrspace(200) @__import.sealing_type.static_sealing_test.EnumSealingKey, ptr addrspace(200) %SealingKey2, align 8
; CHECK: .LBB0_3:                                # %entry
; CHECK:                                         # Label of block must be emitted
; CHECK:         auipcc  ca0, %cheriot_compartment_hi(__import.sealing_type.static_sealing_test.TypeDefSealingKey)
; CHECK:         clc     ca0, %cheriot_compartment_lo_i(.LBB0_3)(ca0)
  store ptr addrspace(200) @__import.sealing_type.static_sealing_test.TypeDefSealingKey, ptr addrspace(200) %SealingKey3, align 8
; CHECK: .LBB0_4:                                # %entry
; CHECK:                                         # Label of block must be emitted
; CHECK:         auipcc  ca0, %cheriot_compartment_hi(__import.sealing_type.static_sealing_test.int)
; CHECK:         clc     ca0, %cheriot_compartment_lo_i(.LBB0_4)(ca0)
  store ptr addrspace(200) @__import.sealing_type.static_sealing_test.int, ptr addrspace(200) %SealingKey4, align 8
; CHECK: .LBB0_5:                                # %entry
; CHECK:                                         # Label of block must be emitted
; CHECK:         auipcc  ca0, %cheriot_compartment_hi(__import.sealing_type.static_sealing_test.int)
; CHECK:         clc     ca0, %cheriot_compartment_lo_i(.LBB0_5)(ca0)
  store ptr addrspace(200) @__import.sealing_type.static_sealing_test.int2, ptr addrspace(200) %SealingKey5, align 8
; CHECK: 	cret
; CHECK: .Lfunc_end0:
; CHECK: 	.size	func, .Lfunc_end0-func
  ret void
}

attributes #0 = { "cheri-compartment"="static_sealing_test" "cheriot_sealing_key"="sealing_type.static_sealing_test.StructSealingKey" }
attributes #1 = { "cheri-compartment"="static_sealing_test" "cheriot_sealing_key"="sealing_type.static_sealing_test.EnumSealingKey" }
attributes #2 = { "cheri-compartment"="static_sealing_test" "cheriot_sealing_key"="sealing_type.static_sealing_test.TypeDefSealingKey" }
attributes #3 = { "cheri-compartment"="static_sealing_test" "cheriot_sealing_key"="sealing_type.static_sealing_test.int" }
attributes #4 = { noinline nounwind optnone "cheri-compartment"="test" "frame-pointer"="none" "interrupt-state"="enabled" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+relax,+xcheri,+xcheri-rvc,-64bit,-save-restore" }

; CHECK: __export.sealing_type.static_sealing_test.StructSealingKey:
; CHECK: 	.half	0
; CHECK: 	.byte	0
; CHECK: 	.byte	32
; CHECK: 	.size	__export.sealing_type.static_sealing_test.StructSealingKey, 4
; CHECK: 	.section	.compartment_exports.sealing_type.static_sealing_test.EnumSealingKey,"awG",@progbits,sealing_type.static_sealing_test.EnumSealingKey,comdat
; CHECK: 	.type	__export.sealing_type.static_sealing_test.EnumSealingKey,@object
; CHECK: 	.globl	__export.sealing_type.static_sealing_test.EnumSealingKey
; CHECK: 	.p2align	2, 0x0
; CHECK: __export.sealing_type.static_sealing_test.EnumSealingKey:
; CHECK: 	.half	0
; CHECK: 	.byte	0
; CHECK: 	.byte	32
; CHECK: 	.size	__export.sealing_type.static_sealing_test.EnumSealingKey, 4
; CHECK: 	.section	.compartment_exports.sealing_type.static_sealing_test.TypeDefSealingKey,"awG",@progbits,sealing_type.static_sealing_test.TypeDefSealingKey,comdat
; CHECK: 	.type	__export.sealing_type.static_sealing_test.TypeDefSealingKey,@object
; CHECK: 	.globl	__export.sealing_type.static_sealing_test.TypeDefSealingKey
; CHECK: 	.p2align	2, 0x0
; CHECK: __export.sealing_type.static_sealing_test.TypeDefSealingKey:
; CHECK: 	.half	0
; CHECK: 	.byte	0
; CHECK: 	.byte	32
; CHECK: 	.size	__export.sealing_type.static_sealing_test.TypeDefSealingKey, 4
; CHECK: 	.section	.compartment_exports.sealing_type.static_sealing_test.int,"awG",@progbits,sealing_type.static_sealing_test.int,comdat
; CHECK: 	.type	__export.sealing_type.static_sealing_test.int,@object
; CHECK: 	.globl	__export.sealing_type.static_sealing_test.int
; CHECK: 	.p2align	2, 0x0
; CHECK: __export.sealing_type.static_sealing_test.int:
; CHECK: 	.half	0
; CHECK: 	.byte	0
; CHECK: 	.byte	32
; CHECK: 	.size	__export.sealing_type.static_sealing_test.int, 4
; CHECK: 	.section	.compartment_imports.sealing_type.static_sealing_test.StructSealingKey,"awG",@progbits,__import.sealing_type.static_sealing_test.StructSealingKey,comdat
; CHECK: 	.type	__import.sealing_type.static_sealing_test.StructSealingKey,@object
; CHECK: 	.globl	__import.sealing_type.static_sealing_test.StructSealingKey
; CHECK: 	.p2align	3, 0x0
; CHECK: __import.sealing_type.static_sealing_test.StructSealingKey:
; CHECK: 	.word	__export.sealing_type.static_sealing_test.StructSealingKey
; CHECK: 	.word	0
; CHECK: 	.size	__import.sealing_type.static_sealing_test.StructSealingKey, 8
; CHECK: 	.section	.compartment_imports.sealing_type.static_sealing_test.EnumSealingKey,"awG",@progbits,__import.sealing_type.static_sealing_test.EnumSealingKey,comdat
; CHECK: 	.type	__import.sealing_type.static_sealing_test.EnumSealingKey,@object
; CHECK: 	.globl	__import.sealing_type.static_sealing_test.EnumSealingKey
; CHECK: 	.p2align	3, 0x0
; CHECK: __import.sealing_type.static_sealing_test.EnumSealingKey:
; CHECK: 	.word	__export.sealing_type.static_sealing_test.EnumSealingKey
; CHECK: 	.word	0
; CHECK: 	.size	__import.sealing_type.static_sealing_test.EnumSealingKey, 8
; CHECK: 	.section	.compartment_imports.sealing_type.static_sealing_test.TypeDefSealingKey,"awG",@progbits,__import.sealing_type.static_sealing_test.TypeDefSealingKey,comdat
; CHECK: 	.type	__import.sealing_type.static_sealing_test.TypeDefSealingKey,@object
; CHECK: 	.globl	__import.sealing_type.static_sealing_test.TypeDefSealingKey
; CHECK: 	.p2align	3, 0x0
; CHECK: __import.sealing_type.static_sealing_test.TypeDefSealingKey:
; CHECK: 	.word	__export.sealing_type.static_sealing_test.TypeDefSealingKey
; CHECK: 	.word	0
; CHECK: 	.size	__import.sealing_type.static_sealing_test.TypeDefSealingKey, 8
; CHECK: 	.section	.compartment_imports.sealing_type.static_sealing_test.int,"awG",@progbits,__import.sealing_type.static_sealing_test.int,comdat
; CHECK: 	.type	__import.sealing_type.static_sealing_test.int,@object
; CHECK: 	.globl	__import.sealing_type.static_sealing_test.int
; CHECK: 	.p2align	3, 0x0
; CHECK: __import.sealing_type.static_sealing_test.int:
; CHECK: 	.word	__export.sealing_type.static_sealing_test.int
; CHECK: 	.word	0
; CHECK: 	.size	__import.sealing_type.static_sealing_test.int, 8


!0 = !{i32 1, !"wchar_size", i32 2}
!1 = !{i32 1, !"target-abi", !"cheriot"}
!2 = !{i32 6, !"riscv-isa", !3}
!3 = !{!"rv32e2p0_m2p0_c2p0_zmmul1p0_xcheri0p0_xcheriot1p0"}
!4 = !{i32 8, !"SmallDataLimit", i32 0}
