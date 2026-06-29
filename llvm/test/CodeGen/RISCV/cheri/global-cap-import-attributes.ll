; RUN: llc --filetype=asm --mcpu=cheriot --mtriple=riscv32-unknown-unknown-cheriotrtos -target-abi cheriot -mattr=+xcheri,+xcheripurecap,+xcheriot < %s | FileCheck %s

target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32-unknown-cheriotrtos"

%struct.Uart = type { i8 }


; Function Attrs: mustprogress noinline optnone
define dso_local void @_Z7examplev() addrspace(200) #0 {
entry:
; CHECK:        .LBB0_1:                                # %entry
; CHECK-NEXT:                                           # Label of block must be emitted
; CHECK-NEXT:	        auipcc	a0, %cheriot_compartment_code_hi("__import_mem_uart_-----")
; CHECK-NEXT:	        clc	a0, %cheriot_compartment_lo_i(.LBB0_1)(a0)
; CHECK-NEXT:	        ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_no_perm)
; CHECK:        .LBB0_2:                                # %entry
; CHECK-NEXT:                                           # Label of block must be emitted
; CHECK-NEXT:        	auipcc	a0, %cheriot_compartment_code_hi("__import_mem_uart_--c--")
; CHECK-NEXT:        	clc	a0, %cheriot_compartment_lo_i(.LBB0_2)(a0)
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_c)
; CHECK:        .LBB0_3:                                # %entry
; CHECK-NEXT:                                           # Label of block must be emitted
; CHECK-NEXT:        	auipcc	a0, %cheriot_compartment_code_hi("__import_mem_uart_---m-")
; CHECK-NEXT:        	clc	a0, %cheriot_compartment_lo_i(.LBB0_3)(a0)
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_m)

; CHECK:        .LBB0_4:                                # %entry
; CHECK-NEXT:                                           # Label of block must be emitted
; CHECK-NEXT:        	auipcc	a0, %cheriot_compartment_code_hi("__import_mem_uart_----g")
; CHECK-NEXT:        	clc	a0, %cheriot_compartment_lo_i(.LBB0_4)(a0)
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_g)

; CHECK:        .LBB0_5:                                # %entry
; CHECK-NEXT:                                           # Label of block must be emitted
; CHECK-NEXT:        	auipcc	a0, %cheriot_compartment_code_hi("__import_mem_uart_--cm-")
; CHECK-NEXT:        	clc	a0, %cheriot_compartment_lo_i(.LBB0_5)(a0)
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_cm)

; CHECK:        .LBB0_6:                                # %entry
; CHECK-NEXT:                                           # Label of block must be emitted
; CHECK-NEXT:        	auipcc	a0, %cheriot_compartment_code_hi("__import_mem_uart_-W---")
; CHECK-NEXT:        	clc	a0, %cheriot_compartment_lo_i(.LBB0_6)(a0)
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_W)

; CHECK:        .LBB0_7:                                # %entry
; CHECK-NEXT:                                           # Label of block must be emitted
; CHECK-NEXT:        	auipcc	a0, %cheriot_compartment_code_hi("__import_mem_uart_-Wc--")
; CHECK-NEXT:        	clc	a0, %cheriot_compartment_lo_i(.LBB0_7)(a0)
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_Wc)

; CHECK:        .LBB0_8:                                # %entry
; CHECK-NEXT:                                           # Label of block must be emitted
; CHECK-NEXT:        	auipcc	a0, %cheriot_compartment_code_hi("__import_mem_uart_-W-m-")
; CHECK-NEXT:        	clc	a0, %cheriot_compartment_lo_i(.LBB0_8)(a0)
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_Wm)

; CHECK:        .LBB0_9:                                # %entry
; CHECK-NEXT:                                           # Label of block must be emitted
; CHECK-NEXT:        	auipcc	a0, %cheriot_compartment_code_hi("__import_mem_uart_-Wcm-")
; CHECK-NEXT:        	clc	a0, %cheriot_compartment_lo_i(.LBB0_9)(a0)
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_Wcm)

; CHECK:        .LBB0_10:                                # %entry
; CHECK-NEXT:                                           # Label of block must be emitted
; CHECK-NEXT:        	auipcc	a0, %cheriot_compartment_code_hi("__import_mem_uart_R----")
; CHECK-NEXT:        	clc	a0, %cheriot_compartment_lo_i(.LBB0_10)(a0)
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_R)

