//===--- CheriotHeapChecker.cpp - Heap management checker ------*- C++ -*-===//
//
// Clang Static Analyzer checker for CHERIoT heap management
//
//===----------------------------------------------------------------------===//

#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallDescription.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ProgramStateTrait.h"

using namespace clang;
using namespace ento;

namespace {

// State for tracking heap pointers from heap_claim and heap_claim_ephemeral
struct HeapPtrState {
  enum Kind : unsigned char {
    Unclaimed, // Pointer passed to a compartment call that is unclaimed
    Claimed,   // Claimed via heap_claim
    Ephemeral, // Claimed via heap_claim_ephemeral
    InvalidatedEphemeral, // Implicitly release by a cross-compartment call
    Escaped,              // Escaped or otherwise unknown
  };

  Kind K;

  HeapPtrState(Kind K) : K(K) {}

  bool isUnclaimed() const { return K == Unclaimed; }
  bool isClaimed() const { return K == Claimed; }
  bool isEphemeral() const { return K == Ephemeral; }
  bool isInvalidatedEphemeral() const { return K == InvalidatedEphemeral; }
  bool isEffectivelyClaimed() const {
    return K == Claimed || K == Ephemeral || K == Escaped;
  }

  bool operator==(const HeapPtrState &X) const { return K == X.K; }

  void Profile(llvm::FoldingSetNodeID &ID) const { ID.AddInteger(K); }
};

class CheriotHeapChecker
    : public Checker<check::PreCall, check::PostCall, check::Location,
                     check::BeginFunction, check::EndFunction,
                     check::PointerEscape> {
  const BugType LeakBugType{this, "Heap claim leak", "CHERIoT heap management"};
  const BugType InvalidUseBugType{this, "Invalid pointer use",
                                  "CHERIoT heap management"};

  using CheckFn = std::function<void(const class CheriotHeapChecker *,
                                     const CallEvent &Call, CheckerContext &C)>;

  const CallDescriptionMap<CheckFn> PreFnMap{
      {{CDM::SimpleFunc, {"check_pointer"}, 4},
       &CheriotHeapChecker::preCheckPointer},
  };

  const CallDescriptionMap<CheckFn> PostFnMap{
      {{CDM::SimpleFunc, {"heap_address_is_valid"}, 1},
       &CheriotHeapChecker::postHeapAddressIsValid},
      {{CDM::SimpleFunc, {"heap_claim"}, 2},
       &CheriotHeapChecker::postHeapClaim},
      {{CDM::SimpleFunc, {"heap_claim_ephemeral"}, 3},
       &CheriotHeapChecker::postHeapClaimEphemeral},
      {{CDM::SimpleFunc, {"heap_free"}, 2}, &CheriotHeapChecker::postHeapFree},
      {{CDM::SimpleFunc, {"heap_free_all"}, 1},
       &CheriotHeapChecker::postHeapFreeAll},
      {{CDM::SimpleFunc, {"token_obj_unseal"}, 2},
       &CheriotHeapChecker::postTokenObjUnseal},
  };

  const CallDescriptionSet SafeFnMap{
      {CDM::SimpleFunc, {"token_obj_unseal"}, 2},
  };

public:
  void checkPreCall(const CallEvent &Call, CheckerContext &C) const;
  void checkPostCall(const CallEvent &Call, CheckerContext &C) const;
  void checkLocation(SVal Loc, bool IsLoad, const Stmt *S,
                     CheckerContext &C) const;
  void checkBeginFunction(CheckerContext &C) const;
  void checkEndFunction(const ReturnStmt *RS, CheckerContext &C) const;
  ProgramStateRef checkPointerEscape(ProgramStateRef State,
                                     const InvalidatedSymbols &Escaped,
                                     const CallEvent *Call,
                                     PointerEscapeKind Kind) const;

  void preCheckPointer(const CallEvent &Call, CheckerContext &C) const;
  void postHeapAddressIsValid(const CallEvent &Call, CheckerContext &C) const;
  void postHeapClaim(const CallEvent &Call, CheckerContext &C) const;
  void postHeapClaimEphemeral(const CallEvent &Call, CheckerContext &C) const;
  void postHeapFree(const CallEvent &Call, CheckerContext &C) const;
  void postHeapFreeAll(const CallEvent &Call, CheckerContext &C) const;
  void postTokenObjUnseal(const CallEvent &Call, CheckerContext &C) const;

private:
  void reportLeak(SymbolRef Sym, CheckerContext &C) const;
};

} // anonymous namespace

