; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=PURECAP
; we should really be getting an error when compiling this with the hybrid ABI (alloca in AS 200)
; RUNTODO: not llc @HYBRID_HARDFLOAT_ARGS@ < %s 2>&1 | FileCheck %s -check-prefix BAD-ABI
; BAD-ABI: error: abc

%struct.Dwarf_Error = type { [1024 x i32] }

@a = common local_unnamed_addr addrspace(200) global %struct.Dwarf_Error zeroinitializer, align 4

; Function Attrs: nounwind
define i32 @fn1() local_unnamed_addr #0 {
entry:
  %tmp = alloca %struct.Dwarf_Error, align 8, addrspace(200)
  %0 = bitcast ptr addrspace(200) %tmp to ptr addrspace(200)
  call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) nonnull align 4 %0, ptr addrspace(200) align 4 @a, i64 4096, i1 false)
  %call = call i32 (...) @fn2(ptr addrspace(200) nonnull byval(%struct.Dwarf_Error) align 8 %tmp) #0
  ret i32 undef
}

; Function Attrs: nounwind
declare i32 @fn2(...) #0

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) noalias nocapture writeonly, ptr addrspace(200) noalias nocapture readonly, i64, i1 immarg) #1

attributes #0 = { nounwind }
attributes #1 = { nocallback nofree nounwind willreturn memory(argmem: readwrite) }
