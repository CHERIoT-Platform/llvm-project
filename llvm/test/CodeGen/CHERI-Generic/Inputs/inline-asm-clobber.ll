; CHERI-GENERIC-UTC: mir
; CHERI-GENERIC-UTC: llc
; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -verify-machineinstrs -o - -O0 %s -stop-after=instruction-select | FileCheck %s -check-prefix MIR
; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -verify-machineinstrs -o - -O3 %s | FileCheck %s -enable-var-scope
;
; https://github.com/CTSRD-CHERI/llvm/issues/274
;
; Source code:
; int test(int intval, const char* ptrval) {
;   __asm__ ("syscall\n"
;   :
;   :
;   : "a0", "$c3");
;   char x = *((volatile char*)ptrval);
;   return (int)x + intval;
; }
;
; The capability clobber registers were previously off by one: -> $c3 would save $c4, etc.

define i32 @test_clobber_c2(i32 signext %intval, ptr addrspace(200) %ptrval, ptr addrspace(200) %ptrval2) local_unnamed_addr {
entry:
@IF-MIPS@  tail call void asm sideeffect "syscall\0A", "~{$4},~{$c2},~{$1}"()
@IFNOT-MIPS@  tail call void asm sideeffect "ecall", "~{a0},~{ca3},~{ct0}"()
  %0 = load volatile i8, ptr addrspace(200) %ptrval, align 1
  %1 = load volatile i8, ptr addrspace(200) %ptrval2, align 1
  %conv = sext i8 %0 to i32
  %conv2 = sext i8 %1 to i32
  %add = add nsw i32 %conv, %intval
  %add2 = add nsw i32 %conv2, %add
  ret i32 %add2
}

define i32 @test_clobber_c3(i32 signext %intval, ptr addrspace(200) %ptrval, ptr addrspace(200) %ptrval2) local_unnamed_addr {
entry:
@IF-MIPS@  tail call void asm sideeffect "syscall\0A", "~{$4},~{$c3},~{$1}"()
@IFNOT-MIPS@  tail call void asm sideeffect "ecall", "~{a0},~{ca1},~{ct0}"()
  %0 = load volatile i8, ptr addrspace(200) %ptrval, align 1
  %1 = load volatile i8, ptr addrspace(200) %ptrval2, align 1
  %conv = sext i8 %0 to i32
  %conv2 = sext i8 %1 to i32
  %add = add nsw i32 %conv, %intval
  %add2 = add nsw i32 %conv2, %add
  ret i32 %add2
}

define i32 @test_clobber_c4(i32 signext %intval, ptr addrspace(200) %ptrval, ptr addrspace(200) %ptrval2) local_unnamed_addr {
entry:
@IF-MIPS@  tail call void asm sideeffect "syscall\0A", "~{$4},~{$c4},~{$1}"()
@IFNOT-MIPS@  tail call void asm sideeffect "ecall", "~{a0},~{ca2},~{ct0}"()
  %0 = load volatile i8, ptr addrspace(200) %ptrval, align 1
  %1 = load volatile i8, ptr addrspace(200) %ptrval2, align 1
  %conv = sext i8 %0 to i32
  %conv2 = sext i8 %1 to i32
  %add = add nsw i32 %conv, %intval
  %add2 = add nsw i32 %conv2, %add
  ret i32 %add2
}

define i32 @test_clobber_c3_c4(i32 signext %intval, ptr addrspace(200) %ptrval, ptr addrspace(200) %ptrval2) local_unnamed_addr {
entry:
@IF-MIPS@  tail call void asm sideeffect "syscall\0A", "~{$4},~{$c3},~{$c4},~{$1}"()
@IFNOT-MIPS@  tail call void asm sideeffect "ecall", "~{a0},~{ca1},~{ca2},~{ct0}"()
  %0 = load volatile i8, ptr addrspace(200) %ptrval, align 1
  %1 = load volatile i8, ptr addrspace(200) %ptrval2, align 1
  %conv = sext i8 %0 to i32
  %conv2 = sext i8 %1 to i32
  %add = add nsw i32 %conv, %intval
  %add2 = add nsw i32 %conv2, %add
  ret i32 %add2
}
