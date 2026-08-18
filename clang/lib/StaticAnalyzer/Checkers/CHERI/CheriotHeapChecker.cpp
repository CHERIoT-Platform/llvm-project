//===--- CheriotHeapChecker.cpp - Heap management checker ------*- C++ -*-===//
//
// Clang Static Analyzer checker for CHERIoT heap management
//
//===----------------------------------------------------------------------===//

#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/StaticAnalyzer/Checkers/Taint.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallDescription.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ProgramStateTrait.h"

using namespace clang;
using namespace ento;
using namespace taint;

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

struct CheckPtrState {
  uint32_t Permissions = 0;
  uint32_t PermissionsKnown = 0;
  bool CheckStack = false;

  bool mayHavePermission(uint32_t P) const {
    if (!(PermissionsKnown & P))
      return true;
    return Permissions & P;
  }

  bool mayNotHavePermission(uint32_t P) const { return !mustHavePermission(P); }

  bool mustHavePermission(uint32_t P) const {
    if (!(PermissionsKnown & P))
      return false;
    return Permissions & P;
  }

  bool mustNotHavePermission(uint32_t P) const { return !mayHavePermission(P); }

  uint32_t getContradictoryPermissions(const CheckPtrState &Prior) const {
    return (Permissions & PermissionsKnown) &
           (~Prior.Permissions & Prior.PermissionsKnown);
  }

  CheckPtrState composeWithPriorState(const CheckPtrState &Prior) const {
    CheckPtrState NewState = *this;
    NewState.Permissions |=
        Prior.Permissions & Prior.PermissionsKnown & ~NewState.PermissionsKnown;
    NewState.Permissions &= ~(~Prior.Permissions & Prior.PermissionsKnown);
    NewState.PermissionsKnown |= Prior.PermissionsKnown;

    uint32_t Contradicting = getContradictoryPermissions(Prior);
    NewState.Permissions &= ~Contradicting;
    NewState.PermissionsKnown &= ~Contradicting;

    return NewState;
  }

  bool operator==(const CheckPtrState &X) const {
    return Permissions == X.Permissions &&
           PermissionsKnown == X.PermissionsKnown && CheckStack == X.CheckStack;
  }

  void Profile(llvm::FoldingSetNodeID &ID) const {
    ID.AddInteger(Permissions);
    ID.AddInteger(PermissionsKnown);
    ID.AddBoolean(CheckStack);
  }

  static constexpr uint32_t PermissionGlobal = 1 << 0;
  static constexpr uint32_t PermissionLoadGlobal = 1 << 1;
  static constexpr uint32_t PermissionStore = 1 << 2;
  static constexpr uint32_t PermissionLoadMutable = 1 << 3;
  static constexpr uint32_t PermissionStoreLocal = 1 << 4;
  static constexpr uint32_t PermissionLoad = 1 << 5;
  static constexpr uint32_t PermissionLoadStoreCap = 1 << 6;
  static constexpr uint32_t PermissionAccessSystemRegisters = 1 << 7;
  static constexpr uint32_t PermissionExecute = 1 << 8;
  static constexpr uint32_t PermissionUnseal = 1 << 9;
  static constexpr uint32_t PermissionSeal = 1 << 10;
  static constexpr uint32_t PermissionUser0 = 1 << 11;
};