REGISTER_TRAIT_WITH_PROGRAMSTATE(ExternalStateMutated, bool)
REGISTER_MAP_WITH_PROGRAMSTATE(HeapPointers, SymbolRef, HeapPtrState)

static bool shouldWarnOnDereferences(ProgramStateRef State) {
  if (State->get<ExternalStateMutated>())
    return true;

  // Any pending non-ephemeral claims count as state mutation,
  // unexpectedly terminating the compartment call without
  // releasing them could cause a leak.
  for (const auto &[Sym, HPS] : State->get<HeapPointers>()) {
    if (HPS.isClaimed())
      return true;
  }

  return false;
}

static bool isCrossCompartmentCall(const CallEvent &Call,
                                   const CheckerContext &C) {
  // Any call through a CC_CHERICCallback pointer is a compartment call.
  auto IsCompartmentCallbackCall = [](const CallEvent &Call) {
    const auto *CallE =
        dyn_cast<CallExpr>(Call.getOriginExpr()->IgnoreParenImpCasts());
    if (!CallE)
      return false;

    const Type *CalleeTy =
        CallE->getCallee()->getType()->getUnqualifiedDesugaredType();
    const auto *PT = dyn_cast<PointerType>(CalleeTy);
    if (!PT)
      return false;

    const auto *FT = dyn_cast<FunctionType>(
        PT->getPointeeType()->getUnqualifiedDesugaredType());
    if (!FT)
      return false;

    return FT->getCallConv() == CallingConv::CC_CHERICCallback;
  };
  if (IsCompartmentCallbackCall(Call))
    return true;

  const Decl *D = Call.getDecl();
  if (!D)
    return false;

  const auto *FD = dyn_cast<FunctionDecl>(D);
  if (!FD)
    return false;
  if (!FD->hasAttr<CHERICompartmentNameAttr>())
    return false;

  const auto *Attr = FD->getAttr<CHERICompartmentNameAttr>();
  StringRef CalleeCompartment = Attr->getCompartmentName();
  StringRef CallerCompartment =
      C.getASTContext().getLangOpts().CheriCompartmentName;

  return CalleeCompartment != CallerCompartment;
}

void CheriotHeapChecker::checkPreCall(const CallEvent &Call,
                                      CheckerContext &C) const {
  if (const auto *PreFN = PreFnMap.lookup(Call))
    (*PreFN)(this, Call, C);
}

void CheriotHeapChecker::checkPostCall(const CallEvent &Call,
                                       CheckerContext &C) const {
  if (const auto *PostFN = PostFnMap.lookup(Call)) {
    (*PostFN)(this, Call, C);
    return;
  }

  // Opaque calls to unrecognized, non-builtin functions may modify
  // state, which should cause us to enable warnings.
  ProgramStateRef State = C.getState();
  bool Changed = false;
  const FunctionDecl *FD = dyn_cast_or_null<FunctionDecl>(Call.getDecl());
  bool IsBuiltin = FD && FD->getBuiltinID() != 0;
  if (!IsBuiltin && !SafeFnMap.contains(Call)) {
    const Decl *RD = Call.getRuntimeDefinition().getDecl();
    if (!RD || !C.getAnalysisManager().getCFG(RD)) {
      State = State->set<ExternalStateMutated>(true);
      Changed = true;
    }
  }

  if (isCrossCompartmentCall(Call, C)) {
    // All ephemeral claims are implicitly released at each cross-compartment
    // call.
    for (const auto &[Sym, HPS] : State->get<HeapPointers>()) {
      if (!HPS.isEphemeral())
        continue;

      State = State->set<HeapPointers>(Sym, HeapPtrState::InvalidatedEphemeral);
      Changed = true;
    }
  }

  if (Changed)
    C.addTransition(State);
}

