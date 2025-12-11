//===-- RISCVInstrInfoXCheri.td - XCheri RISCV instructions -*- tblgen-*---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// RISC-V specific DAG Nodes.
//===----------------------------------------------------------------------===//

// Target-dependent type requirements.
def SDT_RISCVCapCall        : SDTypeProfile<0, -1, [SDTCisVT<0, YLenVT>]>;

def SDT_RISCVCheriBoolUnary : SDTypeProfile<1, 1, [
  SDTCisInt<0>, SDTCisVT<1, YLenVT>
]>;

def SDT_RISCVCheriBoolBinary : SDTypeProfile<1, 2, [
  SDTCisInt<0>, SDTCisVT<1, YLenVT>, SDTCisVT<2, YLenVT>
]>;

// Target-dependent nodes.
def riscv_cap_call        : RVSDNode<"CAP_CALL", SDT_RISCVCapCall,
                                   [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue,
                                    SDNPVariadic]>;
def riscv_cap_compartment_call : RVSDNode<"CAP_COMPARTMENT_CALL", SDT_RISCVCapCall,
                                   [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue,
                                    SDNPVariadic]>;
def riscv_cap_library_call : RVSDNode<"CAP_LIB_CALL", SDT_RISCVCapCall,
                                   [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue,
                                    SDNPVariadic]>;
def riscv_cap_tail        : RVSDNode<"CAP_TAIL", SDT_RISCVCapCall,
                                   [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue,
                                    SDNPVariadic]>;
def riscv_cap_tag_get     : RVSDNode<"CAP_TAG_GET",
                                   SDT_RISCVCheriBoolUnary,
                                   [SDNPHasChain]>;
def riscv_cap_sealed_get  : RVSDNode<"CAP_SEALED_GET",
                                   SDT_RISCVCheriBoolUnary>;
def riscv_cap_subset_test : RVSDNode<"CAP_SUBSET_TEST",
                                   SDT_RISCVCheriBoolBinary>;
def riscv_cap_equal_exact : RVSDNode<"CAP_EQUAL_EXACT",
                                   SDT_RISCVCheriBoolBinary>;
def riscv_cllc            : RVSDNode<"CLLC", SDT_RISCVSymLoad>;
def riscv_clgc            : RVSDNode<"CLGC", SDT_RISCVSymLoad,
                                   [SDNPHasChain, SDNPMayLoad, SDNPMemOperand]>;
def riscv_cla_tls_ie      : RVSDNode<"CLA_TLS_IE", SDT_RISCVSymLoad,
                                   [SDNPHasChain, SDNPMayLoad, SDNPMemOperand]>;
def riscv_clc_tls_gd      : RVSDNode<"CLC_TLS_GD", SDT_RISCVSymLoad>;
def riscv_cap_bounds_set  : RVSDNode<"BOUNDS_SET",
                                   SDTypeProfile<1, 2, [SDTCisVT<0, YLenVT>,
                                                 SDTCisVT<1, YLenVT>,
                                                 SDTCisVT<2, XLenVT>]>>;

//===----------------------------------------------------------------------===//
// Operand and SDNode transformation definitions.
//===----------------------------------------------------------------------===//

def cap_call_symbol : Operand<YLenVT> {
  let ParserMatchClass = CallSymbol;
}

def pseudo_cap_jump_symbol : Operand<YLenVT> {
  let ParserMatchClass = PseudoJumpSymbol;
}

def CheriZeroOffsetMemOpOperand : AsmOperandClass {
  let Name = "CheriZeroOffsetMemOpOperand";
  let RenderMethod = "addRegOperands";
  let PredicateMethod = "isYGPR";
  let ParserMethod = "parseZeroOffsetMemOp";
}

def YGPRMemZeroOffset : RegisterOperand<YGPR> {
  let ParserMatchClass = CheriZeroOffsetMemOpOperand;
  let PrintMethod = "printZeroOffsetMemOp";
}

// A 9-bit unsigned immediate where the least significant four bits are zero.
def uimm9_lsb0000 : Operand<XLenVT>,
                    ImmLeaf<XLenVT, [{return isShiftedUInt<5, 4>(Imm);}]> {
  let ParserMatchClass = UImmAsmOperand<9, "Lsb0000">;
  let EncoderMethod = "getImmOpValue";
  let DecoderMethod = "decodeUImmOperand<9>";
  let MCOperandPredicate = [{
    int64_t Imm;
    if (!MCOp.evaluateAsConstantImm(Imm))
      return false;
    return isShiftedUInt<5, 4>(Imm);
  }];
}

// A 10-bit unsigned immediate where the least significant four bits are zero.
def uimm10_lsb0000 : Operand<XLenVT>,
                     ImmLeaf<XLenVT, [{return isShiftedUInt<6, 4>(Imm);}]> {
  let ParserMatchClass = UImmAsmOperand<10, "Lsb0000">;
  let EncoderMethod = "getImmOpValue";
  let DecoderMethod = "decodeUImmOperand<10>";
  let MCOperandPredicate = [{
    int64_t Imm;
    if (!MCOp.evaluateAsConstantImm(Imm))
      return false;
    return isShiftedUInt<6, 4>(Imm);
  }];
}

def CapFrameAddrRegImm : ComplexPattern<cPTR, 2,
                                        "SelectCapFrameAddrRegImm",
                                        [frameindex, cptradd]>;
def CapRegImm : ComplexPattern<cPTR, 2, "SelectCapRegImm">;

//===----------------------------------------------------------------------===//
// Instruction Formats
//===----------------------------------------------------------------------===//

include "RISCVInstrFormatsXCheri.td"

//===----------------------------------------------------------------------===//
// Instruction Class Templates
//===----------------------------------------------------------------------===//

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class Cheri_r<bits<5> funct5, string opcodestr, RegisterClass rdClass=GPR,
              DAGOperand rs1Operand=YGPR>
    : RVInstCheriSrcDst<0x7f, funct5, 0, OPC_CHERI, (outs rdClass:$rd),
                        (ins rs1Operand:$rs1), opcodestr, "$rd, $rs1">;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class Cheri_rr<bits<7> funct7, string opcodestr, RegisterClass rdClass=YGPR,
               RegisterClass rs2Class=GPR, RegisterClass rs1Class=YGPR>
    : RVInstR<funct7, 0, OPC_CHERI, (outs rdClass:$rd),
              (ins rs1Class:$rs1, rs2Class:$rs2),
              opcodestr, "$rd, $rs1, $rs2">;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class Cheri_ri<bits<3> funct3, string opcodestr, bit simm>
    : RVInstI<funct3, OPC_CHERI, (outs YGPR:$rd),
              (ins YGPR:$rs1, !if(simm,simm12_lo,uimm12):$imm12),
              opcodestr, "$rd, $rs1, $imm12">;

let hasSideEffects = 1, mayLoad = 0, mayStore = 0 in
class Cheri_clear<bits<5> funct5, string opcodestr>
    : RVInstCheriClear<0x7f, funct5, 0, OPC_CHERI, (outs),
                       (ins uimm2:$quarter, uimm8:$mask),
                       opcodestr, "$quarter, $mask">;

let hasSideEffects = 0, mayLoad = 1, mayStore = 0 in
class CheriLoad_r<bits<5> op, string opcodestr, RegisterClass rdClass,
                  RegisterOperand rs1Operand>
    : RVInstCheriSrcDst<0x7d, op, 0, OPC_CHERI, (outs rdClass:$rd),
                        (ins rs1Operand:$rs1), opcodestr, "$rd, $rs1">;

let hasSideEffects = 0, mayLoad = 0, mayStore = 1 in
class CheriStore_r<bits<5> op, string opcodestr, RegisterClass rs2Class,
                   RegisterOperand rs1Operand>
    : RVInstCheriTwoSrc<0x7c, op, 0, OPC_CHERI, (outs),
                        (ins rs2Class:$rs2, rs1Operand:$rs1),
                        opcodestr, "$rs2, $rs1">;

let hasSideEffects = 0, mayLoad = 0, mayStore = 1, Constraints = "$rd = $rs2" in
class CheriStoreCond_r<bits<5> op, string opcodestr, RegisterClass rs2Class,
                       RegisterOperand rs1Operand>
    : RVInstCheriTwoSrc<0x7c, op, 0, OPC_CHERI, (outs rs2Class:$rd),
                        (ins rs2Class:$rs2, rs1Operand:$rs1),
                        opcodestr, "$rs2, $rs1">;

let hasSideEffects = 0, mayLoad = 1, mayStore = 0 in
class LR_C_r<bit aq, bit rl, bits<3> funct3, string opcodestr>
    : RVInstRAtomic<0b00010, aq, rl, funct3, OPC_AMO,
                    (outs YGPR:$rd), (ins GPRMemZeroOffset:$rs1),
                    opcodestr, "$rd, $rs1"> {
  let rs2 = 0;
}

multiclass LR_C_r_aq_rl<string YLenstr, bits<3> funct3, string opcodestr> {
  def _ # YLenstr       : LR_C_r<0, 0, funct3, opcodestr>;
  def _AQ_ # YLenstr    : LR_C_r<1, 0, funct3, opcodestr # ".aq">;
  def _RL_ # YLenstr    : LR_C_r<0, 1, funct3, opcodestr # ".rl">;
  def _AQ_RL_ # YLenstr : LR_C_r<1, 1, funct3, opcodestr # ".aqrl">;
}

let hasSideEffects = 0, mayLoad = 1, mayStore = 1 in
class AMO_C_rr<bits<5> funct5, bit aq, bit rl, bits<3> funct3, string opcodestr,
               RegisterClass rdClass>
    : RVInstRAtomic<funct5, aq, rl, funct3, OPC_AMO,
                    (outs rdClass:$rd), (ins YGPR:$rs2, GPRMemZeroOffset:$rs1),
                    opcodestr, "$rd, $rs2, $rs1">;

multiclass AMO_C_rr_aq_rl<string YLenstr, bits<5> funct5, bits<3> funct3,
                          string opcodestr, RegisterClass rdClass> {
  def _ # YLenstr       : AMO_C_rr<funct5, 0, 0, funct3, opcodestr, rdClass>;
  def _AQ_ # YLenstr    : AMO_C_rr<funct5, 1, 0, funct3, opcodestr # ".aq",
                                   rdClass>;
  def _RL_ # YLenstr    : AMO_C_rr<funct5, 0, 1, funct3, opcodestr # ".rl",
                                   rdClass>;
  def _AQ_RL_ # YLenstr : AMO_C_rr<funct5, 1, 1, funct3, opcodestr # ".aqrl",
                                   rdClass>;
}

multiclass CheriLoad_ri<bits<3> funct3, string opcodestr> {
  let hasSideEffects = 0, mayLoad = 1, mayStore = 0, canFoldAsLoad = 1, isReMaterializable = 1,
      DecoderNamespace = "CapModeOnly_" in {
  def "" : RVInstI<funct3, OPC_LOAD, (outs GPR:$rd),
                   (ins YGPR:$rs1, simm12_lo:$imm12), "c"#opcodestr,
                   "$rd, ${imm12}(${rs1})">;
  }
  def : InstAlias<opcodestr#" $rd, ${imm12}(${rs1})",
                  (!cast<Instruction>(NAME) GPR:$rd, YGPR:$rs1, simm12_lo:$imm12),
                  0>;
}

multiclass CheriStore_ri<bits<3> funct3, string opcodestr> {
  let hasSideEffects = 0, mayLoad = 0, mayStore = 1,
      DecoderNamespace = "CapModeOnly_" in
  def "" : RVInstS<funct3, OPC_STORE, (outs),
                   (ins GPR:$rs2, YGPR:$rs1, simm12_lo:$imm12),
                   "c" # opcodestr, "$rs2, ${imm12}(${rs1})">;
  def : InstAlias<opcodestr # "$rs2, ${imm12}(${rs1})",
                  (!cast<Instruction>(NAME) GPR:$rs2, YGPR:$rs1, simm12_lo:$imm12),
                  0>;
}


multiclass CLR_r<bit aq, bit rl, bits<3> funct3, string opcodestr,
                 string Namespace> {
  let hasSideEffects = 0, mayLoad = 1, mayStore = 0,
      DecoderNamespace = Namespace in
  def "" : RVInstRAtomic<0b00010, aq, rl, funct3, OPC_AMO,
                         (outs GPR:$rd), (ins YGPRMemZeroOffset:$rs1),
                         "c" # opcodestr, "$rd, $rs1"> {
    let rs2 = 0;
  }
  def : InstAlias<opcodestr # "$rd, $rs1",
                  (!cast<Instruction>(NAME) GPR:$rd, YGPRMemZeroOffset:$rs1),
                  0>;
}

multiclass CLR_r_aq_rl<bits<3> funct3, string opcodestr,
                       string Namespace = "CapModeOnly_"> {
  defm ""     : CLR_r<0, 0, funct3, opcodestr, Namespace>;
  defm _AQ    : CLR_r<1, 0, funct3, opcodestr # ".aq", Namespace>;
  defm _RL    : CLR_r<0, 1, funct3, opcodestr # ".rl", Namespace>;
  defm _AQ_RL : CLR_r<1, 1, funct3, opcodestr # ".aqrl", Namespace>;
}

multiclass CAMO_rr<bits<5> funct5, bit aq, bit rl, bits<3> funct3,
                   string opcodestr, string Namespace> {
  let hasSideEffects = 0, mayLoad = 1, mayStore = 1,
      DecoderNamespace = Namespace in
  def "" : RVInstRAtomic<funct5, aq, rl, funct3, OPC_AMO,
                         (outs GPR:$rd), (ins GPR:$rs2, YGPRMemZeroOffset:$rs1),
                         "c" # opcodestr, "$rd, $rs2, $rs1">;
  def : InstAlias<opcodestr # "$rd, $rs2, $rs1",
                  (!cast<Instruction>(NAME) GPR:$rd, GPR:$rs2, YGPRMemZeroOffset:$rs1), 0>;
}

multiclass CAMO_rr_aq_rl<bits<5> funct5, bits<3> funct3, string opcodestr,
                         string Namespace = "CapModeOnly_"> {
  defm ""     : CAMO_rr<funct5, 0, 0, funct3, opcodestr, Namespace>;
  defm _AQ    : CAMO_rr<funct5, 1, 0, funct3, opcodestr # ".aq", Namespace>;
  defm _RL    : CAMO_rr<funct5, 0, 1, funct3, opcodestr # ".rl", Namespace>;
  defm _AQ_RL : CAMO_rr<funct5, 1, 1, funct3, opcodestr # ".aqrl", Namespace>;
}

multiclass CLR_C_r<bit aq, bit rl, bits<3> funct3, string opcodestr,
                   string Namespace> {
let hasSideEffects = 0, mayLoad = 1, mayStore = 0,
    DecoderNamespace = Namespace in
  def "" : RVInstRAtomic<0b00010, aq, rl, funct3, OPC_AMO,
                         (outs YGPR:$rd), (ins YGPRMemZeroOffset:$rs1),
                         "c" # opcodestr, "$rd, $rs1"> {
    let rs2 = 0;
  }
  def : InstAlias<opcodestr # "$rd, $rs1",
                  (!cast<Instruction>(NAME) YGPR:$rd, YGPRMemZeroOffset:$rs1),
                  0>;
}

multiclass CLR_C_r_aq_rl<string YLenstr, bits<3> funct3, string opcodestr,
                         string Namespace = "CapModeOnly_"> {
  defm _ # YLenstr       : CLR_C_r<0, 0, funct3, opcodestr, Namespace>;
  defm _AQ_ # YLenstr    : CLR_C_r<1, 0, funct3, opcodestr # ".aq", Namespace>;
  defm _RL_ # YLenstr    : CLR_C_r<0, 1, funct3, opcodestr # ".rl", Namespace>;
  defm _AQ_RL_ # YLenstr : CLR_C_r<1, 1, funct3, opcodestr # ".aqrl", Namespace>;
}

multiclass CAMO_C_rr<bits<5> funct5, bit aq, bit rl, bits<3> funct3,
                string opcodestr, RegisterClass rdClass, string Namespace> {
  let hasSideEffects = 0, mayLoad = 1, mayStore = 1,
      DecoderNamespace = Namespace in
  def "" : RVInstRAtomic<funct5, aq, rl, funct3, OPC_AMO,
                         (outs rdClass:$rd),
                         (ins YGPR:$rs2, YGPRMemZeroOffset:$rs1),
                         "c" # opcodestr, "$rd, $rs2, $rs1">;
  def : InstAlias<opcodestr # "$rd, $rs2, $rs1",
                 (!cast<Instruction>(NAME) rdClass:$rd, YGPR:$rs2, YGPRMemZeroOffset:$rs1), 0>;
}

multiclass CAMO_C_rr_aq_rl<string YLenstr, bits<5> funct5, bits<3> funct3,
                          string opcodestr, RegisterClass rdClass,
                          string Namespace = "CapModeOnly_"> {
  defm _ # YLenstr       : CAMO_C_rr<funct5, 0, 0, funct3, opcodestr, rdClass,
                                     Namespace>;
  defm _AQ_ # YLenstr    : CAMO_C_rr<funct5, 1, 0, funct3, opcodestr # ".aq",
                                     rdClass, Namespace>;
  defm _RL_ # YLenstr    : CAMO_C_rr<funct5, 0, 1, funct3, opcodestr # ".rl",
                                     rdClass, Namespace>;
  defm _AQ_RL_ # YLenstr : CAMO_C_rr<funct5, 1, 1, funct3, opcodestr # ".aqrl",
                                     rdClass, Namespace>;
}

let hasSideEffects = 0, mayLoad = 1, mayStore = 0 in
class CCheriStackLoad<bits<3> funct3, string OpcodeStr,
                      RegisterClass cls, DAGOperand opnd>
    : RVInst16CI<funct3, 0b10, (outs cls:$rd), (ins CSP:$rs1, opnd:$imm),
                 OpcodeStr, "$rd, ${imm}(${rs1})"> {
  bits<0> rs1;
}

let hasSideEffects = 0, mayLoad = 0, mayStore = 1 in
class CCheriStackStore<bits<3> funct3, string OpcodeStr,
                       RegisterClass cls, DAGOperand opnd>
    : RVInst16CSS<funct3, 0b10, (outs), (ins cls:$rs2, CSP:$rs1, opnd:$imm),
                  OpcodeStr, "$rs2, ${imm}(${rs1})"> {
  bits<0> rs1;
}

let hasSideEffects = 0, mayLoad = 1, mayStore = 0, canFoldAsLoad = 1, isReMaterializable = 1 in
class CCheriLoad_ri<bits<3> funct3, string OpcodeStr,
                    RegisterClass cls, DAGOperand opnd>
    : RVInst16CL<funct3, 0b00, (outs cls:$rd), (ins YGPRC:$rs1, opnd:$imm),
                 OpcodeStr, "$rd, ${imm}(${rs1})">;

let hasSideEffects = 0, mayLoad = 0, mayStore = 1 in
class CCheriStore_rri<bits<3> funct3, string OpcodeStr,
                      RegisterClass cls, DAGOperand opnd>
    : RVInst16CS<funct3, 0b00, (outs), (ins cls:$rs2, YGPRC:$rs1, opnd:$imm),
                 OpcodeStr, "$rs2, ${imm}(${rs1})">;

//===----------------------------------------------------------------------===//
// Capability-Inspection Instructions
//===----------------------------------------------------------------------===//

