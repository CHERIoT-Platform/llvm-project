//===--- RISCV.h - Declare RISC-V target feature support --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares RISC-V TargetInfo objects.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_BASIC_TARGETS_RISCV_H
#define LLVM_CLANG_LIB_BASIC_TARGETS_RISCV_H

#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "llvm/Support/Compiler.h"
#include "llvm/TargetParser/RISCVISAInfo.h"
#include "llvm/TargetParser/Triple.h"
#include <optional>

namespace clang {
namespace targets {

// RISC-V Target
class RISCVTargetInfo : public TargetInfo {
  void setDataLayout() {
    std::string Layout;
    IsABICHERIoT = false;
    IsABICHERIoTBareMetal = false;

    if (ABI == "ilp32" || ABI == "ilp32f" || ABI == "ilp32d" ||
        ABI == "cheriot" || ABI == "cheriot-baremetal" ||
        ABI == "il32pc64" || ABI == "il32pc64f" || ABI == "il32pc64d" ||
        ABI == "il32pc64e") {
      Layout += "e-m:e-p:32:32-i64:64-n32-S128";
      if (HasCheri)
        Layout += "-pf200:64:64:64:32";
    } else if (ABI == "ilp32e") {
      Layout = "e-m:e-p:32:32-i64:64-n32-S32";
      if (HasCheri)
        Layout += "-pf200:64:64:64:32";
    } else if (ABI == "lp64" || ABI == "lp64f" || ABI == "lp64d" ||
               ABI == "l64pc128" || ABI == "l64pc128f" ||
               ABI == "l64pc128d") {
      Layout = "e-m:e-p:64:64-i64:64-i128:128-n32:64-S128";
      if (HasCheri)
        Layout += "-pf200:128:128:128:64";
    } else if (ABI == "lp64e") {
      Layout = "e-m:e-p:64:64-i64:64-i128:128-n32:64-S64";
      if (HasCheri)
        Layout += "-pf200:128:128:128:64";
    } else
      llvm_unreachable("Invalid ABI");

    if (ABI == "cheriot" || ABI == "cheriot-baremetal") {
      IsABICHERIoT = true;
      if (ABI == "cheriot-baremetal")
        IsABICHERIoTBareMetal = true;
      EmptyParameterListIsVoid = true;
    }

    // Only set globals address space to 200 for cap-table mode
    if (CapabilityABI)
      Layout += "-A200-P200-G200";

    resetDataLayout(Layout);
  }

protected:
  std::string ABI, CPU;
  std::unique_ptr<llvm::RISCVISAInfo> ISAInfo;
  int CapSize = -1;
  bool HasCheri = false;
  bool IsABICHERIoT = false;
  bool IsABICHERIoTBareMetal = false;
  void setCapabilityABITypes() {
    IntPtrType = TargetInfo::SignedIntCap;
  }

private:
  bool FastScalarUnalignedAccess;
  bool HasExperimental = false;

public:
  RISCVTargetInfo(const llvm::Triple &Triple, const TargetOptions &)
      : TargetInfo(Triple) {
    BFloat16Width = 16;
    BFloat16Align = 16;
    BFloat16Format = &llvm::APFloat::BFloat();
    LongDoubleWidth = 128;
    LongDoubleAlign = 128;
    LongDoubleFormat = &llvm::APFloat::IEEEquad();
    SuitableAlign = 128;
    WCharType = SignedInt;
    WIntType = UnsignedInt;
    HasRISCVVTypes = true;
    MCountName = "_mcount";
    HasFloat16 = true;
    HasStrictFP = true;

    if (Triple.getSubArch() == llvm::Triple::RISCV32SubArch_cheriot_v1) {
      CPU = "cheriot";
      ABI = (Triple.getOS() == llvm::Triple::CheriotRTOS) ? "cheriot"
                                                          : "cheriot-baremetal";
    }
  }

  bool setCPU(const std::string &Name) override {
    if (!isValidCPUName(Name))
      return false;
    CPU = Name;
    return true;
  }

  StringRef getABI() const override { return ABI; }
  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override;

