//===-- CheriotCompartmentCallbackChecker.cpp -------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Type.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugReporter.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ProgramStateTrait.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace ento;

namespace {

class CheriotCompartmentCallbackChecker
    : public Checker<check::BeginFunction, check::PreCall, check::DeadSymbols> {
  const BugType BT{this, "Invalid un-annotated function pointer call",
                   "CHERIoT compartmentalization"};

public:
  void checkBeginFunction(CheckerContext &C) const;
  void checkPreCall(const CallEvent &Call, CheckerContext &C) const;
  void checkDeadSymbols(SymbolReaper &SymReaper, CheckerContext &C) const;

private:
  void reportCall(SymbolRef Sym, const ParmVarDecl *Origin, const CallExpr *CE,
                  CheckerContext &C) const;
};

} // end anonymous namespace

// Maps the symbolic value of each untrusted function pointer to the
// parameter of the compartment entry point it arrived through. The parameter
// is kept so that we can report its name in diagnostics.
REGISTER_MAP_WITH_PROGRAMSTATE(ForeignFnPtrs, SymbolRef, const ParmVarDecl *)

static const FunctionType *getCalledFunctionType(QualType T) {
  if (const auto *PT = T->getAs<PointerType>())
    return PT->getPointeeType()->getAs<FunctionType>();
  if (const auto *RT = T->getAs<ReferenceType>())
    return RT->getPointeeType()->getAs<FunctionType>();
  return nullptr;
}

static bool isCompartmentCallback(const FunctionType *FT) {
  return FT && FT->getCallConv() == CC_CHERICCallback;
}

static bool isCompartmentEntryPoint(const FunctionDecl *FD) {
  return llvm::any_of(FD->redecls(), [](const FunctionDecl *Redecl) {
    return Redecl->hasAttr<CHERICompartmentNameAttr>();
  });
}

void CheriotCompartmentCallbackChecker::checkBeginFunction(
    CheckerContext &C) const {
  if (!C.inTopFrame())
    return;

  // Start analysis traces at compartment entry points.
  const StackFrame *SF = C.getStackFrame();
  const auto *FD = dyn_cast_or_null<FunctionDecl>(SF->getDecl());
  if (!FD || !isCompartmentEntryPoint(FD))
    return;

  ProgramStateRef State = C.getState();
  bool Changed = false;

  // Collect all non-callback function pointer arguments for tracking.
  for (const ParmVarDecl *PVD : FD->parameters()) {
    const FunctionType *FT = getCalledFunctionType(PVD->getType());
    if (!FT || isCompartmentCallback(FT))
      continue;

    const VarRegion *VR = State->getRegion(PVD, SF);
    SymbolRef Sym = State->getSVal(VR).getAsSymbol(/*IncludeBaseRegions=*/true);
    if (!Sym)
      continue;

    State = State->set<ForeignFnPtrs>(Sym, PVD);
    Changed = true;
  }

  if (Changed)
    C.addTransition(State);
}

void CheriotCompartmentCallbackChecker::checkPreCall(const CallEvent &Call,
                                                     CheckerContext &C) const {
  if (C.getState()->get<ForeignFnPtrs>().isEmpty())
    return;

  const auto *CE = dyn_cast_or_null<CallExpr>(Call.getOriginExpr());
  if (!CE || CE->getDirectCallee())
    return;

  // Use the call site's type so that we don't have to track casting.
  const Expr *Callee = CE->getCallee();
  const FunctionType *CallFT = getCalledFunctionType(Callee->getType());
  if (!CallFT)
    return;
  if (isCompartmentCallback(CallFT))
    return;

  SymbolRef Sym = C.getSVal(Callee).getAsSymbol(/*IncludeBaseRegions=*/true);
  if (!Sym)
    return;
  const ParmVarDecl *const *Origin = C.getState()->get<ForeignFnPtrs>(Sym);
  if (!Origin)
    return;

  reportCall(Sym, *Origin, CE, C);
}

void CheriotCompartmentCallbackChecker::checkDeadSymbols(
    SymbolReaper &SymReaper, CheckerContext &C) const {
  ProgramStateRef State = C.getState();
  bool Changed = false;
  for (const auto &[Sym, Origin] : State->get<ForeignFnPtrs>()) {
    if (SymReaper.isDead(Sym)) {
      State = State->remove<ForeignFnPtrs>(Sym);
      Changed = true;
    }
  }
  if (Changed)
    C.addTransition(State);
}

void CheriotCompartmentCallbackChecker::reportCall(SymbolRef Sym,
                                                   const ParmVarDecl *Origin,
                                                   const CallExpr *CE,
                                                   CheckerContext &C) const {
  ExplodedNode *N = C.generateNonFatalErrorNode();
  if (!N)
    return;

  SmallString<256> Buf;
  llvm::raw_svector_ostream OS(Buf);
  OS << "Calling function pointer that was passed into compartment entry "
        "point ";
  if (const auto *EntryFD = dyn_cast<FunctionDecl>(Origin->getDeclContext()))
    OS << "'" << EntryFD->getNameAsString() << "' ";
  OS << "as parameter '" << Origin->getName()
     << "' without the cheriot_ccallback attribute";

  auto Report = std::make_unique<PathSensitiveBugReport>(BT, OS.str(), N);
  Report->addRange(CE->getCallee()->getSourceRange());
  Report->markInteresting(Sym);
  Report->addNote("Function pointer received from another compartment here",
                  PathDiagnosticLocation::create(Origin, C.getSourceManager()));
  C.emitReport(std::move(Report));
}

// Registration

void ento::registerCheriotCompartmentCallbackChecker(CheckerManager &Mgr) {
  Mgr.registerChecker<CheriotCompartmentCallbackChecker>();
}

bool ento::shouldRegisterCheriotCompartmentCallbackChecker(
    const CheckerManager &Mgr) {
  const auto &TI = Mgr.getASTContext().getTargetInfo();
  return TI.getTriple().getArch() == llvm::Triple::riscv32 &&
         TI.hasFeature("xcheriot");
}
