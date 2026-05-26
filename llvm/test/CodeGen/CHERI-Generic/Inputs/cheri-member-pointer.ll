; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -asm-verbose -verify-regalloc -O0 %s -o - | FileCheck %s
; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -asm-verbose -verify-regalloc -O1 %s -o - | FileCheck %s --check-prefix=OPT
; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -asm-verbose -verify-regalloc -O2 %s -o - | FileCheck %s --check-prefix=OPT

@global_func_ptr = external local_unnamed_addr addrspace(200) global ptr addrspace(200), align 32

; Function Attrs: nounwind
define i32 @func_ptr_dereference(ptr addrspace(200) %a, ptr addrspace(200) inreg %ptr.coerce0, iCAPRANGE inreg %ptr.coerce1) local_unnamed_addr addrspace(200) #0 {
entry:
  %memptr.adj.shifted = ashr iCAPRANGE %ptr.coerce1, 1
  %this.not.adjusted = bitcast ptr addrspace(200) %a to ptr addrspace(200)
  %memptr.vtable.addr = getelementptr inbounds i8, ptr addrspace(200) %this.not.adjusted, iCAPRANGE %memptr.adj.shifted
  %this.adjusted = bitcast ptr addrspace(200) %memptr.vtable.addr to ptr addrspace(200)
  %0 = and iCAPRANGE %ptr.coerce1, 1
  %memptr.isvirtual = icmp eq iCAPRANGE %0, 0
  br i1 %memptr.isvirtual, label %memptr.nonvirtual, label %memptr.virtual

memptr.virtual:                                   ; preds = %entry
  %1 = bitcast ptr addrspace(200) %memptr.vtable.addr to ptr addrspace(200)
  %vtable = load ptr addrspace(200), ptr addrspace(200) %1, align 32, !tbaa !2
  %memptr.vtable.offset = ptrtoint ptr addrspace(200) %ptr.coerce0 to iCAPRANGE
  %2 = getelementptr i8, ptr addrspace(200) %vtable, iCAPRANGE %memptr.vtable.offset
  %3 = bitcast ptr addrspace(200) %2 to ptr addrspace(200)
  %memptr.virtualfn = load ptr addrspace(200), ptr addrspace(200) %3, align 32
  br label %memptr.end

memptr.nonvirtual:                                ; preds = %entry
  %memptr.nonvirtualfn = bitcast ptr addrspace(200) %ptr.coerce0 to ptr addrspace(200)
  br label %memptr.end

memptr.end:                                       ; preds = %memptr.nonvirtual, %memptr.virtual
  %4 = phi ptr addrspace(200) [ %memptr.virtualfn, %memptr.virtual ], [ %memptr.nonvirtualfn, %memptr.nonvirtual ]
  %call = tail call i32 %4(ptr addrspace(200) %this.adjusted) #0
  ret i32 %call
}

attributes #0 = { nounwind }

!llvm.module.flags = !{!0}

!0 = !{i32 1, !"PIC Level", i32 2}
!2 = !{!3, !3, i64 0}
!3 = !{!"vtable pointer", !4, i64 0}
!4 = !{!"Simple C++ TBAA"}
