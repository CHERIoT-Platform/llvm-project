; DO NOT EDIT -- This file was generated from test/CodeGen/CHERI-Generic/Inputs/csetbounds-stats-alloc_size.ll
; RUN: rm -f %t.csv
; RUN: llc -mtriple=riscv64 --relocation-model=pic -target-abi l64pc128d -mattr=+xcheri,+xcheripurecap,+f,+d %s -O0 -o /dev/null \
; RUN:    -collect-csetbounds-output=%t.csv -collect-csetbounds-stats=csv
; RUN: FileCheck %s -input-file=%t.csv -check-prefix CSV

declare ptr addrspace(200) @do_alloc(i32) addrspace(200) allocsize(0)
declare ptr addrspace(200) @do_alloc_callsite_annotated(i32, i32) addrspace(200)
@alloc_fn_ptr = addrspace(200) global ptr addrspace(200) null, align 16

define ptr addrspace(200) @test_direct_call_1() addrspace(200) {
entry:
  %result = call dereferenceable_or_null(100) ptr addrspace(200) @do_alloc(i32 100)
  ret ptr addrspace(200) %result
}

;; Check that an annotated callsite also works if the function doesn't have it
define ptr addrspace(200) @test_direct_call_2() addrspace(200) {
entry:
  %result = call ptr addrspace(200) @do_alloc_callsite_annotated(i32 0, i32 200) allocsize(1)
  ret ptr addrspace(200) %result
}

; Same for function pointers (see also https://reviews.llvm.org/D55212
; and https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88372)
define ptr addrspace(200) @test_indirect_call() addrspace(200) {
entry:
  %0 = load ptr addrspace(200), ptr addrspace(200) @alloc_fn_ptr, align 16
  %result = call ptr addrspace(200) %0(i32 1, i32 8, i32 4) allocsize(1,2)
  ret ptr addrspace(200) %result
}

; CSV-LABEL: alignment_bits,size,kind,source_loc,compiler_pass,details
; CSV-NEXT: 0,100,h,"<somewhere in test_direct_call_1>","function with alloc_size","call to do_alloc"
; CSV-NEXT: 0,200,h,"<somewhere in test_direct_call_2>","function with alloc_size","call to do_alloc_callsite_annotated"
; CSV-NEXT: 0,32,h,"<somewhere in test_indirect_call>","function with alloc_size","call to function pointer"