static void printSymbolNameForError(llvm::raw_ostream &os, SymbolRef Sym) {
  if (const auto *SymName = dyn_cast<SymbolData>(Sym)) {
    if (const VarRegion *VR =
            dyn_cast_or_null<VarRegion>(SymName->getOriginRegion())) {
      if (const VarDecl *VD = VR->getDecl())
        os << "'" << VD->getNameAsString() << "' ";
    }
  }
}

void CheriotHeapChecker::preCheckPointer(const CallEvent &Call,
                                         CheckerContext &C) const {
  ProgramStateRef State = C.getState();
  if (!shouldWarnOnDereferences(State))
    return;

  // If the pointer argument points to memory that could be heap memory,
  // check that it is in Claimed state, and report an error if not. This should
  // make sure to include arguments that are pointers to known stack or constant
  // memory.
  SVal PtrVal = Call.getArgSVal(0);
  SymbolRef Sym = PtrVal.getAsLocSymbol();
  if (!Sym)
    return;

  const HeapPtrState *HPS = State->get<HeapPointers>(Sym);
  if (!HPS || HPS->isEffectivelyClaimed())
    return;

  // Generate an error if the pointer doesn't have a valid claim
  ExplodedNode *N = C.generateErrorNode();
  if (!N)
    return;

  SmallString<200> buf;
  llvm::raw_svector_ostream os(buf);
  os << "check_pointer called on potential heap pointer ";
  printSymbolNameForError(os, Sym);

  if (HPS->isUnclaimed())
    os << "without a valid claim.";
  else if (HPS->isInvalidatedEphemeral())
    os << "after its ephemeral claim was released by a cross-compartment call.";

  auto Report =
      std::make_unique<PathSensitiveBugReport>(InvalidUseBugType, os.str(), N);
  Report->markInteresting(Sym);
  C.emitReport(std::move(Report));
}

void CheriotHeapChecker::postHeapAddressIsValid(const CallEvent &Call,
                                                CheckerContext &C) const {
  SymbolRef Sym = Call.getArgSVal(0).getAsLocSymbol();
  if (!Sym)
    return;

  SVal RetVal = Call.getReturnValue();
  ProgramStateRef State = C.getState();
  const HeapPtrState *HPS = State->get<HeapPointers>(Sym);
  if (!HPS)
    return;

  ProgramStateRef StateTrue, StateFalse;
  std::tie(StateTrue, StateFalse) =
      State->assume(RetVal.castAs<DefinedOrUnknownSVal>());

  if (StateTrue)
    C.addTransition(StateTrue);

  if (StateFalse) {
    StateFalse = StateFalse->remove<HeapPointers>(Sym);
    C.addTransition(StateFalse);
  }
}

void CheriotHeapChecker::postHeapClaim(const CallEvent &Call,
                                       CheckerContext &C) const {
  SymbolRef Sym = Call.getArgSVal(1).getAsLocSymbol();
  if (!Sym)
    return;

  ProgramStateRef State =
      C.getState()->set<HeapPointers>(Sym, HeapPtrState::Claimed);
  C.addTransition(State);
}