  llvm::SmallVector<Builtin::InfosShard> getTargetBuiltins() const override;

  BuiltinVaListKind getBuiltinVaListKind() const override {
    return TargetInfo::VoidPtrBuiltinVaList;
  }

  std::string_view getClobbers() const override { return ""; }

  StringRef getConstraintRegister(StringRef Constraint,
                                  StringRef Expression) const override {
    return Expression;
  }

  ArrayRef<const char *> getGCCRegNames() const override;

  int getEHDataRegisterNumber(unsigned RegNo) const override {
    if (RegNo == 0)
      return 10;
    else if (RegNo == 1)
      return 11;
    else
      return -1;
  }

  ArrayRef<TargetInfo::GCCRegAlias> getGCCRegAliases() const override;

  bool validateAsmConstraint(const char *&Name,
                             TargetInfo::ConstraintInfo &Info) const override;

  std::string convertConstraint(const char *&Constraint) const override;

  bool
  initFeatureMap(llvm::StringMap<bool> &Features, DiagnosticsEngine &Diags,
                 StringRef CPU,
                 const std::vector<std::string> &FeaturesVec) const override;

  std::optional<std::pair<unsigned, unsigned>>
  getVScaleRange(const LangOptions &LangOpts, ArmStreamingKind Mode,
                 llvm::StringMap<bool> *FeatureMap = nullptr) const override;

  bool hasFeature(StringRef Feature) const override;

  bool handleTargetFeatures(std::vector<std::string> &Features,
                            DiagnosticsEngine &Diags) override;

  unsigned getIntCapWidth() const override { return CapSize; }
  unsigned getIntCapAlign() const override { return CapSize; }

  uint64_t getCHERICapabilityWidth() const override { return CapSize; }

  uint64_t getCHERICapabilityAlign() const override { return CapSize; }

  uint64_t getPointerWidthV(LangAS AddrSpace) const override {
    return (AddrSpace == LangAS::cheri_capability) ? CapSize : PointerWidth;

  }

  uint64_t getPointerRangeV(LangAS AddrSpace) const override {
    return (AddrSpace == LangAS::cheri_capability) ? getPointerRangeForCHERICapability() : PointerWidth;
  }

  uint64_t getPointerAlignV(LangAS AddrSpace) const override {
    return (AddrSpace == LangAS::cheri_capability) ? CapSize : PointerAlign;
  }

  CallingConv getLibcallCallingConv() const override {
    return IsABICHERIoT && !IsABICHERIoTBareMetal ?
        CallingConv::CC_CHERILibCall : CallingConv::CC_C;
  }

  bool SupportsCapabilities() const override { return HasCheri; }

  bool validateTarget(DiagnosticsEngine &Diags) const override;

  bool hasBitIntType() const override { return true; }

  bool hasBFloat16Type() const override { return true; }

  CallingConvCheckResult checkCallingConvention(CallingConv CC) const override;

  bool useFP16ConversionIntrinsics() const override {
    return false;
  }

  bool isValidCPUName(StringRef Name) const override;
  void fillValidCPUList(SmallVectorImpl<StringRef> &Values) const override;
  bool isValidTuneCPUName(StringRef Name) const override;
  void fillValidTuneCPUList(SmallVectorImpl<StringRef> &Values) const override;
  bool supportsTargetAttributeTune() const override { return true; }
  ParsedTargetAttr parseTargetAttr(StringRef Str) const override;
  uint64_t getFMVPriority(ArrayRef<StringRef> Features) const override;

  std::pair<unsigned, unsigned> hardwareInterferenceSizes() const override {
    return std::make_pair(32, 32);
  }

  bool supportsCpuSupports() const override { return getTriple().isOSLinux(); }
  bool supportsCpuIs() const override { return getTriple().isOSLinux(); }
  bool supportsCpuInit() const override { return getTriple().isOSLinux(); }
  bool validateCpuSupports(StringRef Feature) const override;
  bool validateCpuIs(StringRef CPUName) const override;
  bool isValidFeatureName(StringRef Name) const override;

