; This test is designed to run twice, once with function attributes and once
; with target attributes added on the command line.
; See compress.ll in the folder above
;
; RUN: cat %s > %t.tgtattr
; RUN: echo 'attributes #0 = { nounwind }' >> %t.tgtattr
; RUN: %riscv32_cheri_purecap_llc -mattr=+c,+xcheri,+xcheripurecap -filetype=obj < %t.tgtattr \
; RUN:   | llvm-objdump -d -M no-aliases - | FileCheck %s
; RUN: %riscv64_cheri_purecap_llc -mattr=+c,+xcheri,+xcheripurecap -filetype=obj < %t.tgtattr \
; RUN:   | llvm-objdump -d -M no-aliases - | FileCheck %s
; RUN: %riscv64_cheri_purecap_llc -mattr=+c,+xcheri,+xcheripurecap,+xcheri-norvc -filetype=obj < %t.tgtattr \
; RUN:   | llvm-objdump -d -M no-aliases - | FileCheck %s --check-prefix=CHECK-NORVC

; RUN: cat %s > %t.fnattr
; RUN: echo 'attributes #0 = { nounwind "target-features"="+c,+xcheri,+xcheripurecap" }' >> %t.fnattr
; RUN: %riscv32_cheri_purecap_llc -filetype=obj < %t.fnattr \
; RUN:   | llvm-objdump -d --mattr=+c -M no-aliases - | FileCheck %s
; RUN: %riscv64_cheri_purecap_llc -filetype=obj < %t.fnattr \
; RUN:   | llvm-objdump -d --mattr=+c -M no-aliases - | FileCheck %s
; RUN: cat %s > %t.fnattr
; RUN: echo 'attributes #0 = { nounwind "target-features"="+c,+xcheri,+xcheripurecap,+xcheri-norvc" }' >> %t.fnattr
; RUN: %riscv64_cheri_purecap_llc -filetype=obj < %t.fnattr \
; RUN:   | llvm-objdump -d --mattr=+c -M no-aliases - | FileCheck %s --check-prefix=CHECK-NORVC

; Basic check that we can use CHERI compressed instructions

define i32 @loadstore(i32 addrspace(200)* %intptrarg, i8 addrspace(200)* addrspace(200)* %ptrptrarg) addrspace(200) #0 {
; CHECK-LABEL: <loadstore>:
; CHECK-NEXT:    c.cincoffset16csp sp, -0x20
; CHECK-NEXT:    c.li a3, 0x1
; CHECK-NEXT:    c.clw a2, 0x0(a0)
; CHECK-NEXT:    c.csw a3, 0x0(a0)
; CHECK-NEXT:    c.clc a0, 0x0(a1)
; CHECK-NEXT:    c.csc a0, 0x0(a1)
; CHECK-NEXT:    c.csccsp a0, 0x10(sp)
; CHECK-NEXT:    c.clccsp a0, 0x10(sp)
; CHECK-NEXT:    c.cswcsp a2, 0x0(sp)
; CHECK-NEXT:    clw zero, 0x0(sp)
; CHECK-NEXT:    c.mv a0, a2
; CHECK-NEXT:    c.cincoffset16csp sp, 0x20
; CHECK-NEXT:    c.cjr ra
; CHECK-NORVC-LABEL: <loadstore>:
; CHECK-NORVC-NEXT:  {{[^a-z.]}}cincoffset sp, sp, -0x20
; CHECK-NORVC-NEXT:  {{[^a-z.]}}c.li a3, 0x1
; CHECK-NORVC-NEXT:  {{[^a-z.]}}clw a2, 0x0(a0)
; CHECK-NORVC-NEXT:  {{[^a-z.]}}csw a3, 0x0(a0)
; CHECK-NORVC-NEXT:  {{[^a-z.]}}clc a0, 0x0(a1)
; CHECK-NORVC-NEXT:  {{[^a-z.]}}csc a0, 0x0(a1)
; CHECK-NORVC-NEXT:  {{[^a-z.]}}csc a0, 0x10(sp)
; CHECK-NORVC-NEXT:  {{[^a-z.]}}clc a0, 0x10(sp)
; CHECK-NORVC-NEXT:  {{[^a-z.]}}csw a2, 0x0(sp)
; CHECK-NORVC-NEXT:  {{[^a-z.]}}clw zero, 0x0(sp)
; CHECK-NORVC-NEXT:  {{[^a-z.]}}c.mv a0, a2
; CHECK-NORVC-NEXT:  {{[^a-z.]}}cincoffset sp, sp, 0x20
; CHECK-NORVC-NEXT:  {{[^a-z.]}}cjalr zero, 0x0(ra)
  %stackptr = alloca i8 addrspace(200)*, align 16, addrspace(200)
  %stackint = alloca i32, align 16, addrspace(200)
  %val = load volatile i32, i32 addrspace(200)* %intptrarg
  store volatile i32 1, i32 addrspace(200)* %intptrarg
  %ptrval = load volatile i8 addrspace(200)*, i8 addrspace(200)* addrspace(200)* %ptrptrarg
  store volatile i8 addrspace(200)* %ptrval, i8 addrspace(200)* addrspace(200)* %ptrptrarg
  store volatile i8 addrspace(200)* %ptrval, i8 addrspace(200)* addrspace(200)* %stackptr
  %stackptrval = load volatile i8 addrspace(200)*, i8 addrspace(200)* addrspace(200)* %stackptr
  store volatile i32 %val, i32 addrspace(200)* %stackint
  %stackintval = load volatile i32, i32 addrspace(200)* %stackint
  ret i32 %val
}