; CHECK:        .LBB0_11:                               # %entry
; CHECK-NEXT:                                           # Label of block must be emitted
; CHECK-NEXT:        	auipcc	a0, %cheriot_compartment_code_hi("__import_mem_uart_R-c--")
; CHECK-NEXT:        	clc	a0, %cheriot_compartment_lo_i(.LBB0_11)(a0)
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_Rc)

; CHECK:        .LBB0_12:                               # %entry
; CHECK-NEXT:                                           # Label of block must be emitted
; CHECK-NEXT:        	auipcc	a0, %cheriot_compartment_code_hi("__import_mem_uart_R--m-")
; CHECK-NEXT:        	clc	a0, %cheriot_compartment_lo_i(.LBB0_12)(a0)
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_Rm)

; CHECK:        .LBB0_13:                               # %entry
; CHECK-NEXT:                                           # Label of block must be emitted
; CHECK-NEXT:        	auipcc	a0, %cheriot_compartment_code_hi("__import_mem_uart_R-cm-")
; CHECK-NEXT:        	clc	a0, %cheriot_compartment_lo_i(.LBB0_13)(a0)
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_Rcm)

; CHECK:        .LBB0_14:                               # %entry
; CHECK-NEXT:                                           # Label of block must be emitted
; CHECK-NEXT:        	auipcc	a0, %cheriot_compartment_code_hi("__import_mem_uart_RW---")
; CHECK-NEXT:        	clc	a0, %cheriot_compartment_lo_i(.LBB0_14)(a0)
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_RW)

; CHECK:        .LBB0_15:                               # %entry
; CHECK-NEXT:                                           # Label of block must be emitted
; CHECK-NEXT:        	auipcc	a0, %cheriot_compartment_code_hi("__import_mem_uart_RWc--")
; CHECK-NEXT:        	clc	a0, %cheriot_compartment_lo_i(.LBB0_15)(a0)
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_RWc)

; CHECK:        .LBB0_16:                               # %entry
; CHECK-NEXT:                                           # Label of block must be emitted
; CHECK-NEXT:        	auipcc	a0, %cheriot_compartment_code_hi("__import_mem_uart_RW-m-")
; CHECK-NEXT:        	clc	a0, %cheriot_compartment_lo_i(.LBB0_16)(a0)
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_RWm)

;; We will add more calls to this global, so it will be automatically spilled
;; to the stack to preserve it across calls.

; CHECK:        .LBB0_17:                               # %entry
; CHECK-NEXT:                                           # Label of block must be emitted
; CHECK-NEXT:        	auipcc	s1, %cheriot_compartment_code_hi("__import_mem_uart_RWcm-")
; CHECK-NEXT:        	clc	s1, %cheriot_compartment_lo_i(.LBB0_17)(s1)
; CHECK-NEXT:        	cmove	a0, s1
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_RWcm)

; CHECK-NEXT:        	cmove   a0, s1
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_RWcm)

; CHECK-NEXT:        	cmove	a0, s1
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_RWcm)

; CHECK-NEXT:        	cmove	a0, s1
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_RWcm)

; CHECK-NEXT:        	cmove	a0, s1
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_RWcm)

; Duplicate entry should refer to `__import_mem_uart_RWcm`, too.

; CHECK:        .LBB0_18:                               # %entry
; CHECK-NEXT:                                           # Label of block must be emitted
; CHECK-NEXT:        	auipcc	s1, %cheriot_compartment_code_hi("__import_mem_uart_RWcm-")
; CHECK-NEXT:        	clc	s1, %cheriot_compartment_lo_i(.LBB0_18)(s1)
; CHECK-NEXT:           cmove	a0, s1
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_RWcm2)

; CHECK-NEXT:           cmove   a0, s1
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_RWcm2)

; CHECK:        .LBB0_19:                               # %entry
; CHECK-NEXT:                                           # Label of block must be emitted
; CHECK-NEXT:        	auipcc	a0, %cheriot_compartment_code_hi("__import_cheriot_shared_object_shared_obj_RWcm-")
; CHECK-NEXT:        	clc	a0, %cheriot_compartment_lo_i(.LBB0_19)(a0)
; CHECK-NEXT:        	ccall	_Z19doSomethingWithSharedObjectP4SharedObject
  call void @_Z19doSomethingWithSharedObjectP4SharedObject(ptr addrspace(200) noundef @shared_obj_RWcm)

