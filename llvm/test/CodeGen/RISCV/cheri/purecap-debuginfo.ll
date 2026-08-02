; Check that the CheriBoundAllocas pass doesn't break debug info.
; It previously moved all the llvm.dbg.declare statements from the alloca
; to the csetbounds which caused the processDbgDeclares() function in
; SelectionDAGISel.cpp to not insert the appropriate local variable entries.
; The allocas for "j"/"j1" escape (passed to @escape()) so that
; CheriBoundAllocas actually bounds them (csetbounds is emitted); the
; DW_AT_location entries must survive that bounding.

; RUN: %riscv64_cheri_purecap_llc %s -O0 -filetype=obj -o - | \
; RUN:   llvm-dwarfdump -debug-info - | FileCheck %s

; CHECK-LABEL: .debug_info contents:
; CHECK: 0x00000000: Compile Unit:
; CHECK-EMPTY:
; CHECK-NEXT: DW_TAG_compile_unit
; CHECK-NEXT:               DW_AT_producer	("clang version
; CHECK-NEXT:               DW_AT_language	(DW_LANG_C99)
; CHECK-NEXT:               DW_AT_name	("
; CHECK-NEXT:               DW_AT_stmt_list	(0x00000000)
; CHECK-NEXT:               DW_AT_comp_dir	("
; CHECK-NEXT:               DW_AT_GNU_pubnames	(0x01)
; CHECK-NEXT:               DW_AT_low_pc	(0x0000000000000000)
; CHECK-NEXT:               DW_AT_high_pc	(0x00000000000{{.+}})
; CHECK-EMPTY:
; CHECK-NEXT: DW_TAG_subprogram
; CHECK-NEXT:                 DW_AT_low_pc	(0x0000000000000000)
; CHECK-NEXT:                 DW_AT_high_pc	(0x00000000000000{{.+}})
; CHECK-NEXT:                 DW_AT_frame_base	(DW_OP_reg2 X2)
; CHECK-NEXT:                 DW_AT_name	("foo")
; CHECK-NEXT:                 DW_AT_decl_file	("/src/test/CodeGen/cheri/cheri-debug-info.c")
; CHECK-NEXT:                 DW_AT_decl_line	(20)
; CHECK-NEXT:                 DW_AT_prototyped	(0x01)
; CHECK-NEXT:                 DW_AT_type	([[INT_TYPE_INFO_ADDR:.*]] "int")
; CHECK-NEXT:                 DW_AT_external	(0x01)
; CHECK-EMPTY:
; CHECK-NEXT: DW_TAG_formal_parameter
; CHECK-NEXT:                   DW_AT_location	(DW_OP_fbreg +{{[0-9]+}})
; CHECK-NEXT:                   DW_AT_name	("i")
; CHECK-NEXT:                   DW_AT_decl_file	("/src/test/CodeGen/cheri/cheri-debug-info.c")
; CHECK-NEXT:                   DW_AT_decl_line	(20)
; CHECK-NEXT:                   DW_AT_type	([[INTPTR_TYPE_INFO_ADDR:0x000.+]] "int *")
; CHECK-EMPTY:
; CHECK-NEXT: DW_TAG_lexical_block
; CHECK-NEXT:                  DW_AT_low_pc	({{.+}})
; CHECK-NEXT:                  DW_AT_high_pc	({{.+}})
; CHECK-EMPTY:
; CHECK-NEXT: DW_TAG_variable
; CHECK-NEXT:                     DW_AT_location	(DW_OP_fbreg +{{[0-9]+}})
; CHECK-NEXT:                     DW_AT_name	("j")
; CHECK-NEXT:                     DW_AT_decl_file	("/src/test/CodeGen/cheri/cheri-debug-info.c")
; CHECK-NEXT:                     DW_AT_decl_line	(22)
; CHECK-NEXT:                     DW_AT_type	([[INT_TYPE_INFO_ADDR]] "int")
; CHECK-EMPTY:
; CHECK-NEXT:  NULL
; CHECK-EMPTY:
; CHECK-NEXT:  DW_TAG_lexical_block
; CHECK-NEXT:                  DW_AT_low_pc	({{.+}})
; CHECK-NEXT:                  DW_AT_high_pc	({{.+}})
; CHECK-EMPTY:
; CHECK-NEXT:  DW_TAG_variable
; CHECK-NEXT:                     DW_AT_location	(DW_OP_fbreg +{{[0-9]+}})
; CHECK-NEXT:                     DW_AT_name	("j")
; CHECK-NEXT:                     DW_AT_decl_file	("{{.+}}/CodeGen/cheri/cheri-debug-info.c")
; CHECK-NEXT:                     DW_AT_decl_line	(25)
; CHECK-NEXT:                     DW_AT_type	([[INT_TYPE_INFO_ADDR]] "int")
; CHECK-EMPTY:
; CHECK-NEXT:   NULL
; CHECK-EMPTY:
; CHECK-NEXT:   NULL
; CHECK-EMPTY:
; CHECK-NEXT: [[INT_TYPE_INFO_ADDR]]: DW_TAG_base_type
; CHECK-NEXT:                 DW_AT_name	("int")
; CHECK-NEXT:                 DW_AT_encoding	(DW_ATE_signed)
; CHECK-NEXT:                 DW_AT_byte_size	(0x04)
; CHECK-EMPTY:
; CHECK-NEXT:  [[INTPTR_TYPE_INFO_ADDR]]:  DW_TAG_pointer_type
; CHECK-NEXT:                 DW_AT_type	([[INT_TYPE_INFO_ADDR]] "int")
; The capability pointer type includes the (non-default) size:
; CHECK-NEXT:                 DW_AT_byte_size ({{0x10|0x20}})
; CHECK-EMPTY:
; CHECK-NEXT:   NULL

target datalayout = "e-m:e-pf200:128:128:128:64-p:64:64-i64:64-i128:128-n32:64-S128-A200-P200-G200"
target triple = "riscv64-unknown-freebsd"

; Function Attrs: noinline nounwind optnone
define i32 @foo(i32 addrspace(200)* %i) addrspace(200) #0 !dbg !9 {
entry:
  %i.addr = alloca i32 addrspace(200)*, align 16, addrspace(200)
  %j = alloca i32, align 4, addrspace(200)
  %j1 = alloca i32, align 4, addrspace(200)
  store i32 addrspace(200)* %i, i32 addrspace(200)* addrspace(200)* %i.addr, align 16
  call void @llvm.dbg.declare(metadata i32 addrspace(200)* addrspace(200)* %i.addr, metadata !14, metadata !DIExpression()), !dbg !15
  %0 = load i32 addrspace(200)*, i32 addrspace(200)* addrspace(200)* %i.addr, align 16, !dbg !16
  %tobool = icmp ne i32 addrspace(200)* %0, null, !dbg !16
  br i1 %tobool, label %if.then, label %if.else, !dbg !18

if.then:                                          ; preds = %entry
  call void @llvm.dbg.declare(metadata i32 addrspace(200)* %j, metadata !19, metadata !DIExpression()), !dbg !21
  store i32 2, i32 addrspace(200)* %j, align 4, !dbg !21
  call void @escape(i32 addrspace(200)* %j), !dbg !21
  br label %if.end, !dbg !22

if.else:                                          ; preds = %entry
  call void @llvm.dbg.declare(metadata i32 addrspace(200)* %j1, metadata !23, metadata !DIExpression()), !dbg !25
  store i32 3, i32 addrspace(200)* %j1, align 4, !dbg !25
  call void @escape(i32 addrspace(200)* %j1), !dbg !25
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %1 = load i32 addrspace(200)*, i32 addrspace(200)* addrspace(200)* %i.addr, align 16, !dbg !26
  %2 = bitcast i32 addrspace(200)* %1 to i8 addrspace(200)*, !dbg !27
  %3 = call i64 @llvm.cheri.cap.address.get.i64(i8 addrspace(200)* %2), !dbg !27
  %4 = trunc i64 %3 to i32, !dbg !27
  ret i32 %4, !dbg !28
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) addrspace(200) #1

; Function Attrs: nounwind readnone
declare i64 @llvm.cheri.cap.address.get.i64(i8 addrspace(200)*) addrspace(200) #2

declare void @escape(i32 addrspace(200)*) addrspace(200)

attributes #0 = { noinline nounwind optnone }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { nounwind readnone }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!5, !6, !7}
!llvm.ident = !{!8}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 7.0.0 (https://github.com/llvm-mirror/clang.git 3b79a88ce02dcb2e8ed16e813156f2d41cac7a83) (https://github.com/llvm-mirror/llvm.git f721a1b6115a98a0cde6f8714dd405b8273c34a9)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, retainedTypes: !3)
!1 = !DIFile(filename: "/Users/alex/cheri/llvm/tools/clang/test/CodeGen/cheri/<stdin>", directory: "/Users/alex/cheri/ctsrd-svn/cheritest")
!2 = !{}
!3 = !{!4}
!4 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!5 = !{i32 2, !"Dwarf Version", i32 2}
!6 = !{i32 2, !"Debug Info Version", i32 3}
!7 = !{i32 1, !"wchar_size", i32 4}
!8 = !{!"clang version 7.0.0 (https://github.com/llvm-mirror/clang.git 3b79a88ce02dcb2e8ed16e813156f2d41cac7a83) (https://github.com/llvm-mirror/llvm.git f721a1b6115a98a0cde6f8714dd405b8273c34a9)"}
!9 = distinct !DISubprogram(name: "foo", scope: !10, file: !10, line: 20, type: !11, isLocal: false, isDefinition: true, scopeLine: 20, flags: DIFlagPrototyped, isOptimized: false, unit: !0, retainedNodes: !2)
!10 = !DIFile(filename: "/src/test/CodeGen/cheri/cheri-debug-info.c", directory: "/src")
!11 = !DISubroutineType(types: !12)
!12 = !{!4, !13}
!13 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !4, size: 128)
!14 = !DILocalVariable(name: "i", arg: 1, scope: !9, file: !10, line: 20, type: !13)
!15 = !DILocation(line: 20, column: 14, scope: !9)
!16 = !DILocation(line: 21, column: 6, scope: !17)
!17 = distinct !DILexicalBlock(scope: !9, file: !10, line: 21, column: 6)
!18 = !DILocation(line: 21, column: 6, scope: !9)
!19 = !DILocalVariable(name: "j", scope: !20, file: !10, line: 22, type: !4)
!20 = distinct !DILexicalBlock(scope: !17, file: !10, line: 21, column: 9)
!21 = !DILocation(line: 22, column: 7, scope: !20)
!22 = !DILocation(line: 23, column: 2, scope: !20)
!23 = !DILocalVariable(name: "j", scope: !24, file: !10, line: 25, type: !4)
!24 = distinct !DILexicalBlock(scope: !17, file: !10, line: 24, column: 7)
!25 = !DILocation(line: 25, column: 7, scope: !24)
!26 = !DILocation(line: 27, column: 14, scope: !9)
!27 = !DILocation(line: 27, column: 9, scope: !9)
!28 = !DILocation(line: 27, column: 2, scope: !9)