void CheriotHeapChecker::postHeapClaimEphemeral(const CallEvent &Call,
                                                CheckerContext &C) const {
  SymbolRef Sym1 = Call.getArgSVal(1).getAsLocSymbol();
  SymbolRef Sym2 = Call.getArgSVal(2).getAsLocSymbol();

  ProgramStateRef State = C.getState();

  // All existing ephemeral claims are released
  for (const auto &[Sym, HPS] : State->get<HeapPointers>()) {
    if (HPS.isEphemeral())
      State = State->set<HeapPointers>(Sym, HeapPtrState::Unclaimed);
  }

  if (Sym1)
    State = State->set<HeapPointers>(Sym1, HeapPtrState::Ephemeral);
  if (Sym2)
    State = State->set<HeapPointers>(Sym2, HeapPtrState::Ephemeral);

  C.addTransition(State);
}

void CheriotHeapChecker::postHeapFree(const CallEvent &Call,
                                      CheckerContext &C) const {
  SymbolRef Sym = Call.getArgSVal(1).getAsLocSymbol();
  if (!Sym)
    return;

  ProgramStateRef State = C.getState();
  const HeapPtrState *HPS = State->get<HeapPointers>(Sym);
  if (!HPS || !HPS->isEffectivelyClaimed())
    return;

  State = State->set<HeapPointers>(Sym, HeapPtrState::Unclaimed);
  C.addTransition(State);
}

void CheriotHeapChecker::postHeapFreeAll(const CallEvent &Call,
                                         CheckerContext &C) const {
  ProgramStateRef State = C.getState();
  for (const auto &[Sym, HPS] : State->get<HeapPointers>()) {
    State = State->set<HeapPointers>(Sym, HeapPtrState::Unclaimed);
  }
  C.addTransition(State);
}

void CheriotHeapChecker::postTokenObjUnseal(const CallEvent &Call,
                                            CheckerContext &C) const {
  // Unsealing a sealed cross-compartment pointer argument produces
  // a new pointer that should be treated the same as the original
  // for the purposes of analysis.
  ProgramStateRef State = C.getState();
  SymbolRef SealedSym = Call.getArgSVal(1).getAsLocSymbol();
  if (!State->contains<HeapPointers>(SealedSym))
    return;

  // Since the sealed pointer was undereferenceable, we can start
  // the new one off in unclaimed state.
  SVal RetVal = Call.getReturnValue();
  SymbolRef Sym = RetVal.getAsLocSymbol();
  State = State->set<HeapPointers>(Sym, HeapPtrState::Unclaimed);
  C.addTransition(State);
}

void CheriotHeapChecker::checkLocation(SVal Loc, bool IsLoad, const Stmt *S,
                                       CheckerContext &C) const {
  ProgramStateRef State = C.getState();
  SymbolRef Sym = Loc.getLocSymbolInBase();

  // If this is a write to non-stack memory that is not one of the
  // tracked compartment call arguments, then it is an internal state
  // change that could cause state desynchronization on compartment
  // crash.
  const HeapPtrState *HPS = Sym ? State->get<HeapPointers>(Sym) : nullptr;
  bool WarningsLive = shouldWarnOnDereferences(State);
  if (!WarningsLive && !IsLoad && !HPS) {
    const MemRegion *R = Loc.getAsRegion();
    if (R)
      R = R->StripCasts();
    if (!R || !isa<StackSpaceRegion>(R->getMemorySpace(State))) {
      State = State->set<ExternalStateMutated>(true);
      C.addTransition(State);
      return;
    }
  }

  if (!WarningsLive || !Sym)
    return;

  // This is a dereference of some form, so this is a bug if the
  // claim has already been released either by freeing or invalidation.
  if (!HPS || HPS->isEffectivelyClaimed())
    return;

  ExplodedNode *N = C.generateErrorNode();
  if (!N)
    return;

  SmallString<200> buf;
  llvm::raw_svector_ostream os(buf);
  if (IsLoad)
    os << "Read of heap pointer ";
  else
    os << "Store through heap pointer ";
  printSymbolNameForError(os, Sym);
  if (HPS->isUnclaimed())
    os << "without a valid claim.";
  else if (HPS->isInvalidatedEphemeral())
    os << "after its ephemeral claim was released by a cross-compartment call.";

  auto Report =
      std::make_unique<PathSensitiveBugReport>(InvalidUseBugType, os.str(), N);
  if (S)
    Report->addRange(S->getSourceRange());
  Report->markInteresting(Sym);
  C.emitReport(std::move(Report));
}

