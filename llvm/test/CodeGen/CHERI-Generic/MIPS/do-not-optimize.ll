; DO NOT EDIT -- This file was generated from test/CodeGen/CHERI-Generic/Inputs/do-not-optimize.ll
; RUN: llc -mtriple=mips64 -mcpu=cheri128 -mattr=+cheri128 --relocation-model=pic -target-abi purecap -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=mips64 -mcpu=cheri128 -mattr=+cheri128 --relocation-model=pic -target-abi purecap -O3 -verify-machineinstrs %s -o - | FileCheck %s

define void @test_C_constraint() addrspace(200) nounwind {
entry:
  ; CHECK-LABEL: test_C_constraint:
; CHECK: # operand is '$c1'
  call void asm sideeffect "# operand is '$0'", "C,~{memory}"(ptr addrspace(200) null) nounwind
  ret void
}

define void @test_m_constraint() addrspace(200) nounwind {
entry:
  ; CHECK-LABEL: test_m_constraint:
; CHECK: # operand is '0($c11)'
  call void asm sideeffect "# operand is '$0'", "m,~{memory}"(ptr addrspace(200) null) nounwind
  ret void
}

define void @test_rm_constraint() addrspace(200) nounwind {
entry:
  ; CHECK-LABEL: test_rm_constraint:
; CHECK: # operand is '0($c11)'
  call void asm sideeffect "# operand is '$0'", "r|m,~{memory}"(ptr addrspace(200) null) nounwind
  ret void
}

define void @test_crm_constraint() addrspace(200) nounwind {
entry:
  ; CHECK-LABEL: test_crm_constraint:
; CHECK: # operand is '0($c11)'
  call void asm sideeffect "# operand is '$0'", "C|r|m,~{memory}"(ptr addrspace(200) null) nounwind
  ret void
}