let Predicates = [HasCheri] in {
def CGetPerm   : Cheri_r<0x0, "cgetperm">;
def CGetType   : Cheri_r<0x1, "cgettype">;
def CGetBase   : Cheri_r<0x2, "cgetbase">;
def CGetLen    : Cheri_r<0x3, "cgetlen">;
let mayLoad = true in
  def CGetTag    : Cheri_r<0x4, "cgettag">;
def CGetSealed : Cheri_r<0x5, "cgetsealed">;
def CGetFlags  : Cheri_r<0x7, "cgetflags">;
def CGetHigh   : Cheri_r<0x17, "cgethigh">;
def CGetTop    : Cheri_r<0x18, "cgettop">;
// For backwards compatibility we still accept cgetaddr from assembly.
let mayStore = false, mayLoad = false, Size = 4, isCodeGenOnly = false,
hasSideEffects = false in
def PseudoCGetAddr : Pseudo<(outs GPR:$rd), (ins YGPR:$cs1), [],
                            "cgetaddr", "$rd, $cs1">;
}

let Predicates = [HasCheri, HasNoCheriot] in {
  def CGetOffset : Cheri_r<0x6, "cgetoffset">;
}

let Predicates = [HasCheriot] in {
  def : InstAlias<"ct.cgetperm $rd, $rs1", (CGetPerm GPR:$rd, YGPR:$rs1)>;
  def : InstAlias<"ct.cgettype $rd, $rs1", (CGetType GPR:$rd, YGPR:$rs1)>;
  def : InstAlias<"ct.cgetbase $rd, $rs1", (CGetBase GPR:$rd, YGPR:$rs1)>;
  def : InstAlias<"ct.cgetlen $rd, $rs1", (CGetLen GPR:$rd, YGPR:$rs1)>;
  def : InstAlias<"ct.cgettag $rd, $rs1", (CGetTag GPR:$rd, YGPR:$rs1)>;
  def : InstAlias<"ct.cgethigh $rd, $rs1", (CGetHigh GPR:$rd, YGPR:$rs1)>;
  def : InstAlias<"ct.cgettop $rd, $rs1", (CGetTop GPR:$rd, YGPR:$rs1)>;
  def : InstAlias<"ct.cgetaddr $rd, $rs1", (PseudoCGetAddr GPR:$rd, YGPR:$rs1)>;
}

//===----------------------------------------------------------------------===//
// Capability-Modification Instructions
//===----------------------------------------------------------------------===//

let Predicates = [HasCheri] in {
let defsCanBeSealed = 1 in
def CSeal           : Cheri_rr<0xb, "cseal", YGPR, YGPR>;
def CUnseal         : Cheri_rr<0xc, "cunseal", YGPR, YGPR>;
def CAndPerm        : Cheri_rr<0xd, "candperm">;
def CSetFlags       : Cheri_rr<0xe, "csetflags">;
def CSetOffset      : Cheri_rr<0xf, "csetoffset">;
def CSetAddr        : Cheri_rr<0x10, "csetaddr">;
def CSetHigh        : Cheri_rr<0x16, "csethigh">;
let isReMaterializable = 1, isAsCheapAsAMove = 1 in
def CIncOffset      : Cheri_rr<0x11, "cincoffset">;
let isReMaterializable = 1, isAsCheapAsAMove = 1 in
def CIncOffsetImm   : Cheri_ri<0x1, "cincoffset", 1>;
def CSetBounds      : Cheri_rr<0x8, "csetbounds">;
def CSetBoundsExact : Cheri_rr<0x9, "csetboundsexact">;
def CSetBoundsImm   : Cheri_ri<0x2, "csetbounds", 0>;
def CSetBoundsRoundDown : Cheri_rr<0xA, "csetboundsrounddown">;
def CClearTag       : Cheri_r<0xb, "ccleartag", YGPR>;
let defsCanBeSealed = 1 in
def CBuildCap       : Cheri_rr<0x1d, "cbuildcap", YGPR, YGPR, YGPRX0IsDDC>;
def CCopyType       : Cheri_rr<0x1e, "ccopytype", YGPR, YGPR>;
let defsCanBeSealed = 1 in
def CCSeal          : Cheri_rr<0x1f, "ccseal", YGPR, YGPR>;
let defsCanBeSealed = 1 in
def CSealEntry      : Cheri_r<0x11, "csealentry", YGPR>;

def : InstAlias<"cincoffsetimm $cd, $cs1, $imm",
                (CIncOffsetImm YGPR:$cd, YGPR:$cs1, simm12_lo:$imm), 0>;
def : InstAlias<"csetboundsimm $cd, $cs1, $imm",
                (CSetBoundsImm YGPR:$cd, YGPR:$cs1, uimm12:$imm), 0>;
}

let Predicates = [HasCheriot] in {
  def : InstAlias<"ct.cseal $rd, $rs1, $rs2", (CSeal YGPR:$rd, YGPR:$rs1,
                                                  YGPR:$rs2)>;
  def : InstAlias<"ct.cunseal $rd, $rs1, $rs2", (CUnseal YGPR:$rd, YGPR:$rs1,
                                                    YGPR:$rs2)>;
  def : InstAlias<"ct.candperm $rd, $rs1, $rs2", (CAndPerm YGPR:$rd, YGPR:$rs1,
                                                     GPR:$rs2)>;
  def : InstAlias<"ct.csetaddr $rd, $rs1, $rs2", (CSetAddr YGPR:$rd, YGPR:$rs1,
                                                     GPR:$rs2)>;
  def : InstAlias<"ct.csethigh $rd, $rs1, $rs2", (CSetHigh YGPR:$rd, YGPR:$rs1,
                                                     GPR:$rs2)>;
  def : InstAlias<"ct.cincaddr $rd, $rs1, $rs2",
                  (CIncOffset YGPR:$rd, YGPR:$rs1, GPR:$rs2), 0>;
  def : InstAlias<"ct.cincoffset $rd, $rs1, $rs2", (CIncOffset YGPR:$rd,
                                                       YGPR:$rs1, GPR:$rs2)>;
  def : InstAlias<"ct.cincaddr $rd, $rs1, $rs2",
                  (CIncOffsetImm YGPR:$rd, YGPR:$rs1, simm12_lo:$rs2), 0>;
  def : InstAlias<"ct.cincoffset $rd, $rs1, $rs2", (CIncOffsetImm YGPR:$rd,
                                                       YGPR:$rs1, simm12_lo:$rs2)>;
  def : InstAlias<"ct.csetbounds $rd, $rs1, $rs2", (CSetBounds YGPR:$rd,
                                                       YGPR:$rs1, GPR:$rs2)>;
  def : InstAlias<"ct.csetboundsexact $rd, $rs1, $rs2",
                  (CSetBoundsExact YGPR:$rd, YGPR:$rs1, GPR:$rs2)>;
  def : InstAlias<"ct.csetbounds $rd, $rs1, $rs2", (CSetBoundsImm YGPR:$rd,
                                                       YGPR:$rs1, uimm12:$rs2)>;
  def : InstAlias<"ct.csetboundsrounddown $rd, $rs1, $rs2",
                  (CSetBoundsRoundDown YGPR:$rd, YGPR:$rs1, GPR:$rs2)>;
  def : InstAlias<"ct.ccleartag $rd, $rs1", (CClearTag YGPR:$rd, YGPR:$rs1)>;

  def : InstAlias<"ct.cincaddrimm $cd, $cs1, $imm",
                  (CIncOffsetImm YGPR:$cd, YGPR:$cs1, simm12_lo:$imm), 0>;
  def : InstAlias<"ct.cincoffsetimm $cd, $cs1, $imm",
                  (CIncOffsetImm YGPR:$cd, YGPR:$cs1, simm12_lo:$imm), 0>;
  def : InstAlias<"ct.csetboundsimm $cd, $cs1, $imm",
                  (CSetBoundsImm YGPR:$cd, YGPR:$cs1, uimm12:$imm), 0>;
}

//===----------------------------------------------------------------------===//
// Pointer-Arithmetic Instructions
//===----------------------------------------------------------------------===//

let Predicates = [HasCheri] in {
let hasSideEffects = 0, mayLoad = 0, mayStore = 0, isCodeGenOnly = 0,
    isAsmParserOnly = 1 in
def PseudoCSub : Pseudo<(outs GPR:$rd), (ins YGPR:$cs1, YGPR:$cs2), [],
                        "csub", "$rd, $cs1, $cs2">;
let isMoveReg = 1, isReMaterializable = 1, isAsCheapAsAMove = 1,
    defsCanBeSealed = 1 in
def CMove       : Cheri_r<0xa, "cmove", YGPR>;
}

let Predicates = [HasCheriot] in {
  def : InstAlias<"ct.cmove $rd, $rs1", (CMove YGPR:$rd, YGPR:$rs1)>;
  def : InstAlias<"ct.csub $rd, $rs1, $rs2", (PseudoCSub GPR:$rd, YGPR:$rs1,
                                                 YGPR:$rs2)>;
}

//===----------------------------------------------------------------------===//
// Control-Flow Instructions
//===----------------------------------------------------------------------===//

let Predicates = [HasCheri] in {
let isCall = 1, hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
def JALR_CAP : RVInstCheriSrcDst<0x7f, 0xc, 0, OPC_CHERI, (outs YGPR:$rd),
                                 (ins YGPR:$rs1), "jalr.cap", "$rd, $rs1">;

let isCall = 1, hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
def JALR_PCC : RVInstCheriSrcDst<0x7f, 0x14, 0, OPC_CHERI, (outs GPR:$rd),
                                 (ins GPR:$rs1), "jalr.pcc", "$rd, $rs1">;

let isCall = 1, hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
def CInvoke : RVInstCheriTwoSrc<0x7e, 0x1, 0, OPC_CHERI, (outs),
                                (ins YGPR:$rs1, YGPR:$rs2),
                                "cinvoke", "$rs1, $rs2">;

def : InstAlias<"jalr.cap $cs1", (JALR_CAP X1_Y, YGPR:$cs1), 1>;
def : InstAlias<"jr.cap $cs1",   (JALR_CAP X0_Y, YGPR:$cs1), 1>;
def : InstAlias<"ret.cap",       (JALR_CAP X0_Y,        X1_Y), 2>;

def : InstAlias<"jalr.pcc $cs1", (JALR_PCC X1, GPR:$cs1), 1>;
def : InstAlias<"jr.pcc $cs1",   (JALR_PCC X0, GPR:$cs1), 1>;
def : InstAlias<"ret.pcc",       (JALR_PCC X0,       X1), 2>;
}

//===----------------------------------------------------------------------===//
// Assertion Instructions
//===----------------------------------------------------------------------===//

let Predicates = [HasCheri] in {
def CTestSubset : Cheri_rr<0x20, "ctestsubset", GPR, YGPR, YGPRX0IsDDC>;
def CSEQX       : Cheri_rr<0x21, "csetequalexact", GPR, YGPR, YGPR>;

def : InstAlias<"cseqx $rd, $cs1, $cs2", (CSEQX GPR:$rd, YGPR:$cs1, YGPR:$cs2)>;
}

let Predicates = [HasCheriot] in {
  def : InstAlias<"ct.ctestsubset $rd, $rs1, $rs2",
                  (CTestSubset GPR:$rd, YGPRX0IsDDC:$rs1, YGPR:$rs2)>;
  def : InstAlias<"ct.csetequalexact $rd, $rs1, $rs2",
                  (CSEQX GPR:$rd, YGPR:$rs1, YGPR:$rs2), 0>;
  def : InstAlias<"ct.cseqx $rd, $cs1, $cs2",
                  (CSEQX GPR:$rd, YGPR:$cs1, YGPR:$cs2), 2>;
}

//===----------------------------------------------------------------------===//
// Special Capabilty Register Access Instructions
//===----------------------------------------------------------------------===//

let Predicates = [HasCheri] in {
let hasSideEffects = 1, mayLoad = 0, mayStore = 0 in
def CSpecialRW : RVInstCheriSCR<0x1, 0, OPC_CHERI, (outs YGPR:$rd),
                                (ins special_capreg:$imm5, YGPR:$rs1),
                                "cspecialrw", "$rd, $imm5, $rs1">;

def : InstAlias<"cspecialr $cd, $scr",
                (CSpecialRW YGPR:$cd, special_capreg:$scr,           X0_Y)>;
def : InstAlias<"cspecialw $scr, $cs",
                (CSpecialRW       X0_Y, special_capreg:$scr,  YGPRNoX0:$cs)>;
}

let Predicates = [HasCheriot] in {
  def : InstAlias<"ct.cspecialrw $rd, $imm5, $rs1",
                  (CSpecialRW YGPR:$rd, special_capreg:$imm5, YGPR:$rs1)>;
  def : InstAlias<"ct.cspecialr $cd, $scr",
                  (CSpecialRW YGPR:$cd, special_capreg:$scr, X0_Y), 2>;
  def : InstAlias<"ct.cspecialw $scr, $cs",
                  (CSpecialRW X0_Y, special_capreg:$scr,  YGPRNoX0:$cs), 2>;
}

//===----------------------------------------------------------------------===//
// Fast Register-Clearing Instructions
//===----------------------------------------------------------------------===//

let Predicates = [HasCheri], hasSideEffects = 1 in {
def CClear  : Cheri_clear<0xe, "cclear">;
def FPClear : Cheri_clear<0x10, "fpclear">;
}

//===----------------------------------------------------------------------===//
// Adjusting to Compressed Capability Precision Instructions
//===----------------------------------------------------------------------===//

let Predicates = [HasCheri] in {
def CRRL : Cheri_r<0x8, "croundrepresentablelength", GPR, GPR>;
def CRAM : Cheri_r<0x9, "crepresentablealignmentmask", GPR, GPR>;

def : InstAlias<"crrl $rd, $rs1", (CRRL GPR:$rd, GPR:$rs1)>;
def : InstAlias<"cram $rd, $rs1", (CRAM GPR:$rd, GPR:$rs1)>;
}

let Predicates = [HasCheriot] in {
  def : InstAlias<"ct.croundrepresentablelength $rd, $rs1", (CRRL GPR:$rd,
                                                                GPR:$rs1)>;
  def : InstAlias<"ct.crepresentablealignmentmask $rd, $rs1", (CRAM GPR:$rd,
                                                                  GPR:$rs1)>;

  def : InstAlias<"ct.crrl $rd, $rs1", (CRRL GPR:$rd, GPR:$rs1), 2>;
  def : InstAlias<"ct.cram $rd, $rs1", (CRAM GPR:$rd, GPR:$rs1), 2>;
}

//===----------------------------------------------------------------------===//
// Tag-Memory Access Instructions
//===----------------------------------------------------------------------===//

let Predicates = [HasCheri], hasSideEffects = 0, mayLoad = 1, mayStore = 0 in
def CLoadTags : Cheri_r<0x12, "cloadtags", GPR, YGPRMemZeroOffset>;

//===----------------------------------------------------------------------===//
// Memory-Access with Explicit Address Type Instructions
//===----------------------------------------------------------------------===//

let Predicates = [HasCheri] in {
def LB_DDC  : CheriLoad_r<0b00000, "lb.ddc",  GPR, GPRMemZeroOffset>;
def LH_DDC  : CheriLoad_r<0b00001, "lh.ddc",  GPR, GPRMemZeroOffset>;
def LW_DDC  : CheriLoad_r<0b00010, "lw.ddc",  GPR, GPRMemZeroOffset>;
def LBU_DDC : CheriLoad_r<0b00100, "lbu.ddc", GPR, GPRMemZeroOffset>;
def LHU_DDC : CheriLoad_r<0b00101, "lhu.ddc", GPR, GPRMemZeroOffset>;
}

let Predicates = [HasCheri, IsRV64] in {
def LWU_DDC : CheriLoad_r<0b00110, "lwu.ddc", GPR, GPRMemZeroOffset>;
def LD_DDC  : CheriLoad_r<0b00011, "ld.ddc",  GPR, GPRMemZeroOffset>;
}

let DecoderNamespace = "RISCV32Only_",
    Predicates = [HasCheri, IsRV32] in
def LC_DDC_64  : CheriLoad_r<0b00011, "lc.ddc", YGPR, GPRMemZeroOffset>;

let Predicates = [HasCheri, IsRV64] in
def LC_DDC_128 : CheriLoad_r<0b10111, "lc.ddc", YGPR, GPRMemZeroOffset>;

let Predicates = [HasCheri] in {
def SB_DDC  : CheriStore_r<0b00000, "sb.ddc", GPR, GPRMemZeroOffset>;
def SH_DDC  : CheriStore_r<0b00001, "sh.ddc", GPR, GPRMemZeroOffset>;
def SW_DDC  : CheriStore_r<0b00010, "sw.ddc", GPR, GPRMemZeroOffset>;
}

let Predicates = [HasCheri, IsRV64] in {
def SD_DDC  : CheriStore_r<0b00011, "sd.ddc", GPR, GPRMemZeroOffset>;
}

let DecoderNamespace = "RISCV32Only_",
    Predicates = [HasCheri, IsRV32] in
def SC_DDC_64  : CheriStore_r<0b00011, "sc.ddc", YGPR, GPRMemZeroOffset>;

let Predicates = [HasCheri, IsRV64] in
def SC_DDC_128 : CheriStore_r<0b00100, "sc.ddc", YGPR, GPRMemZeroOffset>;

let Predicates = [HasCheri] in {
def LB_CAP  : CheriLoad_r<0b01000, "lb.cap",  GPR, YGPRMemZeroOffset>;
def LH_CAP  : CheriLoad_r<0b01001, "lh.cap",  GPR, YGPRMemZeroOffset>;
def LW_CAP  : CheriLoad_r<0b01010, "lw.cap",  GPR, YGPRMemZeroOffset>;
def LBU_CAP : CheriLoad_r<0b01100, "lbu.cap", GPR, YGPRMemZeroOffset>;
def LHU_CAP : CheriLoad_r<0b01101, "lhu.cap", GPR, YGPRMemZeroOffset>;
}

let Predicates = [HasCheri, IsRV64] in {
def LWU_CAP : CheriLoad_r<0b01110, "lwu.cap", GPR, YGPRMemZeroOffset>;
def LD_CAP  : CheriLoad_r<0b01011, "ld.cap",  GPR, YGPRMemZeroOffset>;
}

let DecoderNamespace = "RISCV32Only_",
    Predicates = [HasCheri, IsRV32] in
def LC_CAP_64  : CheriLoad_r<0b01011, "lc.cap", YGPR, YGPRMemZeroOffset>;

let Predicates = [HasCheri, IsRV64] in
def LC_CAP_128 : CheriLoad_r<0b11111, "lc.cap", YGPR, YGPRMemZeroOffset>;

let Predicates = [HasCheri] in {
def SB_CAP  : CheriStore_r<0b01000, "sb.cap", GPR, YGPRMemZeroOffset>;
def SH_CAP  : CheriStore_r<0b01001, "sh.cap", GPR, YGPRMemZeroOffset>;
def SW_CAP  : CheriStore_r<0b01010, "sw.cap", GPR, YGPRMemZeroOffset>;
}

let Predicates = [HasCheri, IsRV64] in {
def SD_CAP  : CheriStore_r<0b01011, "sd.cap", GPR, YGPRMemZeroOffset>;
}

let DecoderNamespace = "RISCV32Only_",
    Predicates = [HasCheri, IsRV32] in
def SC_CAP_64  : CheriStore_r<0b01011, "sc.cap", YGPR, YGPRMemZeroOffset>;

let Predicates = [HasCheri, IsRV64] in
def SC_CAP_128 : CheriStore_r<0b01100, "sc.cap", YGPR, YGPRMemZeroOffset>;