; CHECK:        .LBB0_20:                               # %entry
; CHECK-NEXT:                                           # Label of block must be emitted
; CHECK-NEXT:        	auipcc	a0, %cheriot_compartment_code_hi(__import_mem_uart_RWcmg)
; CHECK-NEXT:        	clc	a0, %cheriot_compartment_lo_i(.LBB0_20)(a0)
; CHECK-NEXT:        	ccall	_Z19doSomethingWithUartP4Uart
  call void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef @uart_RWcmg)

; CHECK:        .LBB0_21:                               # %entry
; CHECK-NEXT:                                           # Label of block must be emitted
; CHECK-NEXT:        	auipcc	a0, %cheriot_compartment_code_hi(__import_cheriot_shared_object_shared_obj_RWcmg)
; CHECK-NEXT:        	clc	a0, %cheriot_compartment_lo_i(.LBB0_21)(a0)
; CHECK-NEXT:        	ccall	_Z19doSomethingWithSharedObjectP4SharedObject
  call void @_Z19doSomethingWithSharedObjectP4SharedObject(ptr addrspace(200) noundef @shared_obj_RWcmg)


  ret void
}

declare dso_local void @_Z19doSomethingWithUartP4Uart(ptr addrspace(200) noundef) addrspace(200) #1
declare dso_local void @_Z19doSomethingWithSharedObjectP4SharedObject(ptr addrspace(200) noundef) addrspace(200) #1

; CHECK:            .section .compartment_imports.uart,"awG",@progbits,"__import_mem_uart_-----",comdat
; CHECK-NEXT:       .type "__import_mem_uart_-----",@object
; CHECK-NEXT: 	    .globl "__import_mem_uart_-----"
; CHECK-NEXT: 	    .p2align	3, 0x0
; CHECK-NEXT:  "__import_mem_uart_-----":
; CHECK-NEXT: 	    .word	__export_mem_uart
; CHECK-NEXT: 	    .word	__export_mem_uart_end-__export_mem_uart+0
; CHECK-NEXT: 	    .size	"__import_mem_uart_-----", 8
@uart_no_perm = external addrspace(200) global %struct.Uart, align 1 "cheriot_global_cap_import" = "mem,uart,-----"

; CHECK: 	    .section .compartment_imports.uart,"awG",@progbits,"__import_mem_uart_--c--",comdat
; CHECK-NEXT:       .type	"__import_mem_uart_--c--",@object
; CHECK-NEXT: 	    .globl "__import_mem_uart_--c--"
; CHECK-NEXT: 	    .p2align	3, 0x0
; CHECK-NEXT:  "__import_mem_uart_--c--":
; CHECK-NEXT: 	    .word	__export_mem_uart
; CHECK-NEXT: 	    .word	__export_mem_uart_end-__export_mem_uart+536870912
; CHECK-NEXT: 	    .size	"__import_mem_uart_--c--", 8
@uart_c = external addrspace(200) global %struct.Uart, align 1 "cheriot_global_cap_import" = "mem,uart,--c--"

; CHECK: 	    .section .compartment_imports.uart,"awG",@progbits,"__import_mem_uart_---m-",comdat
; CHECK-NEXT:       .type	"__import_mem_uart_---m-",@object
; CHECK-NEXT: 	    .globl "__import_mem_uart_---m-"
; CHECK-NEXT: 	    .p2align	3, 0x0
; CHECK-NEXT:  "__import_mem_uart_---m-":
; CHECK-NEXT: 	    .word	__export_mem_uart
; CHECK-NEXT: 	    .word	__export_mem_uart_end-__export_mem_uart+268435456
; CHECK-NEXT: 	    .size	"__import_mem_uart_---m-", 8
@uart_m = external addrspace(200) global %struct.Uart, align 1 "cheriot_global_cap_import" = "mem,uart,---m-"