ProgramStateRef CheriotHeapChecker::checkPointerEscape(
    ProgramStateRef State, const InvalidatedSymbols &Escaped,
    const CallEvent *Call, PointerEscapeKind Kind) const {
  // FIXME: Unsure why this is needed.
  if (Kind == PointerEscapeKind::PSK_EscapeOnBind)
    return State;
  if (Call && PostFnMap.lookup(*Call))
    return State;
  for (SymbolRef Sym : Escaped) {
    if (State->get<HeapPointers>(Sym))
      State = State->set<HeapPointers>(Sym, HeapPtrState::Escaped);
  }
  return State;
}

void CheriotHeapChecker::checkBeginFunction(CheckerContext &C) const {
  const LocationContext *LC = C.getLocationContext();
  if (!LC->inTopFrame())
    return;

  // Only start analysis paths at functions
  const Decl *D = C.getLocationContext()->getDecl();
  const FunctionDecl *FD = dyn_cast_or_null<FunctionDecl>(D);
  if (!FD || !FD->hasAttr<CHERICompartmentNameAttr>()) {
    C.addSink();
    return;
  }

  // Mark all pointer arguments as initially unclaimed
  ProgramStateRef State = C.getState();
  bool Modified = false;
  for (unsigned i = 0; i < FD->getNumParams(); ++i) {
    const ParmVarDecl *Param = FD->getParamDecl(i);
    if (!Param->getType()->isPointerType())
      continue;

    const VarRegion *VR = State->getRegion(Param, LC);
    if (SymbolRef Sym = State->getSVal(VR).getAsLocSymbol()) {
      State = State->set<HeapPointers>(Sym, HeapPtrState::Unclaimed);
      Modified = true;
    }
  }

  // Only proceed with the analysis if at least one argument was a pointer.
  if (Modified)
    C.addTransition(State);
  else
    C.addSink();
}

void CheriotHeapChecker::checkEndFunction(const ReturnStmt *RS,
                                          CheckerContext &C) const {
  // Don't report leaks on returns from intra-compartment helper functions,
  // since the caller might contain the relevant releases.
  const Decl *D = C.getLocationContext()->getDecl();
  const FunctionDecl *FD = dyn_cast_or_null<FunctionDecl>(D);
  if (!FD || !FD->hasAttr<CHERICompartmentNameAttr>())
    return;

  ProgramStateRef State = C.getState();
  for (const auto &[Sym, HPS] : State->get<HeapPointers>()) {
    if (HPS.isClaimed())
      reportLeak(Sym, C);
  }
}

void CheriotHeapChecker::reportLeak(SymbolRef Sym, CheckerContext &C) const {
  ExplodedNode *N = C.generateErrorNode();
  if (!N)
    return;

  std::string Name = "";
  SmallString<200> buf;
  llvm::raw_svector_ostream os(buf);
  os << "Claim on pointer ";
  printSymbolNameForError(os, Sym);
  os << "must be released with heap_free or heap_free_all before returning "
        "from a compartment call.";

  auto Report =
      std::make_unique<PathSensitiveBugReport>(LeakBugType, os.str(), N);
  Report->markInteresting(Sym);
  C.emitReport(std::move(Report));
}

// Registration
void ento::registerCheriotHeapChecker(CheckerManager &mgr) {
  mgr.registerChecker<CheriotHeapChecker>();
}

bool ento::shouldRegisterCheriotHeapChecker(const CheckerManager &mgr) {
  const auto &TI = mgr.getASTContext().getTargetInfo();
  return TI.getTriple().getArch() == llvm::Triple::riscv32 &&
         TI.hasFeature("xcheriot");
}