let Predicates = [HasCheri, HasStdExtA] in {
def LR_B_DDC  : CheriLoad_r<0b10000, "lr.b.ddc",  GPR, GPRMemZeroOffset>;
def LR_H_DDC  : CheriLoad_r<0b10001, "lr.h.ddc",  GPR, GPRMemZeroOffset>;
def LR_W_DDC  : CheriLoad_r<0b10010, "lr.w.ddc",  GPR, GPRMemZeroOffset>;
}

let Predicates = [HasCheri, HasStdExtA, IsRV64] in
def LR_D_DDC  : CheriLoad_r<0b10011, "lr.d.ddc",  GPR, GPRMemZeroOffset>;

let DecoderNamespace = "RISCV32Only_",
    Predicates = [HasCheri, HasStdExtA, IsRV32] in
def LR_C_DDC_64  : CheriLoad_r<0b10011, "lr.c.ddc", YGPR, GPRMemZeroOffset>;

let Predicates = [HasCheri, HasStdExtA, IsRV64] in
def LR_C_DDC_128 : CheriLoad_r<0b10100, "lr.c.ddc", YGPR, GPRMemZeroOffset>;

let Predicates = [HasCheri, HasStdExtA] in {
def LR_B_CAP  : CheriLoad_r<0b11000, "lr.b.cap",  GPR, YGPRMemZeroOffset>;
def LR_H_CAP  : CheriLoad_r<0b11001, "lr.h.cap",  GPR, YGPRMemZeroOffset>;
def LR_W_CAP  : CheriLoad_r<0b11010, "lr.w.cap",  GPR, YGPRMemZeroOffset>;
}

let Predicates = [HasCheri, HasStdExtA, IsRV64] in
def LR_D_CAP  : CheriLoad_r<0b11011, "lr.d.cap",  GPR, YGPRMemZeroOffset>;

let DecoderNamespace = "RISCV32Only_",
    Predicates = [HasCheri, HasStdExtA, IsRV32] in
def LR_C_CAP_64  : CheriLoad_r<0b11011, "lr.c.cap", YGPR, YGPRMemZeroOffset>;

let Predicates = [HasCheri, HasStdExtA, IsRV64] in
def LR_C_CAP_128 : CheriLoad_r<0b11100, "lr.c.cap", YGPR, YGPRMemZeroOffset>;

let Predicates = [HasCheri, HasStdExtA] in {
def SC_B_DDC  : CheriStoreCond_r<0b10000, "sc.b.ddc",  GPR, GPRMemZeroOffset>;
def SC_H_DDC  : CheriStoreCond_r<0b10001, "sc.h.ddc",  GPR, GPRMemZeroOffset>;
def SC_W_DDC  : CheriStoreCond_r<0b10010, "sc.w.ddc",  GPR, GPRMemZeroOffset>;
}

let Predicates = [HasCheri, HasStdExtA, IsRV64] in
def SC_D_DDC  : CheriStoreCond_r<0b10011, "sc.d.ddc",  GPR, GPRMemZeroOffset>;

let DecoderNamespace = "RISCV32Only_",
    Predicates = [HasCheri, HasStdExtA, IsRV32] in
def SC_C_DDC_64  : CheriStoreCond_r<0b10011, "sc.c.ddc", YGPR, GPRMemZeroOffset>;

let Predicates = [HasCheri, HasStdExtA, IsRV64] in
def SC_C_DDC_128 : CheriStoreCond_r<0b10100, "sc.c.ddc", YGPR, GPRMemZeroOffset>;

let Predicates = [HasCheri, HasStdExtA] in {
def SC_B_CAP  : CheriStoreCond_r<0b11000, "sc.b.cap",  GPR, YGPRMemZeroOffset>;
def SC_H_CAP  : CheriStoreCond_r<0b11001, "sc.h.cap",  GPR, YGPRMemZeroOffset>;
def SC_W_CAP  : CheriStoreCond_r<0b11010, "sc.w.cap",  GPR, YGPRMemZeroOffset>;
}

let Predicates = [HasCheri, HasStdExtA, IsRV64] in
def SC_D_CAP  : CheriStoreCond_r<0b11011, "sc.d.cap",  GPR, YGPRMemZeroOffset>;

let DecoderNamespace = "RISCV32Only_",
    Predicates = [HasCheri, HasStdExtA, IsRV32] in
def SC_C_CAP_64  : CheriStoreCond_r<0b11011, "sc.c.cap", YGPR, YGPRMemZeroOffset>;

let Predicates = [HasCheri, HasStdExtA, IsRV64] in
def SC_C_CAP_128 : CheriStoreCond_r<0b11100, "sc.c.cap", YGPR, YGPRMemZeroOffset>;

//===----------------------------------------------------------------------===//
// Memory-Access Instructions
//===----------------------------------------------------------------------===//

let Predicates = [HasCheri, IsRV32, NotCapMode] in {
let DecoderNamespace = "RISCV32Only_",
    hasSideEffects = 0, mayLoad = 1, mayStore = 0 in
def LC_64  : RVInstI<0x3, OPC_LOAD, (outs YGPR:$rd),
                     (ins GPR:$rs1, simm12_lo:$imm12),
                     "lc", "$rd, ${imm12}(${rs1})">;

let DecoderNamespace = "RISCV32Only_",
    hasSideEffects = 0, mayLoad = 0, mayStore = 1 in
def SC_64  : RVInstS<0x3, OPC_STORE, (outs),
                     (ins YGPR:$rs2, GPR:$rs1, simm12_lo:$imm12),
                     "sc", "$rs2, ${imm12}(${rs1})">;

let EmitPriority = 0 in {
def : InstAlias<"lc $rd, (${rs1})",
                (LC_64  YGPR:$rd, GPR:$rs1, 0)>;
def : InstAlias<"sc $rs2, (${rs1})",
                (SC_64  YGPR:$rs2, GPR:$rs1, 0)>;
}
}

let Predicates = [HasCheri, IsRV64, NotCapMode] in {
let hasSideEffects = 0, mayLoad = 1, mayStore = 0 in
def LC_128  : RVInstI<0x2, OPC_MISC_MEM, (outs YGPR:$rd),
                      (ins GPR:$rs1, simm12_lo:$imm12),
                      "lc", "$rd, ${imm12}(${rs1})">;

let hasSideEffects = 0, mayLoad = 0, mayStore = 1 in
def SC_128  : RVInstS<0x4, OPC_STORE, (outs),
                      (ins YGPR:$rs2, GPR:$rs1, simm12_lo:$imm12),
                      "sc", "$rs2, ${imm12}(${rs1})">;

let EmitPriority = 0 in {
def : InstAlias<"lc $rd, (${rs1})",
                (LC_128  YGPR:$rd, GPR:$rs1, 0)>;
def : InstAlias<"sc $rs2, (${rs1})",
                (SC_128  YGPR:$rs2, GPR:$rs1, 0)>;
}
}

let DecoderNamespace = "RISCV32Only_",
    Predicates = [HasCheri, HasStdExtA, IsRV32, NotCapMode] in {
defm LR_C       : LR_C_r_aq_rl<"64", 0b011, "lr.c">;
defm SC_C       : AMO_C_rr_aq_rl<"64", 0b00011, 0b011, "sc.c", GPR>;
defm AMOSWAP_C  : AMO_C_rr_aq_rl<"64", 0b00001, 0b011, "amoswap.c", YGPR>;
}

let Predicates = [HasCheri, HasStdExtA, IsRV64, NotCapMode] in {
defm LR_C       : LR_C_r_aq_rl<"128", 0b100, "lr.c">;
defm SC_C       : AMO_C_rr_aq_rl<"128", 0b00011, 0b100, "sc.c", GPR>;
defm AMOSWAP_C  : AMO_C_rr_aq_rl<"128", 0b00001, 0b100, "amoswap.c", YGPR>;
}

//===----------------------------------------------------------------------===//
// Capability Mode Instructions
//===----------------------------------------------------------------------===//


/// 'I' (Integer) base

let DecoderNamespace = "CapModeOnly_" in {
let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in {
let Predicates = [HasCheri, IsCapMode] in {
def AUIPCC : RVInstU<OPC_AUIPC, (outs YGPR:$rd), (ins uimm20_auipc:$imm20),
                     "auipcc", "$rd, $imm20">;

let Uses = [X3_Y] in {
def AUICGP : RVInstU<OPC_AUIGP, (outs YGPR:$rd), (ins uimm20_auigp:$imm20),
                     "auicgp", "$rd, $imm20">;
}

let isCall = 1 in
def CJAL : RVInstJ<OPC_JAL, (outs YGPR:$rd), (ins simm21_lsb0_jal:$imm20),
                   "cjal", "$rd, $imm20">;

let isCall = 1 in
def CJALR : RVInstI<0b000, OPC_JALR, (outs YGPR:$rd),
                    (ins YGPR:$rs1, simm12_lo:$imm12),
                    "cjalr", "$rd, ${imm12}(${rs1})">;
} // Predicates = [HasCheri, IsCapMode]
} // hasSideEffects = 0, mayLoad = 0, mayStore = 0
} // DecoderNameSpace = "CapModeOnly_"

let Predicates = [HasCheri, IsCapMode] in {
def : InstAlias<"j $offset",    (CJAL X0_Y, simm21_lsb0_jal:$offset)>;
def : InstAlias<"cjal $offset", (CJAL X1_Y, simm21_lsb0_jal:$offset)>;

// Non-canonical form; JAL X0 and CJAL X0 are semantically identical so "j" is
// the canonical form, but we provide this for completeness.
def : InstAlias<"cj $offset",   (CJAL X0_Y, simm21_lsb0_jal:$offset), 0>;

// Non-zero offset aliases of "cjalr" are the lowest weight, followed by the
// two-register form, then the one-register forms and finally "cret".
def : InstAlias<"cjr $rs",                (CJALR       X0_Y, YGPR:$rs, 0), 3>;
def : InstAlias<"cjr ${offset}(${rs})",   (CJALR       X0_Y, YGPR:$rs, simm12_lo:$offset)>;
def : InstAlias<"cjalr $rs",              (CJALR       X1_Y, YGPR:$rs, 0), 3>;
def : InstAlias<"cjalr ${offset}(${rs})", (CJALR       X1_Y, YGPR:$rs, simm12_lo:$offset)>;
def : InstAlias<"cjalr $rd, $rs",         (CJALR YGPR:$rd, YGPR:$rs, 0), 2>;
def : InstAlias<"cret",                   (CJALR       X0_Y,       X1_Y, 0), 4>;

// Non-canonical forms for jump targets also accepted by the assembler.
def : InstAlias<"cjr $rs, $offset",        (CJALR       X0_Y, YGPR:$rs, simm12_lo:$offset), 0>;
def : InstAlias<"cjalr $rs, $offset",      (CJALR       X1_Y, YGPR:$rs, simm12_lo:$offset), 0>;
def : InstAlias<"cjalr $rd, $rs, $offset", (CJALR YGPR:$rd, YGPR:$rs, simm12_lo:$offset), 0>;
} // Predicates = [HasCheri, IsCapMode]

let Predicates = [HasCheriot] in {
  def : InstAlias<"ct.auipcc $rd, $imm20", (AUIPCC YGPR:$rd,
                                               uimm20_auipc:$imm20)>;
  def : InstAlias<"ct.auicgp $rd, $imm20", (AUICGP YGPR:$rd,
                                               uimm20_auigp:$imm20)>;
  def : InstAlias<"ct.cjal $rd, $imm20", (CJAL YGPR:$rd,
                                             simm21_lsb0_jal:$imm20)>;
  def : InstAlias<"ct.cjalr $rd, ${imm12}(${rs1})", (CJALR YGPR:$rd, YGPR:$rs1,
                                                        simm12_lo:$imm12)>;

  def : InstAlias<"ct.cjal $offset", (CJAL X1_Y, simm21_lsb0_jal:$offset), 2>;
  def : InstAlias<"ct.cj $offset", (CJAL X0_Y, simm21_lsb0_jal:$offset), 0>;

  def : InstAlias<"ct.cjr $rs", (CJALR X0_Y, YGPR:$rs, 0), 8>;
  def : InstAlias<"ct.cjr ${offset}(${rs})",
                  (CJALR X0_Y, YGPR:$rs, simm12_lo:$offset), 5>;
  def : InstAlias<"ct.cjalr $rs", (CJALR X1_Y, YGPR:$rs, 0), 8>;
  def : InstAlias<"ct.cjalr ${offset}(${rs})",
                  (CJALR X1_Y, YGPR:$rs, simm12_lo:$offset), 5>;
  def : InstAlias<"ct.cjalr $rd, $rs", (CJALR YGPR:$rd, YGPR:$rs, 0), 7>;
  def : InstAlias<"ct.cret", (CJALR X0_Y, X1_Y, 0), 9>;

  def : InstAlias<"ct.cjr $rs, $offset", (CJALR X0_Y, YGPR:$rs, simm12_lo:$offset),
                  0>;
  def : InstAlias<"ct.cjalr $rs, $offset", (CJALR X1_Y, YGPR:$rs, simm12_lo:$offset),
                  0>;
  def : InstAlias<"ct.cjalr $rd, $rs, $offset",
                  (CJALR YGPR:$rd, YGPR:$rs, simm12_lo:$offset), 0>;
}

// Expands to an instruction alias with and without a c prefix for loads/stores
multiclass CPrefixedInstAlias<string Asm, dag Result> {
  def : InstAlias<"c" # Asm, Result, 0>;
  def : InstAlias<Asm, Result, 0>;
}


let Predicates = [HasCheri, IsCapMode] in {
defm CLB  : CheriLoad_ri<0b000, "lb">;
defm CLH  : CheriLoad_ri<0b001, "lh">;
defm CLW  : CheriLoad_ri<0b010, "lw">;
defm CLBU : CheriLoad_ri<0b100, "lbu">;
defm CLHU : CheriLoad_ri<0b101, "lhu">;

defm CSB : CheriStore_ri<0b000, "sb">;
defm CSH : CheriStore_ri<0b001, "sh">;
defm CSW : CheriStore_ri<0b010, "sw">;
} // Predicates = [HasCheri, IsCapMode]

let Predicates = [HasCheriot] in {
  def : InstAlias<"ct.clb $rd, ${imm12}(${rs1})", (CLB GPR:$rd, YGPR:$rs1,
                                                      simm12_lo:$imm12)>;
  def : InstAlias<"ct.clh $rd, ${imm12}(${rs1})", (CLH GPR:$rd, YGPR:$rs1,
                                                      simm12_lo:$imm12)>;
  def : InstAlias<"ct.clw $rd, ${imm12}(${rs1})", (CLW GPR:$rd, YGPR:$rs1,
                                                      simm12_lo:$imm12)>;
  def : InstAlias<"ct.clbu $rd, ${imm12}(${rs1})", (CLBU GPR:$rd, YGPR:$rs1,
                                                       simm12_lo:$imm12)>;
  def : InstAlias<"ct.clhu $rd, ${imm12}(${rs1})", (CLHU GPR:$rd, YGPR:$rs1,
                                                       simm12_lo:$imm12)>;

  def : InstAlias<"ct.csb $rs2, ${imm12}(${rs1})", (CSB GPR:$rs2, YGPR:$rs1,
                                                       simm12_lo:$imm12)>;
  def : InstAlias<"ct.csh $rs2, ${imm12}(${rs1})", (CSH GPR:$rs2, YGPR:$rs1,
                                                       simm12_lo:$imm12)>;
  def : InstAlias<"ct.csw $rs2, ${imm12}(${rs1})", (CSW GPR:$rs2, YGPR:$rs1,
                                                       simm12_lo:$imm12)>;
}

let Predicates = [HasCheri, IsRV64, IsCapMode] in {
defm CLWU   : CheriLoad_ri<0b110, "lwu">;
defm CLD    : CheriLoad_ri<0b011, "ld">;
defm CSD    : CheriStore_ri<0b011, "sd">;
} // Predicates = [HasCheri, IsRV64, IsCapMode]

let Predicates = [HasCheri, IsRV32, IsCapMode] in {
  let DecoderNamespace = "RISCV32CapModeOnly_", hasSideEffects = 0,
      mayLoad = 1, mayStore = 0, canFoldAsLoad = 1, isReMaterializable = 1 in {
    def CLC_64 : RVInstI<0x3, OPC_LOAD, (outs YGPR:$rd), (ins YGPR:$rs1, simm12_lo:$imm12),
                        "clc", "$rd, ${imm12}(${rs1})">;
  }
  def : InstAlias<"lc $rd, ${imm12}(${rs1})",
                  (CLC_64 YGPR:$rd, YGPR:$rs1, simm12_lo:$imm12), 0>;

  let DecoderNamespace = "RISCV32CapModeOnly_", hasSideEffects = 0, mayLoad = 0, mayStore = 1 in {
    def CSC_64 : RVInstS<0x3, OPC_STORE, (outs), (ins YGPR:$rs2, YGPR:$rs1, simm12_lo:$imm12),
                        "csc", "$rs2, ${imm12}(${rs1})">;
  }
  def : InstAlias<"sc $rs2, ${imm12}(${rs1})",
                    (CSC_64 YGPR:$rs2, YGPR:$rs1, simm12_lo:$imm12), 0>;

  defm : CPrefixedInstAlias<"lc $rd, (${rs1})",
                            (CLC_64 YGPR:$rd, YGPR:$rs1, 0)>;
  defm : CPrefixedInstAlias<"sc $rs2, (${rs1})",
                            (CSC_64 YGPR:$rs2, YGPR:$rs1, 0)>;
}

let Predicates = [HasCheriot] in {
  def : InstAlias<"ct.clc $rd, ${imm12}(${rs1})", (CLC_64 YGPR:$rd, YGPR:$rs1,
                                                      simm12_lo:$imm12)>;
  def : InstAlias<"ct.csc $rd, ${imm12}(${rs1})", (CSC_64 YGPR:$rd, YGPR:$rs1,
                                                      simm12_lo:$imm12)>;
}

let Predicates = [HasCheri, IsRV64, IsCapMode] in {
  let DecoderNamespace = "CapModeOnly_", hasSideEffects = 0,
      mayLoad = 1, mayStore = 0, canFoldAsLoad = 1, isReMaterializable = 1 in {
    def CLC_128 : RVInstI<0x2, OPC_MISC_MEM, (outs YGPR:$rd), (ins YGPR:$rs1, simm12_lo:$imm12),
                          "clc", "$rd, ${imm12}(${rs1})">;
  }
  def : InstAlias<"lc $rd, ${imm12}(${rs1})",
                  (CLC_128 YGPR:$rd, YGPR:$rs1, simm12_lo:$imm12), 0>;

  let DecoderNamespace = "CapModeOnly_", hasSideEffects = 0, mayLoad = 0, mayStore = 1 in {
    def CSC_128 : RVInstS<0x4, OPC_STORE, (outs), (ins YGPR:$rs2, YGPR:$rs1, simm12_lo:$imm12),
                          "csc", "$rs2, ${imm12}(${rs1})">;
  }
  def : InstAlias<"sc $rs2, ${imm12}(${rs1})",
                  (CSC_128 YGPR:$rs2, YGPR:$rs1, simm12_lo:$imm12), 0>;

  defm : CPrefixedInstAlias<"lc $rd, (${rs1})",
                            (CLC_128 YGPR:$rd, YGPR:$rs1, 0)>;
  defm : CPrefixedInstAlias<"sc $rs2, (${rs1})",
                            (CSC_128 YGPR:$rs2, YGPR:$rs1, 0)>;
}