class CheriotHeapChecker
    : public Checker<check::PreCall, check::PostCall, check::Location,
                     check::BeginFunction, check::EndFunction,
                     check::PointerEscape> {
  const BugType LeakBugType{this, "Heap claim leak", "CHERIoT heap management"};
  const BugType InvalidUseBugType{this, "Invalid pointer use",
                                  "CHERIoT heap management"};
  const BugType CheckPointerMisuseBugType{this, "Inconsistent check_pointer",
                                          "CHERIoT heap management"};

  bool requiresGlobalStateMutation(const BugType &BT) const {
    return &BT == &InvalidUseBugType;
  }

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
      {{CDM::SimpleFunc, {"CHERI", "check_pointer"}},
       &CheriotHeapChecker::postCXXCheckPointer},
      {{CDM::SimpleFunc, {"check_timeout_pointer"}},
       &CheriotHeapChecker::postCheckTimeoutPointer},
      {{CDM::SimpleFunc, {"timeout_is_valid"}},
       &CheriotHeapChecker::postCheckTimeoutPointer},
      {{CDM::SimpleFunc, {"setjmp"}}, &CheriotHeapChecker::postSetJmp},
  };

  const CallDescriptionSet UnsealingFns{
      {CDM::SimpleFunc, {"token_obj_unseal"}, 2},
  };

  const CallDescriptionSet SafeFnMap{
      {CDM::SimpleFunc, {"heap_address_is_valid"}, 1},
      {CDM::SimpleFunc, {"heap_claim"}, 2},
      {CDM::SimpleFunc, {"heap_claim_ephemeral"}, 3},
      {CDM::SimpleFunc, {"heap_free"}, 2},
      {CDM::SimpleFunc, {"heap_free_all"}, 1},
      {CDM::SimpleFunc, {"check_timeout_pointer"}, 1},
      {CDM::SimpleFunc, {"timeout_is_valid"}, 1},
      {CDM::SimpleFunc, {"setjmp"}, 2},
      {CDM::SimpleFunc, {"token_obj_unseal"}, 2},
      {CDM::SimpleFunc, {"check_pointer"}, 4},
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
  void postCXXCheckPointer(const CallEvent &Call, CheckerContext &C) const;
  void postCheckTimeoutPointer(const CallEvent &Call, CheckerContext &C) const;
  void postSetJmp(const CallEvent &Call, CheckerContext &C) const;

private:
  void reportLeak(SymbolRef Sym, CheckerContext &C) const;
  void reportDerefOfUnclaimedPointer(SymbolRef Sym, bool IsLoad, const Stmt *S,
                                     const HeapPtrState *HPS,
                                     CheckerContext &C) const;
  void reportDerefMissingPerms(SymbolRef Sym, bool IsLoad, const Stmt *S,
                               const CheckPtrState *CPS,
                               CheckerContext &C) const;
  void reportDerefOfUntaggedCapability(SVal Loc, bool IsLoad, const Stmt *S,
                                       CheckerContext &C) const;
  void reportWriteThroughReadOnlyCap(SVal Loc, const Stmt *S,
                                     CheckerContext &C) const;
  void reportContradictoryCheckPtr(SymbolRef Sym, uint32_t Contradictions,
                                   const CallEvent &CE,
                                   CheckerContext &C) const;
};

static constexpr TaintTagType TaintTagCapabilityUntagged = 1;
static constexpr TaintTagType TaintTagCapabilityReadOnly = 2;

} // anonymous namespace

REGISTER_TRAIT_WITH_PROGRAMSTATE(ExternalStateMutated, const Stmt *)
REGISTER_MAP_WITH_PROGRAMSTATE(HeapPointers, SymbolRef, HeapPtrState)
REGISTER_MAP_WITH_PROGRAMSTATE(CheckedPointers, SymbolRef, CheckPtrState)

static ProgramStateRef addGlobalStateMutation(ProgramStateRef State,
                                              const Stmt *S) {
  if (State->get<ExternalStateMutated>())
    return State;
  return State->set<ExternalStateMutated>(S);
}

static bool warningsEnabled(ProgramStateRef State) {
  if (State->get<ExternalStateMutated>())
    return true;

  for (const auto &[Sym, HPS] : State->get<HeapPointers>())
    if (HPS.isClaimed())
      return true;

  return false;
}

static SymbolRef getAttributableClaim(ProgramStateRef State) {
  SymbolRef Attributed = nullptr;

  // If there are multiple live claims, choose one deterministically to report.
  for (const auto &[Sym, HPS] : State->get<HeapPointers>()) {
    if (!HPS.isClaimed())
      continue;
    if (!Attributed || Sym->getSymbolID() < Attributed->getSymbolID())
      Attributed = Sym;
  }
  return Attributed;
}

static void explainWarningsEnabled(PathSensitiveBugReport &Report,
                                   CheckerContext &C) {
  ProgramStateRef State = C.getState();
  if (State->get<ExternalStateMutated>())
    return;

  if (SymbolRef Claim = getAttributableClaim(State))
    Report.markInteresting(Claim);
}