  bool validateGlobalRegisterVariable(StringRef RegName, unsigned RegSize,
                                      bool &HasSizeMismatch) const override;

  bool checkCFProtectionBranchSupported(DiagnosticsEngine &) const override {
    // Always generate Zicfilp lpad insns
    // Non-zicfilp CPUs would read them as NOP
    return true;
  }

  bool
  checkCFProtectionReturnSupported(DiagnosticsEngine &Diags) const override {
    if (ISAInfo->hasExtension("zicfiss"))
      return true;
    return TargetInfo::checkCFProtectionReturnSupported(Diags);
  }

  CFBranchLabelSchemeKind getDefaultCFBranchLabelScheme() const override {
    return CFBranchLabelSchemeKind::FuncSig;
  }

  bool
  checkCFBranchLabelSchemeSupported(const CFBranchLabelSchemeKind Scheme,
                                    DiagnosticsEngine &Diags) const override {
    switch (Scheme) {
    case CFBranchLabelSchemeKind::Default:
    case CFBranchLabelSchemeKind::Unlabeled:
    case CFBranchLabelSchemeKind::FuncSig:
      return true;
    }
    return TargetInfo::checkCFBranchLabelSchemeSupported(Scheme, Diags);
  }

  CheriCCallbackABIKind cheriCallbackKind() const override {
    return CCB_ImportTable;
  }
};
class LLVM_LIBRARY_VISIBILITY RISCV32TargetInfo : public RISCVTargetInfo {
public:
  RISCV32TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : RISCVTargetInfo(Triple, Opts) {
    IntPtrType = SignedInt;
    PtrDiffType = SignedInt;
    SizeType = UnsignedInt;
  }

  bool setABI(const std::string &Name) override {
    if (Name == "ilp32e") {
      ABI = Name;
      resetDataLayout("e-m:e-p:32:32-i64:64-n32-S32");
      return true;
    }

    if (Name == "ilp32" || Name == "ilp32f" || Name == "ilp32d") {
      ABI = Name;
      return true;
    }
    if (Name == "il32pc64" || Name == "il32pc64f" || Name == "il32pc64d" ||
        Name == "cheriot" || Name == "cheriot-baremetal") {
      setCapabilityABITypes();
      CapabilityABI = true;
      ABI = Name;
      // XXX -cheriot-bare-metal may not be honored
      return true;
    }
    return false;
  }

  void setMaxAtomicWidth() override {
    MaxAtomicPromoteWidth = 128;

    if (ISAInfo->hasExtension("a"))
      MaxAtomicInlineWidth = 32;
    else if (ISAInfo->hasExtension("xcheriot"))
      // XCheriot implies atomic libcalls up to cap size.
      MaxAtomicInlineWidth = 8;
  }

  uint64_t getPointerRangeForCHERICapability() const override { return 32; }
};
class LLVM_LIBRARY_VISIBILITY RISCV64TargetInfo : public RISCVTargetInfo {
public:
  RISCV64TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : RISCVTargetInfo(Triple, Opts) {
    LongWidth = LongAlign = PointerWidth = PointerAlign = 64;
    IntMaxType = Int64Type = SignedLong;
  }

  bool setABI(const std::string &Name) override {
    if (Name == "lp64e") {
      ABI = Name;
      resetDataLayout("e-m:e-p:64:64-i64:64-i128:128-n32:64-S64");
      return true;
    }

    if (Name == "lp64" || Name == "lp64f" || Name == "lp64d") {
      ABI = Name;
      return true;
    }
    if (Name == "l64pc128" || Name == "l64pc128f" || Name == "l64pc128d") {
      setCapabilityABITypes();
      CapabilityABI = true;
      ABI = Name;
      return true;
    }
    return false;
  }

  void setMaxAtomicWidth() override {
    MaxAtomicPromoteWidth = 128;

    if (ISAInfo->hasExtension("a"))
      MaxAtomicInlineWidth = 64;
  }

  uint64_t getPointerRangeForCHERICapability() const override { return 64; }
};
} // namespace targets
} // namespace clang

#endif // LLVM_CLANG_LIB_BASIC_TARGETS_RISCV_H