let Predicates = [HasCheri, IsCapMode] in {
defm : CPrefixedInstAlias<"lb $rd, (${rs1})", (CLB  GPR:$rd, YGPR:$rs1, 0)>;
defm : CPrefixedInstAlias<"lh $rd, (${rs1})", (CLH  GPR:$rd, YGPR:$rs1, 0)>;
defm : CPrefixedInstAlias<"lw $rd, (${rs1})", (CLW  GPR:$rd, YGPR:$rs1, 0)>;
defm : CPrefixedInstAlias<"lbu $rd, (${rs1})", (CLBU  GPR:$rd, YGPR:$rs1, 0)>;
defm : CPrefixedInstAlias<"lhu $rd, (${rs1})", (CLHU  GPR:$rd, YGPR:$rs1, 0)>;

defm : CPrefixedInstAlias<"sb $rs2, (${rs1})", (CSB  GPR:$rs2, YGPR:$rs1, 0)>;
defm : CPrefixedInstAlias<"sh $rs2, (${rs1})", (CSH  GPR:$rs2, YGPR:$rs1, 0)>;
defm : CPrefixedInstAlias<"sw $rs2, (${rs1})", (CSW  GPR:$rs2, YGPR:$rs1, 0)>;
} // Predicates = [HasCheri, IsCapMode]

let Predicates = [HasCheri, IsRV64, IsCapMode] in {
defm : CPrefixedInstAlias<"lwu $rd, (${rs1})", (CLWU  GPR:$rd, YGPR:$rs1, 0)>;
defm : CPrefixedInstAlias<"ld $rd, (${rs1})", (CLD  GPR:$rd, YGPR:$rs1, 0)>;
defm : CPrefixedInstAlias<"sd $rs2, (${rs1})", (CSD  GPR:$rs2, YGPR:$rs1, 0)>;
} // Predicates = [HasCheri, IsRV64, IsCapMode]

/// 'A' (Atomic Instructions) extension

let Predicates = [HasCheri, HasStdExtA, IsCapMode] in {
defm CLR_B       : CLR_r_aq_rl<0b000, "lr.b">;
defm CSC_B       : CAMO_rr_aq_rl<0b00011, 0b000, "sc.b">;

defm CLR_H       : CLR_r_aq_rl<0b001, "lr.h">;
defm CSC_H       : CAMO_rr_aq_rl<0b00011, 0b001, "sc.h">;

defm CLR_W       : CLR_r_aq_rl<0b010, "lr.w">;
defm CSC_W       : CAMO_rr_aq_rl<0b00011, 0b010, "sc.w">;
defm CAMOSWAP_W  : CAMO_rr_aq_rl<0b00001, 0b010, "amoswap.w">;
defm CAMOADD_W   : CAMO_rr_aq_rl<0b00000, 0b010, "amoadd.w">;
defm CAMOXOR_W   : CAMO_rr_aq_rl<0b00100, 0b010, "amoxor.w">;
defm CAMOAND_W   : CAMO_rr_aq_rl<0b01100, 0b010, "amoand.w">;
defm CAMOOR_W    : CAMO_rr_aq_rl<0b01000, 0b010, "amoor.w">;
defm CAMOMIN_W   : CAMO_rr_aq_rl<0b10000, 0b010, "amomin.w">;
defm CAMOMAX_W   : CAMO_rr_aq_rl<0b10100, 0b010, "amomax.w">;
defm CAMOMINU_W  : CAMO_rr_aq_rl<0b11000, 0b010, "amominu.w">;
defm CAMOMAXU_W  : CAMO_rr_aq_rl<0b11100, 0b010, "amomaxu.w">;
} // Predicates = [HasCheri, HasStdExtA, IsCapMode]

let Predicates = [HasCheri, HasStdExtA, IsRV64, IsCapMode] in {
defm CLR_D       : CLR_r_aq_rl<0b011, "lr.d">;
defm CSC_D       : CAMO_rr_aq_rl<0b00011, 0b011, "sc.d">;
defm CAMOSWAP_D  : CAMO_rr_aq_rl<0b00001, 0b011, "amoswap.d">;
defm CAMOADD_D   : CAMO_rr_aq_rl<0b00000, 0b011, "amoadd.d">;
defm CAMOXOR_D   : CAMO_rr_aq_rl<0b00100, 0b011, "amoxor.d">;
defm CAMOAND_D   : CAMO_rr_aq_rl<0b01100, 0b011, "amoand.d">;
defm CAMOOR_D    : CAMO_rr_aq_rl<0b01000, 0b011, "amoor.d">;
defm CAMOMIN_D   : CAMO_rr_aq_rl<0b10000, 0b011, "amomin.d">;
defm CAMOMAX_D   : CAMO_rr_aq_rl<0b10100, 0b011, "amomax.d">;
defm CAMOMINU_D  : CAMO_rr_aq_rl<0b11000, 0b011, "amominu.d">;
defm CAMOMAXU_D  : CAMO_rr_aq_rl<0b11100, 0b011, "amomaxu.d">;
} // Predicates = [HasCheri, HasStdExtA, IsRV64, IsCapMode]

let Predicates = [HasCheri, HasStdExtA, IsRV32, IsCapMode] in {
defm CLR_C       : CLR_C_r_aq_rl<"64", 0b011, "lr.c", "RISCV32CapModeOnly_">;
defm CSC_C       : CAMO_C_rr_aq_rl<"64", 0b00011, 0b011, "sc.c", GPR,
                                   "RISCV32CapModeOnly_">;
defm CAMOSWAP_C  : CAMO_C_rr_aq_rl<"64", 0b00001, 0b011, "amoswap.c", YGPR,
                                   "RISCV32CapModeOnly_">;
}

let Predicates = [HasCheri, HasStdExtA, IsRV64, IsCapMode] in {
defm CLR_C       : CLR_C_r_aq_rl<"128", 0b100, "lr.c">;
defm CSC_C       : CAMO_C_rr_aq_rl<"128", 0b00011, 0b100, "sc.c", GPR>;
defm CAMOSWAP_C  : CAMO_C_rr_aq_rl<"128", 0b00001, 0b100, "amoswap.c", YGPR>;
}

/// 'F' (Single-Precision Floating-Point) extension

let Predicates = [HasCheri, HasStdExtF, IsCapMode] in {
let DecoderNamespace = "CapModeOnly_",
    hasSideEffects = 0, mayLoad = 1, mayStore = 0 in
def CFLW : RVInstI<0b010, OPC_LOAD_FP, (outs FPR32:$rd),
                   (ins YGPR:$rs1, simm12_lo:$imm12),
                   "cflw", "$rd, ${imm12}(${rs1})">;

let DecoderNamespace = "CapModeOnly_",
    hasSideEffects = 0, mayLoad = 0, mayStore = 1 in
def CFSW : RVInstS<0b010, OPC_STORE_FP, (outs),
                   (ins FPR32:$rs2, YGPR:$rs1, simm12_lo:$imm12),
                   "cfsw", "$rs2, ${imm12}(${rs1})">;

defm : CPrefixedInstAlias<"flw $rd, (${rs1})", (CFLW FPR32:$rd, YGPR:$rs1, 0)>;
defm : CPrefixedInstAlias<"fsw $rs2, (${rs1})",
                          (CFSW FPR32:$rs2, YGPR:$rs1, 0)>;
def : InstAlias<"flw $rd, ${imm12}(${rs1})",
                (CFLW FPR32:$rd, YGPR:$rs1, simm12_lo:$imm12), 0>;
def : InstAlias<"fsw $rs2, ${imm12}(${rs1})",
                (CFSW FPR32:$rs2, YGPR:$rs1, simm12_lo:$imm12), 0>;
} // Predicates = [HasCheri, HasStdExtF, IsCapMode]

/// 'D' (Single-Precision Floating-Point) extension

let Predicates = [HasCheri, HasStdExtD, IsCapMode] in {
let DecoderNamespace = "CapModeOnly_",
    hasSideEffects = 0, mayLoad = 1, mayStore = 0 in
def CFLD : RVInstI<0b011, OPC_LOAD_FP, (outs FPR64:$rd),
                   (ins YGPR:$rs1, simm12_lo:$imm12),
                   "cfld", "$rd, ${imm12}(${rs1})">;

let DecoderNamespace = "CapModeOnly_",
    hasSideEffects = 0, mayLoad = 0, mayStore = 1 in
def CFSD : RVInstS<0b011, OPC_STORE_FP, (outs),
                   (ins FPR64:$rs2, YGPR:$rs1, simm12_lo:$imm12),
                   "cfsd", "$rs2, ${imm12}(${rs1})">;

defm : CPrefixedInstAlias<"fld $rd, (${rs1})", (CFLD FPR64:$rd, YGPR:$rs1, 0)>;
defm : CPrefixedInstAlias<"fsd $rs2, (${rs1})",
                          (CFSD FPR64:$rs2, YGPR:$rs1, 0)>;
def : InstAlias<"fld $rd, ${imm12}(${rs1})",
                (CFLD FPR64:$rd, YGPR:$rs1, simm12_lo:$imm12), 0>;
def : InstAlias<"fsd $rs2, ${imm12}(${rs1})",
                (CFSD FPR64:$rs2, YGPR:$rs1, simm12_lo:$imm12), 0>;
} // Predicates = [HasCheri, HasStdExtD, IsCapMode]

/// 'C' (Compressed Instructions) extension

let DecoderNamespace = "CapModeOnly_" in {
let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode] in {

let hasSideEffects = 0, mayLoad = 0, mayStore = 0, Uses = [X2_Y] in
def C_CIncOffsetImm4CSPN : RVInst16CIW<0b000, 0b00, (outs YGPRC:$rd),
                                       (ins CSP:$rs1, uimm10_lsb00nonzero:$imm),
                                       "c.cincoffset4cspn", "$rd, $rs1, $imm"> {
  bits<0> rs1;
  let Inst{12-11} = imm{5-4};
  let Inst{10-7} = imm{9-6};
  let Inst{6} = imm{2};
  let Inst{5} = imm{3};
}

let DecoderNamespace = "RISCV32CapModeOnly_",
    Predicates = [HasCheri, HasCheriRVC, HasStdExtC, HasStdExtD, IsRV32, IsCapMode] in
def C_CFLD : CCheriLoad_ri<0b001, "c.cfld", FPR64C, uimm8_lsb000> {
  bits<8> imm;
  let Inst{12-10} = imm{5-3};
  let Inst{6-5} = imm{7-6};
}

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode] in
def C_CLC_128 : CCheriLoad_ri<0b001, "c.clc", YGPRC, uimm9_lsb0000> {
  bits<9> imm;
  let Inst{12-11} = imm{5-4};
  let Inst{10} = imm{8};
  let Inst{6-5} = imm{7-6};
}

def C_CLW : CCheriLoad_ri<0b010, "c.clw", GPRC, uimm7_lsb00> {
  bits<7> imm;
  let Inst{12-10} = imm{5-3};
  let Inst{6} = imm{2};
  let Inst{5} = imm{6};
}

let DecoderNamespace = "RISCV32CapModeOnly_",
    Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV32, IsCapMode] in
def C_CLC_64 : CCheriLoad_ri<0b011, "c.clc", YGPRC, uimm8_lsb000> {
  bits<8> imm;
  let Inst{12-10} = imm{5-3};
  let Inst{6-5} = imm{7-6};
}

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode] in
def C_CLD : CCheriLoad_ri<0b011, "c.cld", GPRC, uimm8_lsb000> {
  bits<8> imm;
  let Inst{12-10} = imm{5-3};
  let Inst{6-5} = imm{7-6};
}

let DecoderNamespace = "RISCV32CapModeOnly_",
    Predicates = [HasCheri, HasCheriRVC, HasStdExtC, HasStdExtD, IsRV32, IsCapMode] in
def C_CFSD : CCheriStore_rri<0b101, "c.cfsd", FPR64C, uimm8_lsb000> {
  bits<8> imm;
  let Inst{12-10} = imm{5-3};
  let Inst{6-5} = imm{7-6};
}

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode] in
def C_CSC_128 : CCheriStore_rri<0b101, "c.csc", YGPRC, uimm9_lsb0000> {
  bits<9> imm;
  let Inst{12-11} = imm{5-4};
  let Inst{10} = imm{8};
  let Inst{6-5} = imm{7-6};
}

def C_CSW : CCheriStore_rri<0b110, "c.csw", GPRC, uimm7_lsb00> {
  bits<7> imm;
  let Inst{12-10} = imm{5-3};
  let Inst{6} = imm{2};
  let Inst{5} = imm{6};
}

let DecoderNamespace = "RISCV32CapModeOnly_",
    Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV32, IsCapMode]  in
def C_CSC_64 : CCheriStore_rri<0b111, "c.csc", YGPRC, uimm8_lsb000> {
  bits<8> imm;
  let Inst{12-10} = imm{5-3};
  let Inst{6-5} = imm{7-6};
}

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode] in
def C_CSD : CCheriStore_rri<0b111, "c.csd", GPRC, uimm8_lsb000> {
  bits<8> imm;
  let Inst{12-10} = imm{5-3};
  let Inst{6-5} = imm{7-6};
}

let hasSideEffects = 0, mayLoad = 0, mayStore = 0, isCall = 1,
    DecoderNamespace = "RISCV32CapModeOnly_", Defs = [X1_Y],
    Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV32, IsCapMode]  in
def C_CJAL : RVInst16CJ<0b001, 0b01, (outs), (ins bare_simm12_lsb0:$offset),
                        "c.cjal", "$offset">;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
def C_CIncOffsetImm16CSP : RVInst16CI<0b011, 0b01, (outs CSP:$rd_wb),
                                      (ins CSP:$rs1, simm10_lsb0000nonzero:$imm),
                                      "c.cincoffset16csp", "$rs1, $imm"> {
  bits<0> rs1;
  let Constraints = "$rs1 = $rd_wb";
  let Inst{12} = imm{9};
  let Inst{11-7} = 2;
  let Inst{6} = imm{4};
  let Inst{5} = imm{6};
  let Inst{4-3} = imm{8-7};
  let Inst{2} = imm{5};
}

let DecoderNamespace = "RISCV32CapModeOnly_",
    Predicates = [HasCheri, HasCheriRVC, HasStdExtC, HasStdExtD, IsRV32, IsCapMode] in
def C_CFLDCSP : CCheriStackLoad<0b001, "c.cfldcsp", FPR64, uimm9_lsb000> {
  let Inst{6-5} = imm{4-3};
  let Inst{4-2} = imm{8-6};
}

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode] in
def C_CLCCSP_128 : CCheriStackLoad<0b001, "c.clccsp",  YGPRNoX0, uimm10_lsb0000> {
  let Inst{6} = imm{4};
  let Inst{5-2} = imm{9-6};
}

def C_CLWCSP : CCheriStackLoad<0b010, "c.clwcsp", GPRNoX0, uimm8_lsb00> {
  let Inst{6-4} = imm{4-2};
  let Inst{3-2} = imm{7-6};
}

let DecoderNamespace = "RISCV32CapModeOnly_",
    Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV32, IsCapMode] in
def C_CLCCSP_64 : CCheriStackLoad<0b011, "c.clccsp",  YGPRNoX0, uimm9_lsb000> {
  let Inst{6-5} = imm{4-3};
  let Inst{4-2} = imm{8-6};
}

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode] in
def C_CLDCSP : CCheriStackLoad<0b011, "c.cldcsp", GPRNoX0, uimm9_lsb000> {
  let Inst{6-5} = imm{4-3};
  let Inst{4-2} = imm{8-6};
}

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
def C_CJR : RVInst16CR<0b1000, 0b10, (outs), (ins  YGPRNoX0:$rs1),
                       "c.cjr", "$rs1"> {
  let isBranch = 1;
  let isBarrier = 1;
  let isTerminator = 1;
  let isIndirectBranch = 1;
  let rs2 = 0;
}

let hasSideEffects = 0, mayLoad = 0, mayStore = 0,
    isCall = 1, Defs = [X1_Y], rs2 = 0 in
def C_CJALR : RVInst16CR<0b1001, 0b10, (outs), (ins  YGPRNoX0:$rs1),
                         "c.cjalr", "$rs1">;

let DecoderNamespace = "RISCV32CapModeOnly_",
    Predicates = [HasCheri, HasCheriRVC, HasStdExtC, HasStdExtD, IsRV32, IsCapMode] in
def C_CFSDCSP : CCheriStackStore<0b101, "c.cfsdcsp", FPR64, uimm9_lsb000> {
  let Inst{12-10} = imm{5-3};
  let Inst{9-7}   = imm{8-6};
}

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode] in
def C_CSCCSP_128 : CCheriStackStore<0b101, "c.csccsp", YGPR, uimm10_lsb0000> {
  let Inst{12-11} = imm{5-4};
  let Inst{10-7}  = imm{9-6};
}

def C_CSWCSP : CCheriStackStore<0b110, "c.cswcsp", GPR, uimm8_lsb00> {
  let Inst{12-9} = imm{5-2};
  let Inst{8-7}  = imm{7-6};
}

let DecoderNamespace = "RISCV32CapModeOnly_",
    Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV32, IsCapMode] in
def C_CSCCSP_64 : CCheriStackStore<0b111, "c.csccsp", YGPR, uimm9_lsb000> {
  let Inst{12-10} = imm{5-3};
  let Inst{9-7}   = imm{8-6};
}

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode] in
def C_CSDCSP : CCheriStackStore<0b111, "c.csdcsp", GPR, uimm9_lsb000> {
  let Inst{12-10} = imm{5-3};
  let Inst{9-7}   = imm{8-6};
}

} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode]
} // DecoderNamespace = "CapModeOnly_"

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode] in {
def : InstAlias<"c.cincoffsetimm4cspn $rd, $rs1, $imm",
                (C_CIncOffsetImm4CSPN YGPRC:$rd, CSP:$rs1, uimm10_lsb00nonzero:$imm), 0>;
def : InstAlias<"c.cincoffsetimm16csp $rd, $imm",
                (C_CIncOffsetImm16CSP CSP:$rd, simm10_lsb0000nonzero:$imm), 0>;
def : MnemonicAlias<"c.lw", "c.clw">;
def : MnemonicAlias<"c.sw", "c.csw">;
def : MnemonicAlias<"c.sc", "c.csc">;
def : MnemonicAlias<"c.lc", "c.clc">;
def : MnemonicAlias<"c.lwsp", "c.clwcsp">;
def : MnemonicAlias<"c.swsp", "c.cswcsp">;
def : MnemonicAlias<"c.scsp", "c.csccsp">;
def : MnemonicAlias<"c.lcsp", "c.clccsp">;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode]
let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode] in {
def : MnemonicAlias<"c.ld", "c.cld">;
def : MnemonicAlias<"c.sd", "c.csd">;
def : MnemonicAlias<"c.ldsp", "c.cldcsp">;
def : MnemonicAlias<"c.sdsp", "c.csdcsp">;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode]
let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, HasStdExtD, IsRV32,
                  IsCapMode] in {
def : MnemonicAlias<"c.fld", "c.cfld">;
def : MnemonicAlias<"c.fsd", "c.cfsd">;
def : MnemonicAlias<"c.fldsp", "c.cfldcsp">;
def : MnemonicAlias<"c.fsdsp", "c.cfsdcsp">;
}

//===----------------------------------------------------------------------===//
// Pseudo-instructions and codegen patterns
//===----------------------------------------------------------------------===//

