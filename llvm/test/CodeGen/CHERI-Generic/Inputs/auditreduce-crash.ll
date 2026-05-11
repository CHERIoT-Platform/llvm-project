; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -verify-machineinstrs < %s | FileCheck %s --check-prefix=PURECAP
;; Hybrid baseline to compare against
; RUN: sed 's/addrspace(200)//g' %s | llc @HYBRID_HARDFLOAT_ARGS@ -verify-machineinstrs | FileCheck %s --check-prefix=HYBRID

%struct.au_mask = type {}
%struct.tokenstr = type { i8, ptr, i64, %union.anon }
%union.anon = type { %struct.au_execarg_t }
%struct.au_execarg_t = type { i32, [128 x ptr addrspace(200)] }

@maskp = external addrspace(200) global %struct.au_mask

define i32 @select_hdr32(ptr addrspace(200) byval(%struct.tokenstr) %0, ptr addrspace(200) %optchkd) nounwind {
entry:
  %1 = getelementptr %struct.tokenstr, ptr addrspace(200) %0, i64 0, i32 0
  %tok.sroa.1.0..sroa_idx = getelementptr i8, ptr addrspace(200) %1
  %tok.sroa.1.0..sroa_cast = bitcast ptr addrspace(200) %tok.sroa.1.0..sroa_idx to ptr addrspace(200)
  %tok.sroa.1.0.copyload = load i16, ptr addrspace(200) %tok.sroa.1.0..sroa_cast, align 2
  %call26 = call i32 @au_preselect(i16 %tok.sroa.1.0.copyload, ptr addrspace(200) @maskp, i32 3, i32 0)
  ret i32 0
}

define i32 @foo(ptr addrspace(200) byval(i512) %x, ptr addrspace(200) byval(%struct.tokenstr) %0, ptr addrspace(200) %optchkd) nounwind {
entry:
  %1 = getelementptr %struct.tokenstr, ptr addrspace(200) %0, i64 0, i32 0
  %tok.sroa.1.0..sroa_idx = getelementptr i8, ptr addrspace(200) %1
  %tok.sroa.1.0..sroa_cast = bitcast ptr addrspace(200) %tok.sroa.1.0..sroa_idx to ptr addrspace(200)
  %tok.sroa.1.0.copyload = load i16, ptr addrspace(200) %tok.sroa.1.0..sroa_cast, align 2
  %call26 = call i32 @au_preselect(i16 %tok.sroa.1.0.copyload, ptr addrspace(200) @maskp, i32 3, i32 0)
  ret i32 0
}

declare i32 @au_preselect(i16 signext, ptr addrspace(200), i32, i32)