; CHECK: 	    .section .compartment_imports.uart,"awG",@progbits,"__import_mem_uart_----g",comdat
; CHECK-NEXT:       .type	"__import_mem_uart_----g",@object
; CHECK-NEXT: 	    .globl "__import_mem_uart_----g"
; CHECK-NEXT: 	    .p2align	3, 0x0
; CHECK-NEXT:  "__import_mem_uart_----g":
; CHECK-NEXT: 	    .word	__export_mem_uart
; CHECK-NEXT: 	    .word	__export_mem_uart_end-__export_mem_uart+134217728
; CHECK-NEXT: 	    .size	"__import_mem_uart_----g", 8
@uart_g = external addrspace(200) global %struct.Uart, align 1 "cheriot_global_cap_import" = "mem,uart,----g"

; CHECK: 	    .section .compartment_imports.uart,"awG",@progbits,"__import_mem_uart_--cm-",comdat
; CHECK-NEXT:       .type	"__import_mem_uart_--cm-",@object
; CHECK-NEXT: 	    .globl "__import_mem_uart_--cm-"
; CHECK-NEXT: 	    .p2align	3, 0x0
; CHECK-NEXT:  "__import_mem_uart_--cm-":
; CHECK-NEXT: 	    .word	__export_mem_uart
; CHECK-NEXT: 	    .word	__export_mem_uart_end-__export_mem_uart+805306368
; CHECK-NEXT: 	    .size	"__import_mem_uart_--cm-", 8
@uart_cm = external addrspace(200) global %struct.Uart, align 1 "cheriot_global_cap_import" = "mem,uart,--cm-"

; CHECK:  	    .section .compartment_imports.uart,"awG",@progbits,"__import_mem_uart_-W---",comdat
; CHECK-NEXT:       .type	"__import_mem_uart_-W---",@object
; CHECK-NEXT:  	    .globl "__import_mem_uart_-W---"
; CHECK-NEXT:  	    .p2align	3, 0x0
; CHECK-NEXT:   "__import_mem_uart_-W---":
; CHECK-NEXT:  	    .word	__export_mem_uart
; CHECK-NEXT:  	    .word	__export_mem_uart_end-__export_mem_uart+1073741824
; CHECK-NEXT:  	    .size	"__import_mem_uart_-W---", 8
@uart_W = external addrspace(200) global %struct.Uart, align 1 "cheriot_global_cap_import" = "mem,uart,-W---"

; CHECK: 	    .section .compartment_imports.uart,"awG",@progbits,"__import_mem_uart_-Wc--",comdat
; CHECK-NEXT:       .type	"__import_mem_uart_-Wc--",@object
; CHECK-NEXT: 	    .globl "__import_mem_uart_-Wc--"
; CHECK-NEXT: 	    .p2align	3, 0x0
; CHECK-NEXT:  "__import_mem_uart_-Wc--":
; CHECK-NEXT: 	    .word	__export_mem_uart
; CHECK-NEXT: 	    .word	__export_mem_uart_end-__export_mem_uart+1610612736
; CHECK-NEXT: 	    .size	"__import_mem_uart_-Wc--", 8
@uart_Wc = external addrspace(200) global %struct.Uart, align 1 "cheriot_global_cap_import" = "mem,uart,-Wc--"

; CHECK: 	    .section .compartment_imports.uart,"awG",@progbits,"__import_mem_uart_-W-m-",comdat
; CHECK-NEXT:       .type	"__import_mem_uart_-W-m-",@object
; CHECK-NEXT: 	    .globl "__import_mem_uart_-W-m-"
; CHECK-NEXT: 	    .p2align	3, 0x0
; CHECK-NEXT:  "__import_mem_uart_-W-m-":
; CHECK-NEXT: 	    .word	__export_mem_uart
; CHECK-NEXT: 	    .word	__export_mem_uart_end-__export_mem_uart+1342177280
; CHECK-NEXT: 	    .size	"__import_mem_uart_-W-m-", 8
@uart_Wm = external addrspace(200) global %struct.Uart, align 1 "cheriot_global_cap_import" = "mem,uart,-W-m-"