let Predicates = [HasCheri, IsCapMode] in {
// PCC-relative CLLC is two instructions, but GCP-relative needs 3, so the
// maximum expanded size of this Pseudo is 12 bytes.
let hasSideEffects = 0, mayLoad = 0, mayStore = 0, isCodeGenOnly = 0,
    isAsmParserOnly = 1, Size = 12 in {
def PseudoCLLC : Pseudo<(outs YGPR:$dst), (ins bare_symbol:$src), [],
                        "cllc", "$dst, $src">;
def PseudoCLLCInbounds : Pseudo<(outs YGPR:$dst), (ins bare_symbol:$src), [],
                        "cllc.inbounds", "$dst, $src">;
}

def : Pat<(riscv_cllc tglobaladdr:$in), (PseudoCLLC tglobaladdr:$in)>;
def : Pat<(riscv_cllc tblockaddress:$in), (PseudoCLLC tblockaddress:$in)>;
def : Pat<(riscv_cllc tjumptable:$in), (PseudoCLLC tjumptable:$in)>;
def : Pat<(riscv_cllc tconstpool:$in), (PseudoCLLC tconstpool:$in)>;
}

let hasSideEffects = 0, mayLoad = 1, mayStore = 0, isCodeGenOnly = 1,
    Size = 12 in {
  let Predicates = [HasCheri, IsCapMode] in {
    def PseudoCLLCInbounds_CLB
        : Pseudo<(outs GPR:$dst), (ins bare_symbol:$src), [],
                 "cllc.inbounds.clb", "$dst, $src">;

    def PseudoCLLCInbounds_CLBU
        : Pseudo<(outs GPR:$dst), (ins bare_symbol:$src), [],
                 "cllc.inbounds.clbu", "$dst, $src">;

    def PseudoCLLCInbounds_CLH
        : Pseudo<(outs GPR:$dst), (ins bare_symbol:$src), [],
                 "cllc.inbounds.clh", "$dst, $src">;

    def PseudoCLLCInbounds_CLHU
        : Pseudo<(outs GPR:$dst), (ins bare_symbol:$src), [],
                 "cllc.inbounds.clhu", "$dst, $src">;

    def PseudoCLLCInbounds_CLW
        : Pseudo<(outs GPR:$dst), (ins bare_symbol:$src), [],
                 "cllc.inbounds.clw", "$dst, $src">;
  }

  let Predicates = [HasCheri, IsCapMode, IsRV32] in {
    def PseudoCLLCInbounds_CLC_64
        : Pseudo<(outs YGPR:$dst), (ins bare_symbol:$src), [],
                 "cllc.inbounds.clc", "$dst, $src">;
  }

  let Predicates = [HasCheri, IsCapMode, IsRV64] in {
    def PseudoCLLCInbounds_CLWU
        : Pseudo<(outs GPR:$dst), (ins bare_symbol:$src), [],
                 "cllc.inbounds.clwu", "$dst, $src">;

    def PseudoCLLCInbounds_CLD
        : Pseudo<(outs GPR:$dst), (ins bare_symbol:$src), [],
                 "cllc.inbounds.cld", "$dst, $src">;

    def PseudoCLLCInbounds_CLC_128
        : Pseudo<(outs YGPR:$dst), (ins bare_symbol:$src), [],
                 "cllc.inbounds.clc", "$dst, $src">;
  }
}

let hasSideEffects = 0, mayLoad = 0, mayStore = 1, isCodeGenOnly = 1,
    Size = 12 in {
  let Predicates = [HasCheri, IsCapMode],
      Constraints = "@earlyclobber $tmp" in {
    def PseudoCLLCInbounds_CSB
        : Pseudo<(outs YGPR:$tmp), (ins GPR:$dst, bare_symbol:$src), [],
                 "cllc.inbounds.csb", "$tmp, $dst, $src">;

    def PseudoCLLCInbounds_CSH
        : Pseudo<(outs YGPR:$tmp), (ins GPR:$dst, bare_symbol:$src), [],
                 "cllc.inbounds.csh", "$tmp, $dst, $src">;

    def PseudoCLLCInbounds_CSW
        : Pseudo<(outs YGPR:$tmp), (ins GPR:$dst, bare_symbol:$src), [],
                 "cllc.inbounds.csw", "$tmp, $dst, $src">;
  }

  let Predicates = [HasCheri, IsCapMode, IsRV32] in {
    def PseudoCLLCInbounds_CSC_64
        : Pseudo<(outs YGPR:$tmp), (ins GPR:$dst, bare_symbol:$src), [],
                 "cllc.inbounds.csc", "$tmp, $dst, $src">;
  }

  let Predicates = [HasCheri, IsCapMode, IsRV64] in {
    def PseudoCLLCInbounds_CSD
        : Pseudo<(outs YGPR:$tmp), (ins YGPR:$dst, bare_symbol:$src), [],
                 "cllc.inbounds.csd", "$tmp, $dst, $src">;

    def PseudoCLLCInbounds_CSC_128
        : Pseudo<(outs YGPR:$tmp), (ins YGPR:$dst, bare_symbol:$src), [],
                 "cllc.inbounds.csc", "$tmp, $dst, $src">;
  }
}

let Predicates = [HasCheri, IsCapMode] in {
  let hasSideEffects = 0, mayLoad = 1, mayStore = 0, isCodeGenOnly = 0,
      isAsmParserOnly = 1,
      Size = 12 in def PseudoCLGC
      : Pseudo<(outs YGPR:$dst), (ins bare_symbol:$src), [], "clgc",
               "$dst, $src">;

  def : Pat<(riscv_clgc tglobaladdr:$in), (PseudoCLGC tglobaladdr:$in)>;

let hasSideEffects = 0, mayLoad = 1, mayStore = 0, isCodeGenOnly = 0,
    isAsmParserOnly = 1, Size = 8 in
def PseudoCLA_TLS_IE : Pseudo<(outs GPR:$dst, YGPR:$tmp),
                              (ins bare_symbol:$src), [],
                              "cla.tls.ie", "$dst, $src, $tmp">;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0, isCodeGenOnly = 0,
    isAsmParserOnly = 1, Size = 8 in
def PseudoCLC_TLS_GD : Pseudo<(outs YGPR:$dst), (ins bare_symbol:$src), [],
                              "clc.tls.gd", "$dst, $src">;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0, isCodeGenOnly = 0 in
def PseudoCIncOffsetTPRel : Pseudo<(outs YGPR:$rd),
                                   (ins YGPR:$rs1, GPR:$rs2,
                                        tprel_add_symbol:$src), [], "cincoffset",
                                   "$rd, $rs1, $rs2, $src">;
}

let Predicates = [HasCheri, HasStdExtD, IsRV32] in {

// Stores an FPR via splitting to two GPRs.
let hasSideEffects = 0, mayLoad = 0, mayStore = 1,
    Constraints = "@earlyclobber $tmplo,@earlyclobber $tmphi",
    usesCustomInserter = 1 in
def SplitStoreF64Pseudo
    : Pseudo<(outs GPR:$tmplo, GPR:$tmphi, GPR:$tmpdst),
             (ins FPR64:$src, GPR:$dst), []>;

// Stores an FPR via splitting to two GPRs.
let hasSideEffects = 0, mayLoad = 0, mayStore = 1,
    Constraints = "@earlyclobber $tmplo,@earlyclobber $tmphi",
    usesCustomInserter = 1 in
def CheriSplitStoreF64Pseudo
    : Pseudo<(outs GPR:$tmplo, GPR:$tmphi, YGPR:$tmpdst),
             (ins FPR64:$src, YGPR:$dst), []>;

} // Predicates = [HasCheri, HasStdExtD, IsRV32]

class PatYGPR<SDPatternOperator OpNode, RVInstCheriSrcDst Inst, ValueType RetVt=XLenVT>
    : Pat<(RetVt (OpNode YGPR:$rs1)), (Inst YGPR:$rs1)>;

class PatYGPRYGPR<SDPatternOperator OpNode, RVInst Inst, ValueType RetVt=YLenVT>
    : Pat<(RetVt (OpNode YGPR:$rs1, YGPR:$rs2)), (Inst YGPR:$rs1, YGPR:$rs2)>;

class PatYGPRYGPRSwapped<SDPatternOperator OpNode, RVInst Inst, ValueType RetVt=YLenVT>
    : Pat<(RetVt (OpNode YGPR:$rs1, YGPR:$rs2)), (Inst YGPR:$rs2, YGPR:$rs1)>;

class PatYGPRGpr<SDPatternOperator OpNode, RVInst Inst>
    : Pat<(OpNode YGPR:$rs1, (XLenVT GPR:$rs2)), (Inst YGPR:$rs1, GPR:$rs2)>;

class PatYGPRGprSwapped<SDPatternOperator OpNode, RVInst Inst>
    : Pat<(OpNode YGPR:$rs1, (XLenVT GPR:$rs2)), (Inst GPR:$rs2, YGPR:$rs1)>;

class PatYGPRSimm12<SDPatternOperator OpNode, RVInstI Inst>
    : Pat<(OpNode YGPR:$rs1, simm12_lo:$imm12), (Inst YGPR:$rs1, simm12_lo:$imm12)>;

class PatYGPRUimm12<SDPatternOperator OpNode, RVInstI Inst>
    : Pat<(OpNode YGPR:$rs1, uimm12:$imm12), (Inst YGPR:$rs1, uimm12:$imm12)>;

class PatGprYGPR<SDPatternOperator OpNode, RVInst Inst>
    : Pat<(OpNode GPR:$rs1, YGPR:$rs2), (Inst GPR:$rs1, YGPR:$rs2)>;

class PatGprYGPRSwapped<SDPatternOperator OpNode, RVInst Inst>
    : Pat<(OpNode GPR:$rs1, YGPR:$rs2), (Inst YGPR:$rs2, GPR:$rs1)>;

/// Capability-Inspection Instructions

def : PatYGPR<int_cheri_cap_perms_get, CGetPerm>;
def : PatYGPR<int_cheri_cap_type_get, CGetType>;
def : PatYGPR<int_cheri_cap_base_get, CGetBase>;
def : PatYGPR<int_cheri_cap_length_get, CGetLen>;
def : PatYGPR<riscv_cap_sealed_get, CGetSealed>;
let Predicates = [HasNoCheriot] in def : PatYGPR<int_cheri_cap_offset_get,
                                                 CGetOffset>;
def : PatYGPR<int_cheri_cap_flags_get, CGetFlags>;
def : Pat<(XLenVT (int_cheri_cap_address_get YGPR:$cs1)), (PseudoCGetAddr YGPR:$cs1)>;
def : PatYGPR<int_cheri_cap_high_get, CGetHigh>;
def : PatYGPR<int_cheri_cap_top_get, CGetTop>;

/// Capability-Modification Instructions

def : PatYGPRYGPR<int_cheri_cap_seal, CSeal>;
def : PatYGPRYGPR<int_cheri_cap_unseal, CUnseal>;
def : PatYGPRGpr<int_cheri_cap_perms_and, CAndPerm>;
def : PatYGPRGpr<int_cheri_cap_flags_set, CSetFlags>;
def : PatYGPRGpr<int_cheri_cap_offset_set, CSetOffset>;
def : PatYGPRGpr<int_cheri_cap_address_set, CSetAddr>;
def : PatYGPRGpr<int_cheri_cap_high_set, CSetHigh>;
def : PatYGPRGpr<cptradd, CIncOffset>;
def : PatYGPRSimm12<cptradd, CIncOffsetImm>;
def : PatYGPRGpr<int_cheri_cap_bounds_set, CSetBounds>;
def : PatYGPRGpr<int_cheri_cap_bounds_set_exact, CSetBoundsExact>;
def : PatYGPRGpr<int_cheri_cap_bounds_set_round_down, CSetBoundsRoundDown>;
def : PatYGPRGpr<riscv_cap_bounds_set, CSetBounds>;
def : PatYGPRUimm12<riscv_cap_bounds_set, CSetBoundsImm>;
def : PatYGPRUimm12<int_cheri_cap_bounds_set, CSetBoundsImm>;
def : PatYGPR<int_cheri_cap_tag_clear, CClearTag, YLenVT>;
def : PatYGPRYGPR<int_cheri_cap_build, CBuildCap>;
def : PatYGPRYGPR<int_cheri_cap_type_copy, CCopyType>;
def : PatYGPRYGPR<int_cheri_cap_conditional_seal, CCSeal>;
def : PatYGPR<int_cheri_cap_seal_entry, CSealEntry, YLenVT>;

def uimm12_lt512 : Operand<XLenVT>,
                   ImmLeaf<XLenVT, "return isUInt<12>(Imm) && (Imm < 512);">;

let Predicates =
    [HasCheriot] in def : Pat<(int_cheri_cap_bounds_set_exact YGPR:$rs1,
                                  uimm12_lt512:$imm12),
                              (CSetBoundsImm YGPR:$rs1, uimm12_lt512:$imm12)>;

// TODO: Make this rematerialisable like MIPS
def : PatYGPRGpr<int_cheri_bounded_stack_cap, CSetBounds>;
def : PatYGPRUimm12<int_cheri_bounded_stack_cap, CSetBoundsImm>;
def : PatYGPRGpr<int_cheri_bounded_stack_cap_dynamic, CSetBounds>;
def : PatYGPRUimm12<int_cheri_bounded_stack_cap_dynamic, CSetBoundsImm>;

def : Pat<(CapFrameAddrRegImm(cPTR YGPR:$rs1), simm12_lo:$imm12),
          (CIncOffsetImm YGPR:$rs1, simm12_lo:$imm12)>;

/// Pointer-Arithmetic Instructions

// int_cheri_cap_to_pointer and int_cheri_cap_from_pointer are expanded in C++
// code for ISAv9, so we don't need replacement patterns here.
def : Pat<(XLenVT (int_cheri_cap_diff YGPR:$cs1, YGPR:$cs2)),
          (SUB (XLenVT (EXTRACT_SUBREG YGPR:$cs1, sub_cap_addr)),
               (XLenVT (EXTRACT_SUBREG YGPR:$cs2, sub_cap_addr)))>;

let Predicates = [IsPureCapABI] in {
  def : Pat<(inttoptr(XLenVT GPR:$rs2)), (cPTR(CIncOffset(cPTR X0_Y), GPR:$rs2))>;
  def : Pat<(inttoptr simm12_lo:$imm12), (cPTR(CIncOffsetImm(cPTR X0_Y),
                                          simm12_lo:$imm12))>;
  def : Pat<(XLenVT(ptrtoint(cPTR YGPR:$rs1))), (PseudoCGetAddr YGPR:$rs1)>;
}

/// Null Capability Patterns

def : Pat<(inttoptr(XLenVT 0)), (YLenVT(COPY(cPTR X0_Y)))>;
def : Pat<(ptrtoint (YLenVT (inttoptr (XLenVT 0)))),
          (XLenVT (COPY (XLenVT X0)))>;

/// Pointer-Comparison Instructions

// Currently no dedicated instructions, so we:
// * Rely on a merged register file
// * Break untagged < tagged semantics
// * Can't implement exact equality
class CheriSetCCPatYGPRYGPR<PatFrag CondOp, dag GprGprDag>
    : Pat<(XLenVT(CondOp(cPTR YGPR:$cs1), (cPTR YGPR:$cs2))),
          (OutPatFrag<(ops node:$rs1, node:$rs2), GprGprDag>(XLenVT
               (EXTRACT_SUBREG YGPR:$cs1, sub_cap_addr)),
              (XLenVT(EXTRACT_SUBREG YGPR:$cs2, sub_cap_addr)))>;

multiclass CheriSetCCPatYGPRImm<PatFrag CondOp, Operand ImmTy, dag GprImmDag> {
  def : Pat<(XLenVT(CondOp(cPTR YGPR:$cs1), (inttoptr ImmTy:$imm12))),
            (OutPatFrag<(ops node:$rs1, node:$imm12), GprImmDag>(XLenVT
                 (EXTRACT_SUBREG YGPR:$cs1, sub_cap_addr)),
                ImmTy:$imm12)>;
  def : Pat<(XLenVT(CondOp(cPTR YGPR:$cs1), (cptradd(inttoptr(XLenVT 0)),
                                                ImmTy:$imm12))),
            (OutPatFrag<(ops node:$rs1, node:$imm12), GprImmDag>(XLenVT
                 (EXTRACT_SUBREG YGPR:$cs1, sub_cap_addr)),
                ImmTy:$imm12)>;
  def : Pat<(XLenVT(CondOp(cPTR YGPR:$cs1),
                (int_cheri_cap_offset_set(inttoptr(XLenVT 0)), ImmTy:$imm12))),
            (OutPatFrag<(ops node:$rs1, node:$imm12), GprImmDag>(XLenVT
                 (EXTRACT_SUBREG YGPR:$cs1, sub_cap_addr)),
                ImmTy:$imm12)>;
  def : Pat<(XLenVT(CondOp(cPTR YGPR:$cs1),
                (int_cheri_cap_address_set(inttoptr(XLenVT 0)), ImmTy:$imm12))),
            (OutPatFrag<(ops node:$rs1, node:$imm12), GprImmDag>(XLenVT
                 (EXTRACT_SUBREG YGPR:$cs1, sub_cap_addr)),
                ImmTy:$imm12)>;
}

multiclass CheriSetCCPatYGPRSimm12<PatFrag CondOp, dag GprImmDag>
    : CheriSetCCPatYGPRImm<CondOp, simm12_lo, GprImmDag>;

multiclass CheriSetCCPatYGPRSimm12Plus1<PatFrag CondOp, dag GprImmDag>
    : CheriSetCCPatYGPRImm<CondOp, simm12_plus1, GprImmDag>;

class CheriSetCCPatYGPRNull<PatFrag CondOp, dag GprDag>
    : Pat<(XLenVT(CondOp(cPTR YGPR:$cs1), (inttoptr(XLenVT 0)))),
          (OutPatFrag<(ops node:$rs1), GprDag>(XLenVT(EXTRACT_SUBREG YGPR:$cs1,
              sub_cap_addr)))>;

class Swap<PatFrag BinFrag>
    : PatFrag<(ops node:$a, node:$b), (BinFrag $b, $a)>;

// NB: Constants are wrapped as (cptradd (inttoptr 0) C) so the operand order
// isn't canonicalised like for integers so we have to add extra patterns for
// the commutative and inverse cases. Maybe we should support Constant:cN,
// represent them as (inttoptr C), or just canonicalise the current form?

def  : CheriSetCCPatYGPRYGPR<setlt, (SLT node:$rs1, node:$rs2)>;
defm : CheriSetCCPatYGPRSimm12<setlt, (SLTI node:$rs1, node:$imm12)>;
defm : CheriSetCCPatYGPRSimm12<Swap<setgt>, (SLTI node:$rs1, node:$imm12)>;
def  : CheriSetCCPatYGPRYGPR<setult, (SLTU node:$rs1, node:$rs2)>;
defm : CheriSetCCPatYGPRSimm12<setult, (SLTIU node:$rs1, node:$imm12)>;
defm : CheriSetCCPatYGPRSimm12<Swap<setugt>, (SLTIU node:$rs1, node:$imm12)>;

def  : CheriSetCCPatYGPRNull<seteq, (SLTIU node:$rs1, 1)>;
def  : CheriSetCCPatYGPRYGPR<seteq, (SLTIU (XOR node:$rs1, node:$rs2), 1)>;
defm : CheriSetCCPatYGPRSimm12Plus1<
           seteq, (SLTIU (ADDI node:$rs1, (NegImm node:$imm12)), 1)>;
defm : CheriSetCCPatYGPRSimm12Plus1<
           Swap<seteq>, (SLTIU (ADDI node:$rs1, (NegImm node:$imm12)), 1)>;