static bool isCrossCompartmentCall(const CallEvent &Call,
                                   const CheckerContext &C) {
  // Any call through a CC_CHERICCallback pointer is a compartment call.
  auto IsCompartmentCallbackCall = [](const CallEvent &Call) {
    const auto *OrigE = Call.getOriginExpr();
    if (!OrigE)
      return false;
    const auto *CallE = dyn_cast<CallExpr>(OrigE->IgnoreParenImpCasts());
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
  ProgramStateRef OldState = C.getState();
  ProgramStateRef State = OldState;
  bool GlobalStateMutation = false;
  const FunctionDecl *FD = dyn_cast_or_null<FunctionDecl>(Call.getDecl());
  bool IsBuiltin = FD && FD->getBuiltinID() != 0;
  if (!IsBuiltin && !SafeFnMap.contains(Call)) {
    const Decl *RD = Call.getRuntimeDefinition().getDecl();
    if (!RD || !C.getAnalysisManager().getCFG(RD)) {
      ProgramStateRef WithMutation =
          addGlobalStateMutation(State, Call.getOriginExpr());
      if (WithMutation != State) {
        State = WithMutation;
        GlobalStateMutation = true;
      }
    }
  }

  llvm::SmallVector<SymbolRef, 2> Invalidated;
  if (isCrossCompartmentCall(Call, C)) {
    // All ephemeral claims are implicitly released at each cross-compartment
    // call.
    for (const auto &[Sym, HPS] : State->get<HeapPointers>()) {
      if (!HPS.isEphemeral())
        continue;

      State = State->set<HeapPointers>(Sym, HeapPtrState::InvalidatedEphemeral);
      Invalidated.push_back(Sym);
    }
  }

  // Unsealing a pointer propagates claim state to the unsealed pointer.
  if (UnsealingFns.contains(Call)) {
    SymbolRef SealedSym = Call.getArgSVal(1).getAsLocSymbol();
    SymbolRef UnsealedSym = Call.getReturnValue().getAsLocSymbol();
    if (SealedSym && UnsealedSym) {
      if (const HeapPtrState *HPS = State->get<HeapPointers>(SealedSym)) {
        State = State->set<HeapPointers>(UnsealedSym, HPS->K);
      }
    }
  }

  const NoteTag *T = C.getNoteTag([this, Invalidated, GlobalStateMutation](
                                      PathSensitiveBugReport &BR,
                                      llvm::raw_ostream &OS) {
    if (llvm::any_of(Invalidated, [&](const SymbolRef &Sym) {
          return BR.isInteresting(Sym);
        })) {
      OS << "Ephemeral claims dropped by cross-compartment call here";
    }

    if (GlobalStateMutation && requiresGlobalStateMutation(BR.getBugType())) {
      OS << "Externally visible global state potentially mutated by external "
            "call here";
    }
  });

  if (State != OldState)
    C.addTransition(State, T);
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
  if (!warningsEnabled(State))
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
  explainWarningsEnabled(*Report, C);
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

void CheriotHeapChecker::postCheckTimeoutPointer(const CallEvent &Call,
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

  if (StateTrue) {
    StateTrue = StateTrue->remove<HeapPointers>(Sym);
    C.addTransition(StateTrue);
  }

  if (StateFalse)
    C.addTransition(StateFalse);
}

void CheriotHeapChecker::postSetJmp(const CallEvent &Call,
                                    CheckerContext &C) const {
  // Assume that setjmp always returns non-zero, i.e. that we are not
  // analyzing the first return. The primary use case for setjmp
  // on CHERIoT is implementing the CHERIOT_DURING / CHERIOT_HANDLER
  // unwinding. In that scenario, all code executed between the
  // first and second return is guarded by the handler, in which
  // case we assume that any heap errors that occur during that
  // region will be properly cleaned up by the handler.
  ProgramStateRef State = C.getState();
  State =
      State->assume(Call.getReturnValue().castAs<DefinedOrUnknownSVal>(), true);
  if (!State) {
    C.generateSink(C.getState(), C.getPredecessor());
    return;
  }

  C.addTransition(State);
}

void CheriotHeapChecker::postHeapClaim(const CallEvent &Call,
                                       CheckerContext &C) const {
  SymbolRef Sym = Call.getArgSVal(1).getAsLocSymbol();
  if (!Sym)
    return;

  // If the allocation capability was also a cross-compartment argument,
  // then it's possible / likely that the caller will free the claim, so
  // we treat it as effectively escaped.
  //
  // This situation arises commonly where the callee is claiming on behalf
  // of the caller, with claim release by the caller as an explicit part
  // of the function contract.
  HeapPtrState::Kind K = HeapPtrState::Claimed;
  SymbolRef AllocCap = Call.getArgSVal(0).getAsLocSymbol();
  if (AllocCap && C.getState()->contains<HeapPointers>(AllocCap))
    K = HeapPtrState::Escaped;

  ProgramStateRef State = C.getState()->set<HeapPointers>(Sym, K);

  // Assume that the claim always succeeds.
  BasicValueFactory &BVF = C.getSValBuilder().getBasicValueFactory();
  QualType RT = Call.getResultType();
  State = State->assumeInclusiveRange(
      Call.getReturnValue().castAs<DefinedOrUnknownSVal>(), BVF.getValue(1, RT),
      BVF.getMaxValue(RT), true);
  if (!State) {
    C.generateSink(C.getState(), C.getPredecessor());
    return;
  }

  const NoteTag *T =
      C.getNoteTag([=](PathSensitiveBugReport &BR, llvm::raw_ostream &OS) {
        if (!BR.isInteresting(Sym))
          return;
        OS << "Claim acquired here";
      });

  C.addTransition(State, T);
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

  // Assume that the claim always succeeds.
  State = State->assume(Call.getReturnValue().castAs<DefinedOrUnknownSVal>(),
                        false);
  if (!State) {
    C.generateSink(C.getState(), C.getPredecessor());
    return;
  }

  const NoteTag *T =
      C.getNoteTag([=](PathSensitiveBugReport &BR, llvm::raw_ostream &OS) {
        if (!BR.isInteresting(Sym1) && !BR.isInteresting(Sym2))
          return;
        OS << "Ephemeral claim acquired here";
      });

  C.addTransition(State, T);
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
  const NoteTag *T =
      C.getNoteTag([=](PathSensitiveBugReport &BR, llvm::raw_ostream &OS) {
        if (!BR.isInteresting(Sym))
          return;
        OS << "Claim dropped here";
      });

  C.addTransition(State, T);
}

void CheriotHeapChecker::postHeapFreeAll(const CallEvent &Call,
                                         CheckerContext &C) const {
  ProgramStateRef State = C.getState();

  llvm::SmallVector<SymbolRef, 2> Invalidated;
  for (const auto &[Sym, HPS] : State->get<HeapPointers>()) {
    State = State->set<HeapPointers>(Sym, HeapPtrState::Unclaimed);
    Invalidated.push_back(Sym);
  }

  const NoteTag *T =
      C.getNoteTag([=](PathSensitiveBugReport &BR, llvm::raw_ostream &OS) {
        if (llvm::none_of(Invalidated, [&](const SymbolRef &Sym) {
              return BR.isInteresting(Sym);
            }))
          return;
        OS << "All claims dropped here";
      });

  C.addTransition(State, T);
}

static uint32_t
getPermissionsFromCXXCheckPointerArg(const TemplateArgument &Arg) {
  const ValueDecl *VD = Arg.getAsDecl();
  const TemplateParamObjectDecl *TPOD = cast<TemplateParamObjectDecl>(VD);
  const APValue &PermissionSetVal = TPOD->getValue();
  const APValue &RawPerms = PermissionSetVal.getStructField(0);
  return RawPerms.getInt().getExtValue();
}

void CheriotHeapChecker::reportContradictoryCheckPtr(SymbolRef Sym,
                                                     uint32_t Contradictions,
                                                     const CallEvent &CE,
                                                     CheckerContext &C) const {
  ExplodedNode *N = C.generateErrorNode();
  if (!N)
    return;

  SmallString<200> buf;
  llvm::raw_svector_ostream os(buf);
  os << "check_pointer called multiple times on pointer ";
  printSymbolNameForError(os, Sym);
  os << "with required permissions that were removed by a prior call (";

  bool PrintBar = false;
  auto RenderPermission = [&](uint32_t Perm, const char *S) {
    if (!(Contradictions & Perm))
      return;
    if (PrintBar)
      os << "|";
    os << S;
    PrintBar = true;
  };

  RenderPermission(CheckPtrState::PermissionGlobal, "GL");
  RenderPermission(CheckPtrState::PermissionLoadGlobal, "LG");
  RenderPermission(CheckPtrState::PermissionStore, "SD");
  RenderPermission(CheckPtrState::PermissionLoadMutable, "LM");
  RenderPermission(CheckPtrState::PermissionStoreLocal, "SL");
  RenderPermission(CheckPtrState::PermissionLoad, "LD");
  RenderPermission(CheckPtrState::PermissionLoadStoreCap, "MC");
  RenderPermission(CheckPtrState::PermissionAccessSystemRegisters, "SR");
  RenderPermission(CheckPtrState::PermissionExecute, "EX");
  RenderPermission(CheckPtrState::PermissionUnseal, "US");
  RenderPermission(CheckPtrState::PermissionSeal, "SE");
  RenderPermission(CheckPtrState::PermissionUser0, "U0");

  os << ").";

  auto Report = std::make_unique<PathSensitiveBugReport>(
      CheckPointerMisuseBugType, os.str(), N);
  Report->addRange(CE.getSourceRange());
  Report->markInteresting(Sym);
  C.emitReport(std::move(Report));
}

void CheriotHeapChecker::postCXXCheckPointer(const CallEvent &Call,
                                             CheckerContext &C) const {
  ProgramStateRef State = C.getState();
  SVal RefArg = Call.getArgSVal(0);
  SVal PtrVal = State->getSVal(RefArg.castAs<Loc>());
  SymbolRef Sym = PtrVal.getAsLocSymbol();
  if (!Sym)
    return;

  const FunctionDecl *FD = dyn_cast_or_null<FunctionDecl>(Call.getDecl());
  if (!FD)
    return;

  const FunctionTemplateSpecializationInfo *FTSI =
      FD->getTemplateSpecializationInfo();
  if (!FTSI)
    return;

  const TemplateArgumentList *TemplateArgs = FTSI->TemplateArguments;
  if (TemplateArgs->size() != 4)
    return;

  uint32_t Permissions =
      getPermissionsFromCXXCheckPointerArg(TemplateArgs->get(0));
  bool CheckStack = TemplateArgs->get(1).getAsIntegral().getExtValue();
  bool EnforceStrictPermissions =
      TemplateArgs->get(2).getAsIntegral().getExtValue();

  CheckPtrState NewlyChecked = {
      .Permissions = Permissions,
      .PermissionsKnown = EnforceStrictPermissions ? ~0U : Permissions,
      .CheckStack = CheckStack,
  };

  if (const CheckPtrState *CPS = State->get<CheckedPointers>(Sym)) {
    uint32_t Contradictions = NewlyChecked.getContradictoryPermissions(*CPS);
    if (Contradictions)
      reportContradictoryCheckPtr(Sym, Contradictions, Call, C);
    NewlyChecked = NewlyChecked.composeWithPriorState(*CPS);
  }

  State = State->set<CheckedPointers>(Sym, std::move(NewlyChecked));
  C.addTransition(State);
}

void CheriotHeapChecker::reportDerefOfUnclaimedPointer(
    SymbolRef Sym, bool IsLoad, const Stmt *S, const HeapPtrState *HPS,
    CheckerContext &C) const {
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
    os << "after its ephemeral claim was released by a cross-compartment "
          "call.";

  auto Report =
      std::make_unique<PathSensitiveBugReport>(InvalidUseBugType, os.str(), N);
  if (S)
    Report->addRange(S->getSourceRange());
  Report->markInteresting(Sym);
  explainWarningsEnabled(*Report, C);
  C.emitReport(std::move(Report));
}

void CheriotHeapChecker::reportDerefMissingPerms(SymbolRef Sym, bool IsLoad,
                                                 const Stmt *S,
                                                 const CheckPtrState *CPS,
                                                 CheckerContext &C) const {
  ExplodedNode *N = C.generateErrorNode();
  if (!N)
    return;

  SmallString<200> buf;
  llvm::raw_svector_ostream os(buf);
  if (IsLoad)
    os << "Load through heap pointer ";
  else
    os << "Store through heap pointer ";
  printSymbolNameForError(os, Sym);
  os << "without passing the appropriate permission (";
  os << (IsLoad ? "LD" : "SD");
  os << ") to check_pointer.";
  if (CPS->mayHavePermission(IsLoad ? CheckPtrState::PermissionLoad
                                    : CheckPtrState::PermissionStore))
    os << " Runtime behavior will depend on the permissions provided by the "
          "caller. Use the EnforceStrictPermissions template parameter to "
          "check_pointer to enforce consistency across callers.";

  auto Report =
      std::make_unique<PathSensitiveBugReport>(InvalidUseBugType, os.str(), N);
  if (S)
    Report->addRange(S->getSourceRange());
  Report->markInteresting(Sym);
  explainWarningsEnabled(*Report, C);
  C.emitReport(std::move(Report));
}

void CheriotHeapChecker::reportDerefOfUntaggedCapability(
    SVal Loc, bool IsLoad, const Stmt *S, CheckerContext &C) const {
  ExplodedNode *N = C.generateErrorNode();
  if (!N)
    return;

  SmallString<200> buf;
  llvm::raw_svector_ostream os(buf);
  if (IsLoad)
    os << "Load through pointer ";
  else
    os << "Store through pointer ";
  printSymbolNameForError(os, Loc.getLocSymbolInBase());
  os << "which may be an invalid capability because MC permission was not "
        "checked before the pointer was loaded.";

  auto Report =
      std::make_unique<PathSensitiveBugReport>(InvalidUseBugType, os.str(), N);
  if (S) {
    Report->addRange(S->getSourceRange());
    if (isa<Expr>(S))
      bugreporter::trackExpressionValue(N, cast<Expr>(S), *Report);
  }
  Report->markInteresting(Loc);
  explainWarningsEnabled(*Report, C);
  C.emitReport(std::move(Report));
}

void CheriotHeapChecker::reportWriteThroughReadOnlyCap(
    SVal Loc, const Stmt *S, CheckerContext &C) const {
  ExplodedNode *N = C.generateErrorNode();
  if (!N)
    return;

  SmallString<200> buf;
  llvm::raw_svector_ostream os(buf);
  os << "Store through pointer ";
  printSymbolNameForError(os, Loc.getLocSymbolInBase());
  os << "which may be a read-only capability because LM permission was not "
        "checked before the pointer was loaded.";

  auto Report =
      std::make_unique<PathSensitiveBugReport>(InvalidUseBugType, os.str(), N);
  if (S) {
    Report->addRange(S->getSourceRange());
    if (isa<Expr>(S))
      bugreporter::trackExpressionValue(N, cast<Expr>(S), *Report);
  }
  Report->markInteresting(Loc);
  explainWarningsEnabled(*Report, C);
  C.emitReport(std::move(Report));
}

void CheriotHeapChecker::checkLocation(SVal Loc, bool IsLoad, const Stmt *S,
                                       CheckerContext &C) const {
  ProgramStateRef State = C.getState();
  ProgramStateRef OldState = State;
  SymbolRef Sym = Loc.getLocSymbolInBase();
  const MemRegion *Region = Loc.getAsRegion();

  // If this is a write to non-stack memory that is not one of the
  // tracked compartment call arguments, then it is an internal state
  // change that could cause state desynchronization on compartment
  // crash.
  bool CompartmentCrashWarningsLive = warningsEnabled(State);
  const HeapPtrState *HPS = Sym ? State->get<HeapPointers>(Sym) : nullptr;
  if (!IsLoad && !HPS) {
    if (!Region || !isa<StackSpaceRegion>(Region->getMemorySpace(State)))
      State = addGlobalStateMutation(State, S);
  }

  bool GlobalStateMutation = State->get<ExternalStateMutated>() !=
                             OldState->get<ExternalStateMutated>();
  const NoteTag *T = C.getNoteTag([=](PathSensitiveBugReport &BR,
                                      llvm::raw_ostream &OS) {
    if (GlobalStateMutation && requiresGlobalStateMutation(BR.getBugType())) {
      OS << "Externally visible global state mutated here";
    }
  });

  if (!Sym) {
    if (State != OldState)
      C.addTransition(State, T);
    return;
  }

  const CheckPtrState *CPS = State->contains<CheckedPointers>(Sym)
                                 ? State->get<CheckedPointers>(Sym)
                                 : nullptr;

  // Taint-based permissions checks need to be done in increasing order of
  // severity, as only the last applied taint will be propagated.
  bool MissingLM =
      IsLoad && Sym->getType()->getPointeeType()->isPointerType() && CPS &&
      CPS->mayNotHavePermission(CheckPtrState::PermissionLoadMutable);
  if (MissingLM) {
    SVal Loaded = State->getSVal(Loc.castAs<::clang::ento::Loc>());
    State = addTaint(State, Loaded, TaintTagCapabilityReadOnly);
  }

  bool MissingMC =
      IsLoad && Sym->getType()->getPointeeType()->isPointerType() && CPS &&
      CPS->mayNotHavePermission(CheckPtrState::PermissionLoadStoreCap);
  if (MissingMC) {
    SVal Loaded = State->getSVal(Loc.castAs<::clang::ento::Loc>());
    State = addTaint(State, Loaded, TaintTagCapabilityUntagged);
  }

  if (!CompartmentCrashWarningsLive) {
    // If we see a write through a heap pointer at a time when we're
    // not reporting compartment crashes, then we take that as an
    // assertion that the pointer is actually claimed.
    State = State->set<HeapPointers>(Sym, HeapPtrState::Escaped);
    C.addTransition(State, T);
    return;
  }

  if (HPS && !HPS->isEffectivelyClaimed())
    reportDerefOfUnclaimedPointer(Sym, IsLoad, S, HPS, C);

  bool MissingLD =
      IsLoad && CPS && CPS->mayNotHavePermission(CheckPtrState::PermissionLoad);
  bool MissingSD = !IsLoad && CPS &&
                   CPS->mayNotHavePermission(CheckPtrState::PermissionStore);
  if (MissingLD || MissingSD)
    reportDerefMissingPerms(Sym, IsLoad, S, CPS, C);

  bool TagCleared = isTainted(State, Loc, TaintTagCapabilityUntagged);
  if (TagCleared)
    reportDerefOfUntaggedCapability(Loc, IsLoad, S, C);

  bool ReadOnly = isTainted(State, Loc, TaintTagCapabilityReadOnly);
  if (ReadOnly) {
    if (!IsLoad)
      reportWriteThroughReadOnlyCap(Loc, S, C);
    else if (Sym->getType()->getPointeeType()->isPointerType()) {
      // Loading a capability through a capability that lacks LM strips LM from
      // the loaded capability as well.
      SVal Loaded = State->getSVal(Loc.castAs<::clang::ento::Loc>());
      State = addTaint(State, Loaded, TaintTagCapabilityReadOnly);
    }
  }

  if (State != OldState)
    C.addTransition(State, T);
  return;
}

ProgramStateRef CheriotHeapChecker::checkPointerEscape(
    ProgramStateRef State, const InvalidatedSymbols &Escaped,
    const CallEvent *Call, PointerEscapeKind Kind) const {
  // FIXME: Unsure why this is needed.
  if (Kind == PointerEscapeKind::PSK_EscapeOnBind)
    return State;
  if (Call) {
    if (SafeFnMap.contains(*Call) || PreFnMap.lookup(*Call) ||
        PostFnMap.lookup(*Call))
      return State;
  }
  for (SymbolRef Sym : Escaped) {
    if (State->get<HeapPointers>(Sym))
      State = State->set<HeapPointers>(Sym, HeapPtrState::Escaped);
  }
  return State;
}

void CheriotHeapChecker::checkBeginFunction(CheckerContext &C) const {
  const StackFrame *SF = C.getStackFrame();
  if (!SF->inTopFrame())
    return;

  // Only start analysis paths at functions
  const Decl *D = C.getStackFrame()->getDecl();
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

    const VarRegion *VR = State->getRegion(Param, SF);
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
  const Decl *D = C.getStackFrame()->getDecl();
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