; CHECK: 	    .section .compartment_imports.uart,"awG",@progbits,"__import_mem_uart_-Wcm-",comdat
; CHECK-NEXT:       .type	"__import_mem_uart_-Wcm-",@object
; CHECK-NEXT: 	    .globl "__import_mem_uart_-Wcm-"
; CHECK-NEXT: 	    .p2align	3, 0x0
; CHECK-NEXT:  "__import_mem_uart_-Wcm-":
; CHECK-NEXT: 	    .word	__export_mem_uart
; CHECK-NEXT: 	    .word	__export_mem_uart_end-__export_mem_uart+1879048192
; CHECK-NEXT: 	    .size	"__import_mem_uart_-Wcm-", 8
@uart_Wcm = external addrspace(200) global %struct.Uart, align 1 "cheriot_global_cap_import" = "mem,uart,-Wcm-"

; CHECK: 	    .section .compartment_imports.uart,"awG",@progbits,"__import_mem_uart_R----",comdat
; CHECK-NEXT:       .type	"__import_mem_uart_R----",@object
; CHECK-NEXT: 	    .globl "__import_mem_uart_R----"
; CHECK-NEXT: 	    .p2align	3, 0x0
; CHECK-NEXT:  "__import_mem_uart_R----":
; CHECK-NEXT: 	    .word	__export_mem_uart
; CHECK-NEXT: 	    .word	__export_mem_uart_end-__export_mem_uart-2147483648
; CHECK-NEXT: 	    .size	"__import_mem_uart_R----", 8
@uart_R = external addrspace(200) global %struct.Uart, align 1 "cheriot_global_cap_import" = "mem,uart,R----"

; CHECK: 	    .section .compartment_imports.uart,"awG",@progbits,"__import_mem_uart_R-c--",comdat
; CHECK-NEXT:       .type	"__import_mem_uart_R-c--",@object
; CHECK-NEXT: 	    .globl "__import_mem_uart_R-c--"
; CHECK-NEXT: 	    .p2align	3, 0x0
; CHECK-NEXT:  "__import_mem_uart_R-c--":
; CHECK-NEXT: 	    .word	__export_mem_uart
; CHECK-NEXT: 	    .word	__export_mem_uart_end-__export_mem_uart-1610612736
; CHECK-NEXT: 	    .size	"__import_mem_uart_R-c--", 8
@uart_Rc = external addrspace(200) global %struct.Uart, align 1 "cheriot_global_cap_import" = "mem,uart,R-c--"

; CHECK: 	    .section .compartment_imports.uart,"awG",@progbits,"__import_mem_uart_R--m-",comdat
; CHECK-NEXT:       .type	"__import_mem_uart_R--m-",@object
; CHECK-NEXT: 	    .globl "__import_mem_uart_R--m-"
; CHECK-NEXT: 	    .p2align	3, 0x0
; CHECK-NEXT:  "__import_mem_uart_R--m-":
; CHECK-NEXT: 	    .word	__export_mem_uart
; CHECK-NEXT: 	    .word	__export_mem_uart_end-__export_mem_uart-1879048192
; CHECK-NEXT: 	    .size	"__import_mem_uart_R--m-", 8
@uart_Rm = external addrspace(200) global %struct.Uart, align 1 "cheriot_global_cap_import" = "mem,uart,R--m-"

; CHECK: 	    .section .compartment_imports.uart,"awG",@progbits,"__import_mem_uart_R-cm-",comdat
; CHECK-NEXT:       .type	"__import_mem_uart_R-cm-",@object
; CHECK-NEXT: 	    .globl "__import_mem_uart_R-cm-"
; CHECK-NEXT: 	    .p2align	3, 0x0
; CHECK-NEXT:  "__import_mem_uart_R-cm-":
; CHECK-NEXT: 	    .word	__export_mem_uart
; CHECK-NEXT: 	    .word	__export_mem_uart_end-__export_mem_uart-1342177280
; CHECK-NEXT: 	    .size	"__import_mem_uart_R-cm-", 8
@uart_Rcm = external addrspace(200) global %struct.Uart, align 1 "cheriot_global_cap_import" = "mem,uart,R-cm-"

; CHECK: 	    .section .compartment_imports.uart,"awG",@progbits,"__import_mem_uart_RW---",comdat
; CHECK-NEXT:       .type	"__import_mem_uart_RW---",@object
; CHECK-NEXT: 	    .globl "__import_mem_uart_RW---"
; CHECK-NEXT: 	    .p2align	3, 0x0
; CHECK-NEXT:  "__import_mem_uart_RW---":
; CHECK-NEXT: 	    .word	__export_mem_uart
; CHECK-NEXT: 	    .word __export_mem_uart_end-__export_mem_uart-1073741824
; CHECK-NEXT: 	    .size	"__import_mem_uart_RW---", 8
@uart_RW = external addrspace(200) global %struct.Uart, align 1 "cheriot_global_cap_import" = "mem,uart,RW---"

