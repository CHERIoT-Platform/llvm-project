// REQUIRES: clang

// RUN: %cheri_purecap_cc1 -mllvm -mxcaptable -emit-obj -O2 -mllvm -cheri-cap-table-abi=plt %s -o %t.o
// RUN: llvm-objdump -d -r -t %t.o | FileCheck %s --check-prefix OBJECT
// RUN: ld.lld -o %t.exe %t.o
// RUN: llvm-objdump --no-print-imm-hex -d -r --cap-relocs -t %t.exe | %cheri_FileCheck %s --check-prefixes EXE

// OBJECT-LABEL: SYMBOL TABLE:
// OBJECT: 0000000000000000 l O .rodata 0000000000000009 .Lswitch.table.__start

// OBJECT:      10:	3c 01 00 00 	lui	$1, 0
// OBJECT-NEXT: 0000000000000010:  R_MIPS_CHERI_CAPTAB_HI16/R_MIPS_NONE/R_MIPS_NONE	.Lswitch.table.__start
// OBJECT-NEXT: 14:	64 21 00 00 	daddiu	$1, $1, 0
// OBJECT-NEXT: 0000000000000014:  R_MIPS_CHERI_CAPTAB_LO16/R_MIPS_NONE/R_MIPS_NONE	.Lswitch.table.__start

// EXE: SYMBOL TABLE:
// EXE-DAG: 00000000000102a0 l O .rodata 0000000000000009 .Lswitch.table.__start
// EXE-DAG: 00000000000302e0 l     O .captable		 00000000000000{{1|2}}0 .Lswitch.table.__start@CAPTABLE

// __cap_relocs should contain length:
// EXE:      CAPABILITY RELOCATION RECORDS:
// EXE-NEXT: OFFSET           TYPE    VALUE
// EXE-NEXT: 00000000000302e0 RODATA  00000000000102a0 [00000000000102a0-00000000000102a9]

// EXE: 202c0:	3c 01 00 00 	lui	$1, 0
// EXE-NEXT: 202c4:	64 21 00 00 	daddiu	$1, $1, 0
// EXE-NEXT: 202c8:	d8 3a 08 00 	clc	$c1, $1, 0($c26)

int __start(int i) {
  switch(i) {
    case 1: return 9;
    case 4: return 10;
    case 5: return 22;
    case 7: return 77;
    case 9: return 9;
    default: return 0;
  }
}