def  : CheriSetCCPatYGPRNull<setne, (SLTU (XLenVT X0), node:$rs1)>;
def  : CheriSetCCPatYGPRYGPR<setne, (SLTU (XLenVT X0), (XOR node:$rs1, node:$rs2))>;
defm : CheriSetCCPatYGPRSimm12Plus1<
           setne, (SLTU (XLenVT X0), (ADDI node:$rs1, (NegImm node:$imm12)))>;
defm : CheriSetCCPatYGPRSimm12Plus1<
           Swap<setne>, (SLTU (XLenVT X0), (ADDI node:$rs1, (NegImm node:$imm12)))>;
def  : CheriSetCCPatYGPRYGPR<setugt, (SLTU node:$rs2, node:$rs1)>;
def  : CheriSetCCPatYGPRYGPR<setuge, (XORI (SLTU node:$rs1, node:$rs2), 1)>;
def  : CheriSetCCPatYGPRYGPR<setule, (XORI (SLTU node:$rs2, node:$rs1), 1)>;
def  : CheriSetCCPatYGPRYGPR<setgt, (SLT node:$rs2, node:$rs1)>;
def  : CheriSetCCPatYGPRYGPR<setge, (XORI (SLT node:$rs1, node:$rs2), 1)>;
def  : CheriSetCCPatYGPRYGPR<setle, (XORI (SLT node:$rs2, node:$rs1), 1)>;

defm Select_YGPR : SelectCC_GPR_rrirr<YGPR, YLenVT>;
defm Select_YGPR_f64 : SelectCC_GPR_rrirr<YGPR, f64>;

/// Control-Flow Instructions

// No dedicated instructions; see above

class CheriBccPat<PatFrag CondOp, RVInstB Inst>
    : Pat<(brcond(XLenVT(CondOp(cPTR YGPR:$rs1), (cPTR YGPR:$rs2))), bb:$imm12),
          (Inst(XLenVT(EXTRACT_SUBREG YGPR:$rs1, sub_cap_addr)),
              (XLenVT(EXTRACT_SUBREG YGPR:$rs2, sub_cap_addr)),
              bare_simm13_lsb0_bb:$imm12)>;

def : CheriBccPat<seteq, BEQ>;
def : CheriBccPat<setne, BNE>;
def : CheriBccPat<setlt, BLT>;
def : CheriBccPat<setge, BGE>;
def : CheriBccPat<setult, BLTU>;
def : CheriBccPat<setuge, BGEU>;

class CheriBccSwapPat<PatFrag CondOp, RVInst InstBcc>
    : Pat<(brcond(XLenVT(CondOp(cPTR YGPR:$rs1), (cPTR YGPR:$rs2))), bb:$imm12),
          (InstBcc(XLenVT(EXTRACT_SUBREG YGPR:$rs2, sub_cap_addr)),
              (XLenVT(EXTRACT_SUBREG YGPR:$rs1, sub_cap_addr)),
              bare_simm13_lsb0_bb:$imm12)>;

def : CheriBccSwapPat<setgt, BLT>;
def : CheriBccSwapPat<setle, BGE>;
def : CheriBccSwapPat<setugt, BLTU>;
def : CheriBccSwapPat<setule, BGEU>;

/// Assertion Instructions

def : PatYGPRYGPR<riscv_cap_subset_test, CTestSubset, XLenVT>;
def : PatYGPRYGPR<riscv_cap_equal_exact, CSEQX, XLenVT>;

/// Special Capability Register Access Instructions

def : Pat<(int_cheri_ddc_get), (CSpecialRW SCR_DDC.Encoding, (cPTR X0_Y))>;
let Predicates = [HasCheri,
                  IsPureCapABI] in def : Pat<(int_cheri_stack_cap_get),
                                             (YLenVT(COPY(cPTR X2_Y)))>;

let Predicates = [HasCheri, IsCapMode] in
def : Pat<(int_cheri_pcc_get), (AUIPCC 0)>;

let Predicates = [HasCheri,
                  NotCapMode] in def : Pat<(int_cheri_pcc_get),
                                           (CSpecialRW SCR_PCC.Encoding,
                                               (cPTR X0_Y))>;

/// Fast Register-Clearing Instructions

/// Adjusting to Compressed Capability Precision Instructions

def : PatGpr<int_cheri_round_representable_length, CRRL>;
def : PatGpr<int_cheri_representable_alignment_mask, CRAM>;

/// Tag-Memory Access Instructions

def : PatYGPR<int_cheri_cap_load_tags, CLoadTags, XLenVT>;

/// Memory-Access with Explicit Address Type Instructions

multiclass CheriExplicitLdPat<PatFrag LoadOp, RVInst Inst,
                              RegisterClass AddrTy,
                              ValueType AddrVt = XLenVT,
                              ValueType ReturnVt = XLenVT> {
  def : Pat<(ReturnVt (LoadOp (AddrVt AddrTy:$rs1))), (Inst AddrTy:$rs1)>;
}

multiclass CheriExplicitStPat<PatFrag StoreOp, RVInst Inst,
                              RegisterClass StTy, RegisterClass AddrTy,
                              ValueType StoreVt = XLenVT,
                              ValueType AddrVt = XLenVT> {
  def : Pat<(StoreOp (StoreVt StTy:$rs2), (AddrVt AddrTy:$rs1)), (Inst StTy:$rs2, AddrTy:$rs1)>;
}

/// DDC-relative loads

let Predicates = [HasCheri, IsCapMode] in {
defm : CheriExplicitLdPat<sextloadi8, LB_DDC, GPR>;
defm : CheriExplicitLdPat<extloadi8, LB_DDC, GPR>;
defm : CheriExplicitLdPat<sextloadi16, LH_DDC, GPR>;
defm : CheriExplicitLdPat<extloadi16, LH_DDC, GPR>;
defm : CheriExplicitLdPat<zextloadi8, LBU_DDC, GPR>;
defm : CheriExplicitLdPat<zextloadi16, LHU_DDC, GPR>;
}

let Predicates = [HasCheri, IsCapMode, IsRV32] in {
defm : CheriExplicitLdPat<load, LW_DDC, GPR>;
defm : CheriExplicitLdPat<load, LC_DDC_64, GPR, XLenVT, YLenVT>;
}

let Predicates = [HasCheri, IsCapMode, IsRV64] in {
defm : CheriExplicitLdPat<sextloadi32, LW_DDC, GPR>;
defm : CheriExplicitLdPat<extloadi32, LW_DDC, GPR>;
defm : CheriExplicitLdPat<load, LD_DDC, GPR>;
defm : CheriExplicitLdPat<zextloadi32, LWU_DDC, GPR>;
defm : CheriExplicitLdPat<load, LC_DDC_128, GPR, XLenVT, YLenVT>;
}

let Predicates = [HasCheri, IsCapMode, HasStdExtF] in
def : Pat<(load (XLenVT GPR:$rs1)), (FMV_W_X (LW_DDC (XLenVT GPR:$rs1)))>;

let Predicates = [HasCheri, IsCapMode, HasStdExtD, IsRV32] in
def : Pat<(load (XLenVT GPR:$rs1)),
          (BuildPairF64Pseudo (LW_DDC (XLenVT GPR:$rs1)),
                              (LW_DDC (ADDI (XLenVT GPR:$rs1), 4)))>;

let Predicates = [HasCheri, IsCapMode, HasStdExtD, IsRV64] in
def : Pat<(load (XLenVT GPR:$rs1)), (FMV_D_X (LD_DDC (XLenVT GPR:$rs1)))>;

/// DDC-relative stores

let Predicates = [HasCheri, IsCapMode] in {
defm : CheriExplicitStPat<truncstorei8, SB_DDC, GPR, GPR>;
defm : CheriExplicitStPat<truncstorei16, SH_DDC, GPR, GPR>;
}

let Predicates = [HasCheri, IsCapMode, IsRV32] in {
defm : CheriExplicitStPat<store, SW_DDC, GPR, GPR>;
defm : CheriExplicitStPat<store, SC_DDC_64, YGPR, GPR, YLenVT, XLenVT>;
}

let Predicates = [HasCheri, IsCapMode, IsRV64] in {
defm : CheriExplicitStPat<truncstorei32, SW_DDC, GPR, GPR>;
defm : CheriExplicitStPat<store, SD_DDC, GPR, GPR>;
defm : CheriExplicitStPat<store, SC_DDC_128, YGPR, GPR, YLenVT, XLenVT>;
}

let Predicates = [HasCheri, IsCapMode, HasStdExtF] in
def : Pat<(store FPR32:$rs2, (XLenVT GPR:$rs1)),
          (SW_DDC (FMV_X_W FPR32:$rs2), (XLenVT GPR:$rs1))>;

let Predicates = [HasCheri, IsCapMode, HasStdExtD, IsRV32] in
def : Pat<(store FPR64:$rs2, (XLenVT GPR:$rs1)),
          (KILL (SplitStoreF64Pseudo FPR64:$rs2, (XLenVT GPR:$rs1)))>;

let Predicates = [HasCheri, IsCapMode, HasStdExtD, IsRV64] in
def : Pat<(store FPR64:$rs2, (XLenVT GPR:$rs1)),
          (SD_DDC (FMV_X_D FPR64:$rs2), (XLenVT GPR:$rs1))>;

/// Capability loads

let Predicates = [HasCheri, NotCapMode] in {
defm : CheriExplicitLdPat<sextloadi8, LB_CAP, YGPR, YLenVT, XLenVT>;
defm : CheriExplicitLdPat<extloadi8, LB_CAP, YGPR, YLenVT, XLenVT>;
defm : CheriExplicitLdPat<sextloadi16, LH_CAP, YGPR, YLenVT, XLenVT>;
defm : CheriExplicitLdPat<extloadi16, LH_CAP, YGPR, YLenVT, XLenVT>;
defm : CheriExplicitLdPat<zextloadi8, LBU_CAP, YGPR, YLenVT, XLenVT>;
defm : CheriExplicitLdPat<zextloadi16, LHU_CAP, YGPR, YLenVT, XLenVT>;
}

let Predicates = [HasCheri, NotCapMode, IsRV32] in {
defm : CheriExplicitLdPat<load, LW_CAP, YGPR, YLenVT, XLenVT>;
defm : CheriExplicitLdPat<load, LC_CAP_64, YGPR, YLenVT, YLenVT>;
}

let Predicates = [HasCheri, NotCapMode, IsRV64] in {
defm : CheriExplicitLdPat<sextloadi32, LW_CAP, YGPR, YLenVT, XLenVT>;
defm : CheriExplicitLdPat<extloadi32, LW_CAP, YGPR, YLenVT, XLenVT>;
defm : CheriExplicitLdPat<load, LD_CAP, YGPR, YLenVT, XLenVT>;
defm : CheriExplicitLdPat<zextloadi32, LWU_CAP, YGPR, YLenVT, XLenVT>;
defm : CheriExplicitLdPat<load, LC_CAP_128, YGPR, YLenVT, YLenVT>;
}

let Predicates = [HasCheri, NotCapMode, HasStdExtF] in
def : Pat<(load YGPR:$rs1), (FMV_W_X (LW_CAP YGPR:$rs1))>;

let Predicates = [HasCheri, NotCapMode, HasStdExtD, IsRV32] in
def : Pat<(load YGPR:$rs1),
          (BuildPairF64Pseudo (LW_CAP YGPR:$rs1),
                              (LW_CAP (CIncOffsetImm YGPR:$rs1, 4)))>;

let Predicates = [HasCheri, NotCapMode, HasStdExtD, IsRV64] in
def : Pat<(load YGPR:$rs1), (FMV_D_X (LD_CAP YGPR:$rs1))>;

/// Capability stores

let Predicates = [HasCheri, NotCapMode] in {
defm : CheriExplicitStPat<truncstorei8, SB_CAP, GPR, YGPR, XLenVT, YLenVT>;
defm : CheriExplicitStPat<truncstorei16, SH_CAP, GPR, YGPR, XLenVT, YLenVT>;
}

let Predicates = [HasCheri, NotCapMode, IsRV32] in {
defm : CheriExplicitStPat<store, SW_CAP, GPR, YGPR, XLenVT, YLenVT>;
defm : CheriExplicitStPat<store, SC_CAP_64, YGPR, YGPR, YLenVT, YLenVT>;
}

let Predicates = [HasCheri, NotCapMode, IsRV64] in {
defm : CheriExplicitStPat<truncstorei32, SW_CAP, GPR, YGPR, XLenVT, YLenVT>;
defm : CheriExplicitStPat<store, SD_CAP, GPR, YGPR, XLenVT, YLenVT>;
defm : CheriExplicitStPat<store, SC_CAP_128, YGPR, YGPR, YLenVT, YLenVT>;
}

let Predicates = [HasCheri, NotCapMode, HasStdExtF] in
def : Pat<(store FPR32:$rs2, YGPR:$rs1),
          (SW_CAP (FMV_X_W FPR32:$rs2), YGPR:$rs1)>;

let Predicates = [HasCheri, NotCapMode, HasStdExtD, IsRV32] in
def : Pat<(store FPR64:$rs2, YGPR:$rs1),
          (KILL (CheriSplitStoreF64Pseudo FPR64:$rs2, YGPR:$rs1))>;

let Predicates = [HasCheri, NotCapMode, HasStdExtD, IsRV64] in
def : Pat<(store FPR64:$rs2, YGPR:$rs1),
          (SD_CAP (FMV_X_D FPR64:$rs2), YGPR:$rs1)>;

/// Non-Capability Mode Instructions

/// Memory-Access Instructions