; CHECK: 	    .section .compartment_imports.uart,"awG",@progbits,"__import_mem_uart_RWc--",comdat
; CHECK-NEXT:       .type	"__import_mem_uart_RWc--",@object
; CHECK-NEXT: 	    .globl "__import_mem_uart_RWc--"
; CHECK-NEXT: 	    .p2align	3, 0x0
; CHECK-NEXT:  "__import_mem_uart_RWc--":
; CHECK-NEXT: 	    .word	__export_mem_uart
; CHECK-NEXT: 	    .word __export_mem_uart_end-__export_mem_uart-536870912
; CHECK-NEXT: 	    .size	"__import_mem_uart_RWc--", 8
@uart_RWc = external addrspace(200) global %struct.Uart, align 1 "cheriot_global_cap_import" = "mem,uart,RWc--"

; CHECK: 	    .section .compartment_imports.uart,"awG",@progbits,"__import_mem_uart_RW-m-",comdat
; CHECK-NEXT:       .type	"__import_mem_uart_RW-m-",@object
; CHECK-NEXT: 	    .globl "__import_mem_uart_RW-m-"
; CHECK-NEXT: 	    .p2align	3, 0x0
; CHECK-NEXT:  "__import_mem_uart_RW-m-":
; CHECK-NEXT: 	    .word	__export_mem_uart
; CHECK-NEXT: 	    .word	__export_mem_uart_end-__export_mem_uart-805306368
; CHECK-NEXT: 	    .size	"__import_mem_uart_RW-m-", 8
@uart_RWm = external addrspace(200) global %struct.Uart, align 1 "cheriot_global_cap_import" = "mem,uart,RW-m-"

; Arbitrarily duplicated entry, made to check that two different globals that
; refer to the same mmio device do not produce two duplicated entries in the
; imports table.
@uart_RWcm2 = external addrspace(200) global %struct.Uart, align 1 "cheriot_global_cap_import" = "mem,uart,RWcm-"

; CHECK: 	    .section .compartment_imports.uart,"awG",@progbits,"__import_mem_uart_RWcm-",comdat
; CHECK-NEXT:       .type	"__import_mem_uart_RWcm-",@object
; CHECK-NEXT: 	    .globl "__import_mem_uart_RWcm-"
; CHECK-NEXT: 	    .p2align	3, 0x0
; CHECK-NEXT:  "__import_mem_uart_RWcm-":
; CHECK-NEXT: 	    .word	__export_mem_uart
; CHECK-NEXT: 	    .word	__export_mem_uart_end-__export_mem_uart-268435456
; CHECK-NEXT: 	    .size	"__import_mem_uart_RWcm-", 8
@uart_RWcm = external addrspace(200) global %struct.Uart, align 1 "cheriot_global_cap_import" = "mem,uart,RWcm-"

; CHECK: 	    .section .compartment_imports.shared_obj,"awG",@progbits,"__import_cheriot_shared_object_shared_obj_RWcm-",comdat
; CHECK-NEXT:       .type "__import_cheriot_shared_object_shared_obj_RWcm-",@object
; CHECK-NEXT: 	    .globl "__import_cheriot_shared_object_shared_obj_RWcm-"
; CHECK-NEXT: 	    .p2align	3, 0x0
; CHECK-NEXT:  "__import_cheriot_shared_object_shared_obj_RWcm-":
; CHECK-NEXT: 	    .word	__cheriot_shared_object_shared_obj
; CHECK-NEXT: 	    .word __cheriot_shared_object_shared_obj_end-__cheriot_shared_object_shared_obj-268435456
; CHECK-NEXT: 	    .size "__import_cheriot_shared_object_shared_obj_RWcm-", 8
@shared_obj_RWcm = external addrspace(200) global %struct.Uart, align 1 "cheriot_global_cap_import" = "cheriot_shared_object,shared_obj,RWcm-"