;; NB: No c.cjalr here, linker relaxations expect the full sequence.
define i32 @call() addrspace(200) #0 {
; CHECK-LABEL: <call>:
; CHECK-NEXT:    c.cincoffset16csp sp, -{{0x70|0x90}}
; CHECK-NEXT:    c.csccsp ra, {{0x68|0x80}}(sp)
; CHECK-NEXT:    c.cincoffset4cspn a0, sp, 0x40
; CHECK-NEXT:    cincoffset a1, sp, 0x0
; CHECK-NEXT:    csetbounds a0, a0, {{0x20|0x40}}
; CHECK-NEXT:    csetbounds a2, a1, 0x40
; CHECK-NEXT:    cincoffset a1, a0, {{0x18|0x30}}
; CHECK-NEXT:    cincoffset a0, a2, 0xc
; CHECK-NEXT:    auipcc ra, 0x0
; CHECK-NEXT:    cjalr ra, 0x0(ra)
; CHECK-NEXT:    c.clccsp ra, {{0x68|0x80}}(sp)
; CHECK-NEXT:    c.cincoffset16csp sp, {{0x70|0x90}}
; CHECK-NEXT:    c.cjr ra
; CHECK-NORVC-LABEL: <call>:
; CHECK-NORVC-NEXT:  {{[^a-z.]}}cincoffset sp, sp, -0x90
; CHECK-NORVC-NEXT:  {{[^a-z.]}}csc ra, 0x80(sp)
; CHECK-NORVC-NEXT:  {{[^a-z.]}}cincoffset a0, sp, 0x40
; CHECK-NORVC-NEXT:  {{[^a-z.]}}cincoffset a1, sp, 0x0
; CHECK-NORVC-NEXT:  {{[^a-z.]}}csetbounds a0, a0, 0x40
; CHECK-NORVC-NEXT:  {{[^a-z.]}}csetbounds a2, a1, 0x40
; CHECK-NORVC-NEXT:  {{[^a-z.]}}cincoffset a1, a0, 0x30
; CHECK-NORVC-NEXT:  {{[^a-z.]}}cincoffset a0, a2, 0xc
; CHECK-NORVC-NEXT:  {{[^a-z.]}}auipcc ra, 0x0
; CHECK-NORVC-NEXT:  {{[^a-z.]}}cjalr ra, 0x0(ra)
; CHECK-NORVC-NEXT:  {{[^a-z.]}}clc ra, 0x80(sp)
; CHECK-NORVC-NEXT:  {{[^a-z.]}}cincoffset sp, sp, 0x90
; CHECK-NORVC-NEXT:  {{[^a-z.]}}cjalr zero, 0x0(ra)

  %ptrarray = alloca [4 x i8 addrspace(200)*], align 16, addrspace(200)
  %intarray = alloca [16 x i32], align 1, addrspace(200)
  %ptrgep = getelementptr inbounds [4 x i8 addrspace(200)*], [4 x i8 addrspace(200)*] addrspace(200)* %ptrarray, i64 0, i64 3
  %intgep = getelementptr inbounds [16 x i32], [16 x i32] addrspace(200)* %intarray, i64 0, i64 3
  %ret = call i32 @loadstore(i32 addrspace(200)* %intgep, i8 addrspace(200)* addrspace(200)* %ptrgep)
  ret i32 %ret
}
