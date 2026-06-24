; !DO NOT AUTOGEN!
; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -O3 -verify-machineinstrs %s -o - | FileCheck %s

define void @test_C_constraint() addrspace(200) nounwind {
entry:
  ; CHECK-LABEL: test_C_constraint:
@IF-MIPS@; CHECK: # operand is '$c1'
@IF-RISCV@; CHECK: # operand is '{{a0|zero}}'
  call void asm sideeffect "# operand is '$0'", "C,~{memory}"(ptr addrspace(200) null) nounwind
  ret void
}

define void @test_m_constraint() addrspace(200) nounwind {
entry:
  ; CHECK-LABEL: test_m_constraint:
@IF-MIPS@; CHECK: # operand is '0($c11)'
@IF-RISCV32@; CHECK: # operand is '8(sp)'
@IF-RISCV64@; CHECK: # operand is '0(sp)'
  call void asm sideeffect "# operand is '$0'", "m,~{memory}"(ptr addrspace(200) null) nounwind
  ret void
}

define void @test_rm_constraint() addrspace(200) nounwind {
entry:
  ; CHECK-LABEL: test_rm_constraint:
@IF-MIPS@; CHECK: # operand is '0($c11)'
@IF-RISCV32@; CHECK: # operand is '8(sp)'
@IF-RISCV64@; CHECK: # operand is '0(sp)'
  call void asm sideeffect "# operand is '$0'", "r|m,~{memory}"(ptr addrspace(200) null) nounwind
  ret void
}

define void @test_crm_constraint() addrspace(200) nounwind {
entry:
  ; CHECK-LABEL: test_crm_constraint:
@IF-MIPS@; CHECK: # operand is '0($c11)'
@IF-RISCV32@; CHECK: # operand is '8(sp)'
@IF-RISCV64@; CHECK: # operand is '0(sp)'
  call void asm sideeffect "# operand is '$0'", "C|r|m,~{memory}"(ptr addrspace(200) null) nounwind
  ret void
}