; CHECK: 	    .section .compartment_imports.uart,"awG",@progbits,__import_mem_uart_RWcmg,comdat
; CHECK-NEXT:       .type	__import_mem_uart_RWcmg,@object
; CHECK-NEXT: 	    .globl __import_mem_uart_RWcmg
; CHECK-NEXT: 	    .p2align	3, 0x0
; CHECK-NEXT:  __import_mem_uart_RWcmg:
; CHECK-NEXT: 	    .word	__export_mem_uart
; CHECK-NEXT: 	    .word	__export_mem_uart_end-__export_mem_uart-134217728
; CHECK-NEXT: 	    .size	__import_mem_uart_RWcmg, 8
@uart_RWcmg = external addrspace(200) global %struct.Uart, align 1 "cheriot_global_cap_import" = "mem,uart,RWcmg"

; CHECK: 	    .section .compartment_imports.shared_obj,"awG",@progbits,__import_cheriot_shared_object_shared_obj_RWcmg,comdat
; CHECK-NEXT:       .type __import_cheriot_shared_object_shared_obj_RWcmg,@object
; CHECK-NEXT: 	    .globl __import_cheriot_shared_object_shared_obj_RWcmg
; CHECK-NEXT: 	    .p2align	3, 0x0
; CHECK-NEXT:  __import_cheriot_shared_object_shared_obj_RWcmg:
; CHECK-NEXT: 	    .word	__cheriot_shared_object_shared_obj
; CHECK-NEXT: 	    .word __cheriot_shared_object_shared_obj_end-__cheriot_shared_object_shared_obj-134217728
; CHECK-NEXT: 	    .size __import_cheriot_shared_object_shared_obj_RWcmg, 8
@shared_obj_RWcmg = external addrspace(200) global %struct.Uart, align 1 "cheriot_global_cap_import" = "cheriot_shared_object,shared_obj,RWcmg"


attributes #0 = { mustprogress noinline optnone "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+32bit,+c,+xcheripurecap,+e,+m,+relax,+xcheri,+zmmul,-a,-b,-d,-experimental-sdext,-experimental-sdtrig,-experimental-smctr,-experimental-ssctr,-experimental-svukte,-experimental-xqcia,-experimental-xqciac,-experimental-xqcicli,-experimental-xqcicm,-experimental-xqcics,-experimental-xqcicsr,-experimental-xqciint,-experimental-xqcilo,-experimental-xqcilsm,-experimental-xqcisls,-experimental-zalasr,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvkgs,-f,-h,-i,-sha,-shcounterenw,-shgatpa,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcsrind,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscmove,-xmipslsp,-xsfcease,-xsfvcp,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zaamo,-zabha,-zacas,-zalrsc,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,+zca,-zcb,-zcd,-zce,-zcf,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zicsr,-zifencei,-zihintntl,-zihintpause,-zihpm,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+32bit,+c,+xcheripurecap,+e,+m,+relax,+xcheri,+zmmul,-a,-b,-d,-experimental-sdext,-experimental-sdtrig,-experimental-smctr,-experimental-ssctr,-experimental-svukte,-experimental-xqcia,-experimental-xqciac,-experimental-xqcicli,-experimental-xqcicm,-experimental-xqcics,-experimental-xqcicsr,-experimental-xqciint,-experimental-xqcilo,-experimental-xqcilsm,-experimental-xqcisls,-experimental-zalasr,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvkgs,-f,-h,-i,-sha,-shcounterenw,-shgatpa,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcsrind,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscmove,-xmipslsp,-xsfcease,-xsfvcp,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zaamo,-zabha,-zacas,-zalrsc,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,+zca,-zcb,-zcd,-zce,-zcf,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zicsr,-zifencei,-zihintntl,-zihintpause,-zihpm,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }

!llvm.module.flags = !{!0, !1, !2, !4, !5}
!llvm.ident = !{!6}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, !"target-abi", !"cheriot"}
!2 = !{i32 6, !"riscv-isa", !3}
!3 = !{!"rv32e2p0_m2p0_c2p0_zmmul1p0_xcheri0p0"}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{i32 8, !"SmallDataLimit", i32 0}
!6 = !{!"clang version 20.1.3 (https://github.com/CHERIoT-Platform/llvm-project.git 44c01c22f58a1fa95df120c6045886ca38c44339)"}