multiclass AMOCapPat<string YLenStr, string AtomicOp, string BaseInst> {
  def : PatGprYGPRSwapped<!cast<PatFrag>(AtomicOp#"_monotonic"),
                   !cast<RVInst>(BaseInst#"_"#YLenStr)>;
  def : PatGprYGPRSwapped<!cast<PatFrag>(AtomicOp#"_acquire"),
                   !cast<RVInst>(BaseInst#"_AQ_"#YLenStr)>;
  def : PatGprYGPRSwapped<!cast<PatFrag>(AtomicOp#"_release"),
                   !cast<RVInst>(BaseInst#"_RL_"#YLenStr)>;
  def : PatGprYGPRSwapped<!cast<PatFrag>(AtomicOp#"_acq_rel"),
                   !cast<RVInst>(BaseInst#"_AQ_RL_"#YLenStr)>;
  def : PatGprYGPRSwapped<!cast<PatFrag>(AtomicOp#"_seq_cst"),
                   !cast<RVInst>(BaseInst#"_AQ_RL_"#YLenStr)>;
}

let Predicates = [HasCheri, IsRV32, NotCapMode] in {
def : LdPat<load, LC_64, c64>;
def : StPat<store, SC_64, YGPR, c64>;
} // Predicates = [HasCheri, IsRV32, NotCapMode]

let Predicates = [HasCheri, IsRV64, NotCapMode] in {
def : LdPat<load, LC_128, c128>;
def : StPat<store, SC_128, YGPR, c128>;
} // Predicates = [HasCheri, IsRV64, NotCapMode]

let Predicates = [HasCheri, HasStdExtA, IsRV32, NotCapMode] in {
def : LdPat<atomic_load_cap, LC_64, c64>;
def : StPat<atomic_store_cap, SC_64, YGPR, c64>;
defm : AMOCapPat<"64", "atomic_swap_cap", "AMOSWAP_C">;
} // Predicates = [HasCheri, HasStdExtA, IsRV32, NotCapMode]

let Predicates = [HasCheri, HasStdExtA, IsRV64, NotCapMode] in {
def : LdPat<atomic_load_cap, LC_128, c128>;
def : StPat<atomic_store_cap, SC_128, YGPR, c128>;
defm : AMOCapPat<"128", "atomic_swap_cap", "AMOSWAP_C">;
} // Predicates = [HasCheri, HasStdExtA, IsRV64, NotCapMode]

let Predicates = [HasCheri, HasStdExtA] in {
def PseudoAtomicLoadAddCap : PseudoAMO<YGPR> { let Size = 16; }
def PseudoAtomicLoadSubCap : PseudoAMO<YGPR> { let Size = 20; }
def PseudoAtomicLoadAndCap : PseudoAMO<YGPR> { let Size = 20; }
def PseudoAtomicLoadOrCap : PseudoAMO<YGPR> { let Size = 20; }
def PseudoAtomicLoadXorCap : PseudoAMO<YGPR> { let Size = 20; }
def PseudoAtomicLoadMaxCap : PseudoAMO<YGPR> { let Size = 24; }
def PseudoAtomicLoadMinCap : PseudoAMO<YGPR> { let Size = 24; }
def PseudoAtomicLoadUMaxCap : PseudoAMO<YGPR> { let Size = 24; }
def PseudoAtomicLoadUMinCap : PseudoAMO<YGPR> { let Size = 24; }
def PseudoAtomicLoadNandCap : PseudoAMO<YGPR> { let Size = 24; }
def PseudoCmpXchgCap : PseudoCmpXchg<YGPR> { let Size = 16; }
} // Predicates = [HasCheri, HasStdExtA]f

let Predicates = [HasCheri, HasStdExtA, NotCapMode] in {
defm : PseudoAMOPat<"atomic_load_add_cap", PseudoAtomicLoadAddCap, YLenVT, YGPR>;
defm : PseudoAMOPat<"atomic_load_sub_cap", PseudoAtomicLoadSubCap, YLenVT, YGPR>;
defm : PseudoAMOPat<"atomic_load_and_cap", PseudoAtomicLoadAndCap, YLenVT, YGPR>;
defm : PseudoAMOPat<"atomic_load_or_cap", PseudoAtomicLoadOrCap, YLenVT, YGPR>;
defm : PseudoAMOPat<"atomic_load_xor_cap", PseudoAtomicLoadXorCap, YLenVT, YGPR>;
defm : PseudoAMOPat<"atomic_load_max_cap", PseudoAtomicLoadMaxCap, YLenVT, YGPR>;
defm : PseudoAMOPat<"atomic_load_min_cap", PseudoAtomicLoadMinCap, YLenVT, YGPR>;
defm : PseudoAMOPat<"atomic_load_umax_cap", PseudoAtomicLoadUMaxCap, YLenVT, YGPR>;
defm : PseudoAMOPat<"atomic_load_umin_cap", PseudoAtomicLoadUMinCap, YLenVT, YGPR>;
defm : PseudoAMOPat<"atomic_load_nand_cap", PseudoAtomicLoadNandCap, YLenVT, YGPR>;
defm : PseudoCmpXchgPat<"atomic_cmp_swap_cap", PseudoCmpXchgCap, YLenVT, YGPR>;
} // Predicates = [HasCheri, HasStdExtA, NotCapMode]

/// Capability Mode Instructions

multiclass CheriLdPat<PatFrag LoadOp, RVInst Inst, ValueType ReturnVt = XLenVT> {
  def : Pat<(ReturnVt(LoadOp(CapRegImm(cPTR YGPR:$rs1), simm12_lo:$imm12))),
            (Inst YGPR:$rs1, simm12_lo:$imm12)>;
}

multiclass CheriStPat<PatFrag StoreOp, RVInst Inst, RegisterClass StTy, ValueType StoreVt = XLenVT> {
  def : Pat<(StoreOp(StoreVt StTy:$rs2), (CapRegImm(cPTR YGPR:$rs1),
                                             simm12_lo:$imm12)),
            (Inst(StoreVt StTy:$rs2), YGPR:$rs1, simm12_lo:$imm12)>;
}

multiclass CheriAtomicStPat<PatFrag StoreOp, RVInst Inst, RegisterClass StTy, ValueType StoreVt>
    : CheriStPat<StoreOp, Inst, StTy, StoreVt>;

multiclass CheriAMOPat<string AtomicOp, string BaseInst> {
  def : PatYGPRGprSwapped<!cast<PatFrag>(AtomicOp#"_monotonic"),
                   !cast<RVInst>(BaseInst)>;
  def : PatYGPRGprSwapped<!cast<PatFrag>(AtomicOp#"_acquire"),
                   !cast<RVInst>(BaseInst#"_AQ")>;
  def : PatYGPRGprSwapped<!cast<PatFrag>(AtomicOp#"_release"),
                   !cast<RVInst>(BaseInst#"_RL")>;
  def : PatYGPRGprSwapped<!cast<PatFrag>(AtomicOp#"_acq_rel"),
                   !cast<RVInst>(BaseInst#"_AQ_RL")>;
  def : PatYGPRGprSwapped<!cast<PatFrag>(AtomicOp#"_seq_cst"),
                   !cast<RVInst>(BaseInst#"_AQ_RL")>;
}

multiclass CheriAMOSubPat<string AtomicOp, string BaseInst> {
  def : Pat<(!cast<PatFrag>(AtomicOp#"_monotonic") YGPR:$addr, (XLenVT GPR:$incr)),
            (!cast<RVInst>(BaseInst) (SUB (XLenVT X0), (XLenVT GPR:$incr)), YGPR:$addr)>;
  def : Pat<(!cast<PatFrag>(AtomicOp#"_acquire") YGPR:$addr, (XLenVT GPR:$incr)),
            (!cast<RVInst>(BaseInst#"_AQ") (SUB (XLenVT X0), (XLenVT GPR:$incr)), YGPR:$addr)>;
  def : Pat<(!cast<PatFrag>(AtomicOp#"_release") YGPR:$addr, (XLenVT GPR:$incr)),
            (!cast<RVInst>(BaseInst#"_RL") (SUB (XLenVT X0), (XLenVT GPR:$incr)), YGPR:$addr)>;
  def : Pat<(!cast<PatFrag>(AtomicOp#"_acq_rel") YGPR:$addr, (XLenVT GPR:$incr)),
            (!cast<RVInst>(BaseInst#"_AQ_RL") (SUB (XLenVT X0), (XLenVT GPR:$incr)), YGPR:$addr)>;
  def : Pat<(!cast<PatFrag>(AtomicOp#"_seq_cst") YGPR:$addr, (XLenVT GPR:$incr)),
            (!cast<RVInst>(BaseInst#"_AQ_RL") (SUB (XLenVT X0), (XLenVT GPR:$incr)), YGPR:$addr)>;
}

class PseudoCheriAMO<RegisterClass ValTy=GPR>
    : Pseudo<(outs ValTy:$res, ValTy:$scratch),
             (ins YGPR:$addr, ValTy:$incr, ordering:$ordering), []> {
  let Constraints = "@earlyclobber $res,@earlyclobber $scratch";
  let mayLoad = 1;
  let mayStore = 1;
  let hasSideEffects = 0;
}

multiclass PseudoCheriAMOPat<string AtomicOp, Instruction BaseInst,
                             RegisterClass ValTy=GPR> {
  defvar vt = ValTy.RegTypes[0];
  def : Pat<(!cast<PatFrag>(AtomicOp#"_monotonic") YGPR:$addr, (vt ValTy:$incr)),
            (BaseInst YGPR:$addr, ValTy:$incr, 2)>;
  def : Pat<(!cast<PatFrag>(AtomicOp#"_acquire") YGPR:$addr, (vt ValTy:$incr)),
            (BaseInst YGPR:$addr, ValTy:$incr, 4)>;
  def : Pat<(!cast<PatFrag>(AtomicOp#"_release") YGPR:$addr, (vt ValTy:$incr)),
            (BaseInst YGPR:$addr, ValTy:$incr, 5)>;
  def : Pat<(!cast<PatFrag>(AtomicOp#"_acq_rel") YGPR:$addr, (vt ValTy:$incr)),
            (BaseInst YGPR:$addr, ValTy:$incr, 6)>;
  def : Pat<(!cast<PatFrag>(AtomicOp#"_seq_cst") YGPR:$addr, (vt ValTy:$incr)),
            (BaseInst YGPR:$addr, ValTy:$incr, 7)>;
}

class PseudoCheriCmpXchg<RegisterClass ValTy=GPR>
    : Pseudo<(outs ValTy:$res, GPR:$scratch),
             (ins YGPR:$addr, ValTy:$cmpval, ValTy:$newval, ordering:$ordering), []> {
  let Constraints = "@earlyclobber $res,@earlyclobber $scratch";
  let mayLoad = 1;
  let mayStore = 1;
  let hasSideEffects = 0;
}

multiclass PseudoCheriCmpXchgPat<string Op, Pseudo CmpXchgInst,
                                 RegisterClass ValTy=GPR> {
  defvar vt = ValTy.RegTypes[0];
  def : Pat<(!cast<PatFrag>(Op#"_monotonic") YGPR:$addr, (vt ValTy:$cmp), (vt ValTy:$new)),
            (CmpXchgInst YGPR:$addr, ValTy:$cmp, ValTy:$new, 2)>;
  def : Pat<(!cast<PatFrag>(Op#"_acquire") YGPR:$addr, (vt ValTy:$cmp), (vt ValTy:$new)),
            (CmpXchgInst YGPR:$addr, ValTy:$cmp, ValTy:$new, 4)>;
  def : Pat<(!cast<PatFrag>(Op#"_release") YGPR:$addr, (vt ValTy:$cmp), (vt ValTy:$new)),
            (CmpXchgInst YGPR:$addr, ValTy:$cmp, ValTy:$new, 5)>;
  def : Pat<(!cast<PatFrag>(Op#"_acq_rel") YGPR:$addr, (vt ValTy:$cmp), (vt ValTy:$new)),
            (CmpXchgInst YGPR:$addr, ValTy:$cmp, ValTy:$new, 6)>;
  def : Pat<(!cast<PatFrag>(Op#"_seq_cst") YGPR:$addr, (vt ValTy:$cmp), (vt ValTy:$new)),
            (CmpXchgInst YGPR:$addr, ValTy:$cmp, ValTy:$new, 7)>;
} 

multiclass CheriAMOCapPat<string YLenStr, string AtomicOp, string BaseInst> {
  def : PatYGPRYGPRSwapped<!cast<PatFrag>(AtomicOp#"_monotonic"),
                    !cast<RVInst>(BaseInst#"_"#YLenStr), YLenVT>;
  def : PatYGPRYGPRSwapped<!cast<PatFrag>(AtomicOp#"_acquire"),
                    !cast<RVInst>(BaseInst#"_AQ_"#YLenStr), YLenVT>;
  def : PatYGPRYGPRSwapped<!cast<PatFrag>(AtomicOp#"_release"),
                    !cast<RVInst>(BaseInst#"_RL_"#YLenStr), YLenVT>;
  def : PatYGPRYGPRSwapped<!cast<PatFrag>(AtomicOp#"_acq_rel"),
                    !cast<RVInst>(BaseInst#"_AQ_RL_"#YLenStr), YLenVT>;
  def : PatYGPRYGPRSwapped<!cast<PatFrag>(AtomicOp#"_seq_cst"),
                    !cast<RVInst>(BaseInst#"_AQ_RL_"#YLenStr), YLenVT>;
}

let Predicates = [HasCheri, HasStdExtA] in {
def PseudoCheriAtomicSwap8 : PseudoCheriAMO { let Size = 16; }
def PseudoCheriAtomicLoadAdd8 : PseudoCheriAMO { let Size = 16; }
def PseudoCheriAtomicLoadSub8 : PseudoCheriAMO { let Size = 16; }
def PseudoCheriAtomicLoadAnd8 : PseudoCheriAMO { let Size = 16; }
def PseudoCheriAtomicLoadOr8 : PseudoCheriAMO { let Size = 16; }
def PseudoCheriAtomicLoadXor8 : PseudoCheriAMO { let Size = 16; }
def PseudoCheriAtomicLoadMax8 : PseudoCheriAMO { let Size = 24; }
def PseudoCheriAtomicLoadMin8 : PseudoCheriAMO { let Size = 24; }
def PseudoCheriAtomicLoadUMax8 : PseudoCheriAMO { let Size = 24; }
def PseudoCheriAtomicLoadUMin8 : PseudoCheriAMO { let Size = 24; }
def PseudoCheriAtomicLoadNand8 : PseudoCheriAMO { let Size = 20; }
def PseudoCheriCmpXchg8 : PseudoCheriCmpXchg { let Size = 16; }

def PseudoCheriAtomicSwap16 : PseudoCheriAMO { let Size = 16; }
def PseudoCheriAtomicLoadAdd16 : PseudoCheriAMO { let Size = 16; }
def PseudoCheriAtomicLoadSub16 : PseudoCheriAMO { let Size = 16; }
def PseudoCheriAtomicLoadAnd16 : PseudoCheriAMO { let Size = 16; }
def PseudoCheriAtomicLoadOr16 : PseudoCheriAMO { let Size = 16; }
def PseudoCheriAtomicLoadXor16 : PseudoCheriAMO { let Size = 16; }
def PseudoCheriAtomicLoadMax16 : PseudoCheriAMO { let Size = 24; }
def PseudoCheriAtomicLoadMin16 : PseudoCheriAMO { let Size = 24; }
def PseudoCheriAtomicLoadUMax16 : PseudoCheriAMO { let Size = 24; }
def PseudoCheriAtomicLoadUMin16 : PseudoCheriAMO { let Size = 24; }
def PseudoCheriAtomicLoadNand16 : PseudoCheriAMO { let Size = 20; }
def PseudoCheriCmpXchg16 : PseudoCheriCmpXchg { let Size = 16; }

def PseudoCheriAtomicLoadNand32 : PseudoCheriAMO { let Size = 20; }
def PseudoCheriCmpXchg32 : PseudoCheriCmpXchg { let Size = 16; }

def PseudoCheriAtomicLoadAddCap : PseudoCheriAMO<YGPR> { let Size = 16; }
def PseudoCheriAtomicLoadSubCap : PseudoCheriAMO<YGPR> { let Size = 20; }
def PseudoCheriAtomicLoadAndCap : PseudoCheriAMO<YGPR> { let Size = 20; }
def PseudoCheriAtomicLoadOrCap : PseudoCheriAMO<YGPR> { let Size = 20; }
def PseudoCheriAtomicLoadXorCap : PseudoCheriAMO<YGPR> { let Size = 20; }
def PseudoCheriAtomicLoadMaxCap : PseudoCheriAMO<YGPR> { let Size = 24; }
def PseudoCheriAtomicLoadMinCap : PseudoCheriAMO<YGPR> { let Size = 24; }
def PseudoCheriAtomicLoadUMaxCap : PseudoCheriAMO<YGPR> { let Size = 24; }
def PseudoCheriAtomicLoadUMinCap : PseudoCheriAMO<YGPR> { let Size = 24; }
def PseudoCheriAtomicLoadNandCap : PseudoCheriAMO<YGPR> { let Size = 24; }
def PseudoCheriCmpXchgCap : PseudoCheriCmpXchg<YGPR> { let Size = 16; }
} // Predicates = [HasCheri, HasStdExtA]

let Predicates = [HasCheri, HasStdExtA, IsRV64] in {
def PseudoCheriAtomicLoadNand64 : PseudoCheriAMO { let Size = 20; }
def PseudoCheriCmpXchg64 : PseudoCheriCmpXchg { let Size = 16; }
} // Predicates = [HasCheri, HasStdExtA, IsRV64]

/// 'I' (Integer) base

let Predicates = [HasCheri, IsCapMode, IsPureCapABI] in {
let isBarrier = 1, isBranch = 1, isTerminator = 1 in
def PseudoCBR : Pseudo<(outs), (ins simm21_lsb0_jal:$imm20), [(br bb:$imm20)]>,
                PseudoInstExpansion<(CJAL X0_Y, simm21_lsb0_jal:$imm20)>;

let isCall = 1, Defs = [X1_Y] in
let isBarrier = 1, isBranch = 1, isIndirectBranch = 1, isTerminator = 1 in
def PseudoCBRIND : Pseudo<(outs), (ins  YGPRNoX0X1:$rs1, simm12_lo:$imm12), []>,
                   PseudoInstExpansion<(CJALR X0_Y, YGPR:$rs1, simm12_lo:$imm12)>;

def : Pat<(brind YGPR:$rs1), (PseudoCBRIND YGPR:$rs1, 0)>;
def : Pat<(brind (cptradd YGPR:$rs1, simm12_lo:$imm12)),
          (PseudoCBRIND YGPR:$rs1, simm12_lo:$imm12)>;
} // Predicates = [HasCheri, IsCapMode, IsPureCapABI]

let Predicates = [HasCheri, IsCapMode],
    isCall = 1, isBarrier = 1, isCodeGenOnly = 0, hasSideEffects = 0,
    mayStore = 0, mayLoad = 0, Size = 8 in
def PseudoCCALLReg : Pseudo<(outs YGPR:$rd),
                            (ins cap_call_symbol:$func), []> {
  let AsmString = "ccall\t$rd, $func";
}

let Predicates = [HasCheri, IsCapMode],
    isCall = 1, Defs = [X1_Y], isCodeGenOnly = 0, Size = 8 in
def PseudoCCALL : Pseudo<(outs), (ins cap_call_symbol:$func), []> {
  let AsmString = "ccall\t$func";
}

let Predicates = [HasCheriot] in {
  def : InstAlias<"ct.ccall $rd, $func", (PseudoCCALLReg YGPR:$rd,
                                             cap_call_symbol:$func)>;
  def : InstAlias<"ct.ccall $func", (PseudoCCALL cap_call_symbol:$func)>;
}

// The PseudoCompartmentCall of expands to AUIPCC, CLC_64, C_CJALR (10 bytes),
// but the setup to obtain the callee capability from an import table can
// require up to 8 additional bytes.
let Predicates = [HasCheri, IsCapMode],
    isCall = 1, Defs = [X1_Y], isCodeGenOnly = 1, Size = 18 in
def PseudoCompartmentCall : Pseudo<(outs), (ins cap_call_symbol:$func), []>;

let Predicates = [HasCheri, IsCapMode],
    isCall = 1, Defs = [X1_Y], isCodeGenOnly = 1, Size = 10 in
def PseudoLibraryCall : Pseudo<(outs), (ins cap_call_symbol:$func), []>;

let Predicates = [HasCheri, IsCapMode, IsPureCapABI] in {
def : Pat<(riscv_cap_call tglobaladdr:$func),
          (PseudoCCALL tglobaladdr:$func)>;
def : Pat<(riscv_cap_call texternalsym:$func),
          (PseudoCCALL texternalsym:$func)>;
def : Pat<(riscv_cap_compartment_call tglobaladdr:$func),
          (PseudoCompartmentCall tglobaladdr:$func)>;
def : Pat<(riscv_cap_compartment_call texternalsym:$func),
          (PseudoCompartmentCall texternalsym:$func)>;
def : Pat<(riscv_cap_compartment_call YGPR:$func),
          (PseudoCompartmentCall YGPR:$func)>;
def : Pat<(riscv_cap_library_call tglobaladdr:$func),
          (PseudoLibraryCall tglobaladdr:$func)>;
def : Pat<(riscv_cap_library_call texternalsym:$func),
          (PseudoLibraryCall texternalsym:$func)>;
def : Pat<(riscv_cap_library_call YGPR:$func),
          (PseudoLibraryCall YGPR:$func)>;
} // Predicates = [HasCheri, IsCapMode, IsPureCapABI]

let Predicates = [HasCheri, IsCapMode, IsPureCapABI],
    isCall = 1, Defs = [X1_Y] in
def PseudoCCALLIndirect : Pseudo<(outs), (ins  YGPRNoX0X1:$rs1),
                                 [(riscv_cap_call  YGPRNoX0X1:$rs1)]>,
                          PseudoInstExpansion<(CJALR X1_Y, YGPR:$rs1, 0)>;

let Predicates = [HasCheri, IsCapMode, IsPureCapABI],
    isBarrier = 1, isReturn = 1, isTerminator = 1 in
def PseudoCRET : Pseudo<(outs), (ins), [(riscv_ret_glue)]>,
                 PseudoInstExpansion<(CJALR X0_Y, X1_Y, 0)>;

let Predicates = [HasCheri, IsCapMode],
    isCall = 1, isTerminator = 1, isReturn = 1, isBarrier = 1, Uses = [X2_Y],
    isCodeGenOnly = 0, Size = 8 in
def PseudoCTAIL : Pseudo<(outs), (ins cap_call_symbol:$dst), []> {
  let AsmString = "ctail\t$dst";
}

let Predicates = [HasCheriot] in
def : InstAlias<"ct.ctail $dst", (PseudoCTAIL cap_call_symbol:$dst)>;

let Predicates = [HasCheri, IsCapMode, IsPureCapABI],
    isCall = 1, isTerminator = 1, isReturn = 1, isBarrier = 1, Uses = [X2_Y] in
def PseudoCTAILIndirect : Pseudo<(outs), (ins  YGPRNoX0X1:$rs1),
                                 [(riscv_cap_tail  YGPRNoX0X1:$rs1)]>,
                          PseudoInstExpansion<(CJALR X0_Y, YGPR:$rs1, 0)>;

let Predicates = [HasCheri, IsCapMode, IsPureCapABI] in {
def : Pat<(riscv_cap_tail tglobaladdr:$dst),
          (PseudoCTAIL texternalsym:$dst)>;
def : Pat<(riscv_cap_tail texternalsym:$dst),
          (PseudoCTAIL texternalsym:$dst)>;
} // Predicates = [HasCheri, IsCapMode, IsPureCapABI]

let Predicates = [HasCheri, IsCapMode, IsPureCapABI] in
let isCall = 0, isBarrier = 1, isBranch = 1, isTerminator = 1, Size = 8,
    isCodeGenOnly = 0, hasSideEffects = 0, mayStore = 0, mayLoad = 0 in
def PseudoCJump : Pseudo<(outs YGPR:$rd),
                         (ins pseudo_cap_jump_symbol:$target), []> {
  let AsmString = "cjump\t$target, $rd";
}

let Predicates = [HasCheriot] in def : InstAlias<
                                           "ct.cjump $target, $rd",
                                           (PseudoCJump YGPR:$rd,
                                               pseudo_cap_jump_symbol:$target)>;

defm : CheriLdPat<sextloadi8, CLB>, Requires<[HasCheri, IsCapMode]>;
defm : CheriLdPat<extloadi8, CLB>, Requires<[HasCheri, IsCapMode]>;
defm : CheriLdPat<sextloadi16, CLH>, Requires<[HasCheri, IsCapMode]>;
defm : CheriLdPat<extloadi16, CLH>, Requires<[HasCheri, IsCapMode]>;
defm : CheriLdPat<load, CLW>, Requires<[HasCheri, IsRV32, IsCapMode]>;
defm : CheriLdPat<zextloadi8, CLBU>, Requires<[HasCheri, IsCapMode]>;
defm : CheriLdPat<zextloadi16, CLHU>, Requires<[HasCheri, IsCapMode]>;

defm : CheriStPat<truncstorei8, CSB, GPR>, Requires<[HasCheri, IsCapMode]>;
defm : CheriStPat<truncstorei16, CSH, GPR>, Requires<[HasCheri, IsCapMode]>;
defm : CheriStPat<store, CSW, GPR>, Requires<[HasCheri, IsRV32, IsCapMode]>;

let Predicates = [HasCheri, IsRV64, IsCapMode] in {

/// Loads

defm : CheriLdPat<sextloadi32, CLW>;
defm : CheriLdPat<extloadi32, CLW>;
defm : CheriLdPat<zextloadi32, CLWU>;
defm : CheriLdPat<load, CLD>;

/// Stores

defm : CheriStPat<truncstorei32, CSW, GPR>;
defm : CheriStPat<store, CSD, GPR>;
} // Predicates = [HasCheri, IsRV64, IsCapMode]

/// Other pseudo-instructions

// Pessimistically assume the stack pointer will be clobbered
let Predicates = [HasCheri, IsPureCapABI] in {
let Defs = [X2_Y], Uses = [X2_Y] in {
def ADJCALLSTACKDOWNCAP : Pseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                                 [(callseq_start timm:$amt1, timm:$amt2)]>;
def ADJCALLSTACKUPCAP   : Pseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                                 [(callseq_end timm:$amt1, timm:$amt2)]>;
} // Defs = [X2_Y], Uses = [X2_Y]
} // Predicates = [HasCheri, IsPureCapABI]

/// 'A' (Atomic Instructions) extension

let Predicates = [HasCheri, HasStdExtA, IsCapMode] in {

/// Atomic loads and stores

// Fences will be inserted for atomic load/stores according to the logic in
// RISCVTargetLowering::{emitLeadingFence,emitTrailingFence}.

defm : CheriLdPat<atomic_load_8,  CLB>;
defm : CheriLdPat<atomic_load_16, CLH>;
defm : CheriLdPat<atomic_load_32, CLW>;

defm : CheriAtomicStPat<atomic_store_8,  CSB, GPR, XLenVT>;
defm : CheriAtomicStPat<atomic_store_16, CSH, GPR, XLenVT>;
defm : CheriAtomicStPat<atomic_store_32, CSW, GPR, XLenVT>;

/// AMOs

defm : CheriAMOPat<"atomic_swap_i32", "CAMOSWAP_W">;
defm : CheriAMOPat<"atomic_load_add_i32", "CAMOADD_W">;
defm : CheriAMOSubPat<"atomic_load_sub_i32", "CAMOADD_W">;
defm : CheriAMOPat<"atomic_load_and_i32", "CAMOAND_W">;
defm : CheriAMOPat<"atomic_load_or_i32", "CAMOOR_W">;
defm : CheriAMOPat<"atomic_load_xor_i32", "CAMOXOR_W">;
defm : CheriAMOPat<"atomic_load_max_i32", "CAMOMAX_W">;
defm : CheriAMOPat<"atomic_load_min_i32", "CAMOMIN_W">;
defm : CheriAMOPat<"atomic_load_umax_i32", "CAMOMAXU_W">;
defm : CheriAMOPat<"atomic_load_umin_i32", "CAMOMINU_W">;

/// Pseudo AMOs

defm : PseudoCheriAMOPat<"atomic_swap_i8", PseudoCheriAtomicSwap8>;
defm : PseudoCheriAMOPat<"atomic_load_add_i8", PseudoCheriAtomicLoadAdd8>;
defm : PseudoCheriAMOPat<"atomic_load_sub_i8", PseudoCheriAtomicLoadSub8>;
defm : PseudoCheriAMOPat<"atomic_load_and_i8", PseudoCheriAtomicLoadAnd8>;
defm : PseudoCheriAMOPat<"atomic_load_or_i8", PseudoCheriAtomicLoadOr8>;
defm : PseudoCheriAMOPat<"atomic_load_xor_i8", PseudoCheriAtomicLoadXor8>;
defm : PseudoCheriAMOPat<"atomic_load_max_i8", PseudoCheriAtomicLoadMax8>;
defm : PseudoCheriAMOPat<"atomic_load_min_i8", PseudoCheriAtomicLoadMin8>;
defm : PseudoCheriAMOPat<"atomic_load_umax_i8", PseudoCheriAtomicLoadUMax8>;
defm : PseudoCheriAMOPat<"atomic_load_umin_i8", PseudoCheriAtomicLoadUMin8>;
defm : PseudoCheriAMOPat<"atomic_load_nand_i8", PseudoCheriAtomicLoadNand8>;

defm : PseudoCheriAMOPat<"atomic_swap_i16", PseudoCheriAtomicSwap16>;
defm : PseudoCheriAMOPat<"atomic_load_add_i16", PseudoCheriAtomicLoadAdd16>;
defm : PseudoCheriAMOPat<"atomic_load_sub_i16", PseudoCheriAtomicLoadSub16>;
defm : PseudoCheriAMOPat<"atomic_load_and_i16", PseudoCheriAtomicLoadAnd16>;
defm : PseudoCheriAMOPat<"atomic_load_or_i16", PseudoCheriAtomicLoadOr16>;
defm : PseudoCheriAMOPat<"atomic_load_xor_i16", PseudoCheriAtomicLoadXor16>;
defm : PseudoCheriAMOPat<"atomic_load_max_i16", PseudoCheriAtomicLoadMax16>;
defm : PseudoCheriAMOPat<"atomic_load_min_i16", PseudoCheriAtomicLoadMin16>;
defm : PseudoCheriAMOPat<"atomic_load_umax_i16", PseudoCheriAtomicLoadUMax16>;
defm : PseudoCheriAMOPat<"atomic_load_umin_i16", PseudoCheriAtomicLoadUMin16>;
defm : PseudoCheriAMOPat<"atomic_load_nand_i16", PseudoCheriAtomicLoadNand16>;

defm : PseudoCheriAMOPat<"atomic_load_nand_i32", PseudoCheriAtomicLoadNand32>;

defm : PseudoCheriAMOPat<"atomic_load_add_cap", PseudoCheriAtomicLoadAddCap, YGPR>;
defm : PseudoCheriAMOPat<"atomic_load_sub_cap", PseudoCheriAtomicLoadSubCap, YGPR>;
defm : PseudoCheriAMOPat<"atomic_load_and_cap", PseudoCheriAtomicLoadAndCap, YGPR>;
defm : PseudoCheriAMOPat<"atomic_load_or_cap", PseudoCheriAtomicLoadOrCap, YGPR>;
defm : PseudoCheriAMOPat<"atomic_load_xor_cap", PseudoCheriAtomicLoadXorCap, YGPR>;
defm : PseudoCheriAMOPat<"atomic_load_max_cap", PseudoCheriAtomicLoadMaxCap, YGPR>;
defm : PseudoCheriAMOPat<"atomic_load_min_cap", PseudoCheriAtomicLoadMinCap, YGPR>;
defm : PseudoCheriAMOPat<"atomic_load_umax_cap", PseudoCheriAtomicLoadUMaxCap, YGPR>;
defm : PseudoCheriAMOPat<"atomic_load_umin_cap", PseudoCheriAtomicLoadUMinCap, YGPR>;
defm : PseudoCheriAMOPat<"atomic_load_nand_cap", PseudoCheriAtomicLoadNandCap, YGPR>;

/// Compare and exchange

defm : PseudoCheriCmpXchgPat<"atomic_cmp_swap_i8", PseudoCheriCmpXchg8>;
defm : PseudoCheriCmpXchgPat<"atomic_cmp_swap_i16", PseudoCheriCmpXchg16>;
defm : PseudoCheriCmpXchgPat<"atomic_cmp_swap_i32", PseudoCheriCmpXchg32>;

defm : PseudoCheriCmpXchgPat<"atomic_cmp_swap_cap", PseudoCheriCmpXchgCap, YGPR>;

} // Predicates = [HasCheri, HasStdExtA, IsCapMode]

let Predicates = [HasCheri, HasStdExtA, IsRV64, IsCapMode] in {

/// 64-bit atomic loads and stores

// Fences will be inserted for atomic load/stores according to the logic in
// RISCVTargetLowering::{emitLeadingFence,emitTrailingFence}.
defm : CheriLdPat<atomic_load_64, CLD>;
defm : CheriAtomicStPat<atomic_store_64, CSD, GPR, XLenVT>;

/// 64-bit AMOs

defm : CheriAMOPat<"atomic_swap_i64", "CAMOSWAP_D">;
defm : CheriAMOPat<"atomic_load_add_i64", "CAMOADD_D">;
defm : CheriAMOSubPat<"atomic_load_sub_i64", "CAMOADD_D">;
defm : CheriAMOPat<"atomic_load_and_i64", "CAMOAND_D">;
defm : CheriAMOPat<"atomic_load_or_i64", "CAMOOR_D">;
defm : CheriAMOPat<"atomic_load_xor_i64", "CAMOXOR_D">;
defm : CheriAMOPat<"atomic_load_max_i64", "CAMOMAX_D">;
defm : CheriAMOPat<"atomic_load_min_i64", "CAMOMIN_D">;
defm : CheriAMOPat<"atomic_load_umax_i64", "CAMOMAXU_D">;
defm : CheriAMOPat<"atomic_load_umin_i64", "CAMOMINU_D">;

/// 64-bit pseudo AMOs

defm : PseudoCheriAMOPat<"atomic_load_nand_i64", PseudoCheriAtomicLoadNand64>;

/// 64-bit compare and exchange

defm : PseudoCheriCmpXchgPat<"atomic_cmp_swap_i64", PseudoCheriCmpXchg64>;

} // Predicates = [HasCheri, HasStdExtA, IsRV64, IsCapMode]

let Predicates = [HasCheri, HasStdExtA, IsRV32, IsCapMode] in {
defm : CheriLdPat<atomic_load_cap, CLC_64, YLenVT>;
defm : CheriAtomicStPat<atomic_store_cap, CSC_64, YGPR, YLenVT>;
defm : CheriAMOCapPat<"64", "atomic_swap_cap", "CAMOSWAP_C">;
} // Predicates = [HasCheri, HasStdExtA, IsRV32, IsCapMode]

let Predicates = [HasCheri, HasStdExtA, IsRV64, IsCapMode] in {
defm : CheriLdPat<atomic_load_cap, CLC_128, YLenVT>;
defm : CheriAtomicStPat<atomic_store_cap, CSC_128, YGPR, YLenVT>;
defm : CheriAMOCapPat<"128", "atomic_swap_cap", "CAMOSWAP_C">;
} // Predicates = [HasCheri, HasStdExtA, IsRV64, IsCapMode]

/// 'F' (Single-Precision Floating-Point) extension

let Predicates = [HasCheri, HasStdExtF, IsCapMode] in {

/// Loads

defm : CheriLdPat<load, CFLW, f32>;

/// Stores

defm : CheriStPat<store, CFSW, FPR32, f32>;

} // Predicates = [HasCheri, HasStdExtF, IsCapMode]

/// 'D' (Single-Precision Floating-Point) extension

let Predicates = [HasCheri, HasStdExtD, IsCapMode] in {

/// Loads

defm : CheriLdPat<load, CFLD, f64>;

/// Stores

defm : CheriStPat<store, CFSD, FPR64, f64>;

} // Predicates = [HasCheri, HasStdExtD, IsCapMode]

/// 'XCheri' extension

let Predicates = [HasCheri, IsRV32, IsCapMode] in {
defm : CheriLdPat<load, CLC_64, YLenVT>;
defm : CheriStPat<store, CSC_64, YGPR, YLenVT>;
} // Predicates = [HasCheri, IsRV32, IsCapMode]

let Predicates = [HasCheri, IsRV64, IsCapMode] in {
defm : CheriLdPat<load, CLC_128, YLenVT>;
defm : CheriStPat<store, CSC_128, YGPR, YLenVT>;
} // Predicates = [HasCheri, IsRV64, IsCapMode]

let Predicates = [HasCheri, HasCheriot, IsRV32, IsCapMode] in {
  defm : CheriLdPat<load, CLC_64, f64>;
  defm : CheriStPat<store, CSC_64, YGPR, f64>;
} // Predicates = [HasCheri, HasCheriot, IsRV32, IsCapMode]

//===----------------------------------------------------------------------===//
// Compress Instruction tablegen backend.
//===----------------------------------------------------------------------===//

// Quadrant 0
let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode] in {
def : CompressPat<(CIncOffsetImm YGPRC:$rd, CSP:$rs1, uimm10_lsb00nonzero:$imm),
                  (C_CIncOffsetImm4CSPN YGPRC:$rd, CSP:$rs1, uimm10_lsb00nonzero:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, HasStdExtD, IsRV32, IsCapMode] in {
def : CompressPat<(CFLD FPR64C:$rd, YGPRC:$rs1, uimm8_lsb000:$imm),
                  (C_CFLD FPR64C:$rd, YGPRC:$rs1, uimm8_lsb000:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, HasStdExtD, IsRV32, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode] in {
def : CompressPat<(CLC_128 YGPRC:$rd, YGPRC:$rs1, uimm9_lsb0000:$imm),
                  (C_CLC_128 YGPRC:$rd, YGPRC:$rs1, uimm9_lsb0000:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode] in {
def : CompressPat<(CLW GPRC:$rd, YGPRC:$rs1, uimm7_lsb00:$imm),
                  (C_CLW GPRC:$rd, YGPRC:$rs1, uimm7_lsb00:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV32, IsCapMode] in {
def : CompressPat<(CLC_64 YGPRC:$rd, YGPRC:$rs1, uimm8_lsb000:$imm),
                  (C_CLC_64 YGPRC:$rd, YGPRC:$rs1, uimm8_lsb000:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV32, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode] in {
def : CompressPat<(CLD GPRC:$rd, YGPRC:$rs1, uimm8_lsb000:$imm),
                  (C_CLD GPRC:$rd, YGPRC:$rs1, uimm8_lsb000:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, HasStdExtD, IsRV32, IsCapMode] in {
def : CompressPat<(CFSD FPR64C:$rs2, YGPRC:$rs1, uimm8_lsb000:$imm),
                  (C_CFSD FPR64C:$rs2, YGPRC:$rs1, uimm8_lsb000:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, HasStdExtD, IsRV32, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode] in {
def : CompressPat<(CSC_128 YGPRC:$rs2, YGPRC:$rs1, uimm9_lsb0000:$imm),
                  (C_CSC_128 YGPRC:$rs2, YGPRC:$rs1, uimm9_lsb0000:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode] in {
def : CompressPat<(CSW GPRC:$rs2, YGPRC:$rs1, uimm7_lsb00:$imm),
                  (C_CSW GPRC:$rs2, YGPRC:$rs1, uimm7_lsb00:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV32, IsCapMode] in {
def : CompressPat<(CSC_64 YGPRC:$rs2, YGPRC:$rs1, uimm8_lsb000:$imm),
                  (C_CSC_64 YGPRC:$rs2, YGPRC:$rs1, uimm8_lsb000:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV32, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode] in {
def : CompressPat<(CSD GPRC:$rs2, YGPRC:$rs1, uimm8_lsb000:$imm),
                  (C_CSD GPRC:$rs2, YGPRC:$rs1, uimm8_lsb000:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode]

// Quadrant 1
let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV32, IsCapMode] in {
def : CompressPat<(CJAL X1_Y, bare_simm12_lsb0:$offset),
                  (C_CJAL bare_simm12_lsb0:$offset)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV32, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode] in {
def : CompressPat<(CIncOffsetImm X2_Y, X2_Y, simm10_lsb0000nonzero:$imm),
                  (C_CIncOffsetImm16CSP X2_Y, simm10_lsb0000nonzero:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode] in {
def : CompressPat<(CJAL X0_Y, bare_simm12_lsb0:$offset),
                  (C_J bare_simm12_lsb0:$offset)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode]

// Quadrant 2
let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, HasStdExtD, IsRV32, IsCapMode] in {
def : CompressPat<(CFLD FPR64:$rd, CSP:$rs1, uimm9_lsb000:$imm),
                  (C_CFLDCSP FPR64:$rd, CSP:$rs1, uimm9_lsb000:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, HasStdExtD, IsRV32, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode] in {
def : CompressPat<(CLC_128  YGPRNoX0:$rd, CSP:$rs1, uimm10_lsb0000:$imm),
                  (C_CLCCSP_128  YGPRNoX0:$rd, CSP:$rs1, uimm10_lsb0000:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode] in {
def : CompressPat<(CLW GPRNoX0:$rd, CSP:$rs1,  uimm8_lsb00:$imm),
                  (C_CLWCSP GPRNoX0:$rd, CSP:$rs1, uimm8_lsb00:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV32, IsCapMode] in {
def : CompressPat<(CLC_64  YGPRNoX0:$rd, CSP:$rs1, uimm9_lsb000:$imm),
                  (C_CLCCSP_64  YGPRNoX0:$rd, CSP:$rs1, uimm9_lsb000:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV32, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode] in {
def : CompressPat<(CLD GPRNoX0:$rd, CSP:$rs1, uimm9_lsb000:$imm),
                  (C_CLDCSP GPRNoX0:$rd, CSP:$rs1, uimm9_lsb000:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode] in {
def : CompressPat<(CJALR X0_Y,  YGPRNoX0:$rs1, 0),
                  (C_CJR  YGPRNoX0:$rs1)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode] in {
def : CompressPat<(CJALR X1_Y,  YGPRNoX0:$rs1, 0),
                  (C_CJALR  YGPRNoX0:$rs1)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, HasStdExtD, IsRV32, IsCapMode] in {
def : CompressPat<(CFSD FPR64:$rs2, CSP:$rs1, uimm9_lsb000:$imm),
                  (C_CFSDCSP FPR64:$rs2, CSP:$rs1, uimm9_lsb000:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, HasStdExtD, IsRV32, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode] in {
def : CompressPat<(CSC_128 YGPR:$rs2, CSP:$rs1, uimm10_lsb0000:$imm),
                  (C_CSCCSP_128 YGPR:$rs2, CSP:$rs1, uimm10_lsb0000:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode] in {
def : CompressPat<(CSW GPR:$rs2, CSP:$rs1, uimm8_lsb00:$imm),
                  (C_CSWCSP GPR:$rs2, CSP:$rs1, uimm8_lsb00:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV32, IsCapMode] in {
def : CompressPat<(CSC_64 YGPR:$rs2, CSP:$rs1, uimm9_lsb000:$imm),
                  (C_CSCCSP_64 YGPR:$rs2, CSP:$rs1, uimm9_lsb000:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV32, IsCapMode]

let Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode] in {
def : CompressPat<(CSD GPR:$rs2, CSP:$rs1, uimm9_lsb000:$imm),
                  (C_CSDCSP GPR:$rs2, CSP:$rs1, uimm9_lsb000:$imm)>;
} // Predicates = [HasCheri, HasCheriRVC, HasStdExtC, IsRV64, IsCapMode]


let Predicates = [HasCheri, IsRV32, IsCapMode, IsRVE] in {
let mayLoad = true, mayStore = false, hasSideEffects = false in
def PseudoCLLW : Pseudo<(outs YGPR:$dst), (ins bare_symbol:$src), [],
                        "cllc", "$dst, $src">;
def : Pat<(c64(load(cPTR(load(iPTR globaladdr:$src))))),
          (PseudoCLLW bare_symbol:$src)>;
def : Pat<(f64(load(cPTR(load(iPTR globaladdr:$src))))),
          (PseudoCLLW bare_symbol:$src)>;
} // Predicates = [HasCheri, IsRV32, IsCapMode, IsRVE]

// Cheriot stores f64 in cap registers, so bitcasting between f64 and c64
// is a no-op.
multiclass NopCapRegCast<ValueType Ty1, ValueType Ty2> {
  def : Pat<(Ty1(bitconvert(Ty2 YGPR:$Val))), (Ty1 YGPR:$Val)>;
  def : Pat<(Ty2(bitconvert(Ty1 YGPR:$Val))), (Ty2 YGPR:$Val)>;
}

let Predicates = [HasCheri, HasCheriot] in { defm : NopCapRegCast<c64, f64>; }
