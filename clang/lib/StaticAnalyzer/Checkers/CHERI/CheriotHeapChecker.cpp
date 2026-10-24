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
#include "llvm/ADT/STLExtras.h"

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
  // Each permission bit is tracked on a four-point lattice, encoded across the
  // KnownZero and KnownOne masks:
  //
  // clang-format off
  //   KnownZero  KnownOne  Value
  //       0         0      Top      (unconstrained)
  //       1         0      Known(0) (permission removed by a strict check_pointer)
  //       0         1      Known(1) (permission enforced by a check_pointer)
  //       1         1      Bottom   (contradicted)
  // clang-format on
  uint32_t KnownZero = 0;
  uint32_t KnownOne = 0;
  bool CheckStack = false;
  bool ExplicitCheck = false;

  bool isKnownZeroPermission(uint32_t P) const { return (KnownZero & P); }

  bool mayHavePermission(uint32_t P) const { return !(KnownZero & P); }
  bool mayNotHavePermission(uint32_t P) const { return !(KnownOne & P); }

  bool isOverDeterminedPermission(uint32_t P) const {
    return (P & KnownZero & KnownOne);
  }

  // Returns the permission bits that this check requires but a prior check
  // removed, excluding bits that were already Bottom.
  uint32_t getContradictoryPermissions(const CheckPtrState &Prior) const {
    return KnownOne & Prior.KnownZero & ~Prior.KnownOne;
  }

  // Apply this check on top of a prior state:
  // * A strict removal overrides a prior Known(1) to Known(0).
  // * An enforcement over a prior Known(0) yields Bottom.
  // * Bottom and ExplicitCheck sticky.
  CheckPtrState composeWithPriorState(const CheckPtrState &Prior) const {
    CheckPtrState NewState = *this;
    uint32_t PriorBottom = Prior.KnownZero & Prior.KnownOne;
    NewState.KnownZero = KnownZero | Prior.KnownZero;
    NewState.KnownOne = KnownOne | (Prior.KnownOne & ~KnownZero) | PriorBottom;
    NewState.ExplicitCheck = ExplicitCheck || Prior.ExplicitCheck;
    return NewState;
  }

  bool operator==(const CheckPtrState &X) const {
    return KnownZero == X.KnownZero && KnownOne == X.KnownOne &&
           CheckStack == X.CheckStack && ExplicitCheck == X.ExplicitCheck;
  }

  void Profile(llvm::FoldingSetNodeID &ID) const {
    ID.AddInteger(KnownZero);
    ID.AddInteger(KnownOne);
    ID.AddBoolean(CheckStack);
    ID.AddBoolean(ExplicitCheck);
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

  static constexpr uint32_t PermissionsMask = 0xFFF;
};

class CheriotHeapChecker
    : public Checker<check::PreCall, check::PostCall, check::Location,
                     check::Bind, check::BeginFunction, check::EndFunction,
                     check::PointerEscape> {
  const BugType LeakBugType{this, "Heap claim leak", "CHERIoT heap management"};
  const BugType InvalidUseBugType{this, "Invalid pointer use",
                                  "CHERIoT heap management"};
  const BugType CheckPointerMisuseBugType{this, "Inconsistent check_pointer",
                                          "CHERIoT heap management"};
  const BugType LocalCapStoreBugType{
      this, "Locally-scoped capability stored through globally-scoped pointer",
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
      {CDM::SimpleFunc, {"setjmp"}, 1},
      {CDM::SimpleFunc, {"token_obj_unseal"}, 2},
      {CDM::SimpleFunc, {"check_pointer"}, 4},
  };

public:
  void checkPreCall(const CallEvent &Call, CheckerContext &C) const;
  void checkPostCall(const CallEvent &Call, CheckerContext &C) const;
  void checkLocation(SVal Loc, bool IsLoad, const Stmt *S,
                     CheckerContext &C) const;
  void checkBind(SVal Loc, SVal Val, const Stmt *S, bool AtDeclInit,
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

private:
  void reportLeak(const SymbolicRegion *ObjR, ExplodedNode *N,
                  CheckerContext &C) const;
  void reportDerefOfUnclaimedPointer(const SymbolicRegion *ObjR, bool IsLoad,
                                     const Stmt *S, const HeapPtrState *HPS,
                                     ExplodedNode *N, CheckerContext &C) const;
  void reportDerefMissingPerms(SymbolRef Sym, bool IsLoad, const Stmt *S,
                               const CheckPtrState *CPS, ExplodedNode *N,
                               CheckerContext &C) const;
  void reportDerefOfUntaggedCapability(SVal Loc, bool IsLoad, const Stmt *S,
                                       ExplodedNode *N,
                                       CheckerContext &C) const;
  ExplodedNode *reportContradictoryCheckPtr(SymbolRef Sym,
                                            uint32_t Contradictions,
                                            const CallEvent &CE,
                                            CheckerContext &C) const;
};

static constexpr TaintTagType TaintTagCapabilityUntagged = 1;

} // anonymous namespace

// Track whether analysis is currently enabled on this trace
REGISTER_TRAIT_WITH_PROGRAMSTATE(AnalyzingCompartmentEntry, bool)

// Tracker whether we've observed an external state mutation, which
// indicates that the compartment call can no longer safely "just fail".
REGISTER_TRAIT_WITH_PROGRAMSTATE(ExternalStateMutated, bool)

// Claim state, include the where claiming is being tracked at all, is tracked
// per symbolic region.
REGISTER_MAP_WITH_PROGRAMSTATE(HeapClaims, const SymbolicRegion *, HeapPtrState)

// Pointer checking state, which is tracked on the individual pointer.
REGISTER_MAP_WITH_PROGRAMSTATE(CheckedPointers, SymbolRef, CheckPtrState)

// An additional mapping of symbolic regions that are known to be the same as
// each other. This is required to work around an analyzer limitation: after a
// check_pointer, we need to conjure a new pointer SVal to carry the newly
// gained information, but the analyzer does not permit this pointer to point to
// the same symbolic region as the original. This mapping lets us paper over
// that.
REGISTER_MAP_WITH_PROGRAMSTATE(SameObjectAs, const SymbolicRegion *,
                               const SymbolicRegion *)

// Return the symbolic region that a pointer points to, taking into account
// our local SameObjectAs relationship.
static const SymbolicRegion *getObjectRegion(ProgramStateRef State,
                                             SVal PtrVal) {
  const MemRegion *R = PtrVal.getAsRegion();
  if (!R)
    return nullptr;
  const auto *SR = dyn_cast<SymbolicRegion>(R->getBaseRegion());
  if (!SR)
    return nullptr;
  if (const SymbolicRegion *const *Same = State->get<SameObjectAs>(SR))
    return *Same;
  return SR;
}

static bool warningsEnabled(ProgramStateRef State) {
  if (State->get<ExternalStateMutated>())
    return true;

  for (const auto &[ObjR, HPS] : State->get<HeapClaims>())
    if (HPS.isClaimed())
      return true;

  return false;
}

static const SymbolicRegion *getAttributableClaim(ProgramStateRef State) {
  HeapClaimsTy Claims = State->get<HeapClaims>();

  auto It = llvm::min_element(Claims, [](const auto &A, const auto &B) {
    if (A.second.isClaimed() != B.second.isClaimed())
      return A.second.isClaimed();
    return A.first->getSymbol()->getSymbolID() <
           B.first->getSymbol()->getSymbolID();
  });

  if (It == Claims.end() || !It->second.isClaimed())
    return nullptr;

  return It->first;
}

static void explainWarningsEnabled(PathSensitiveBugReport &Report,
                                   CheckerContext &C) {
  ProgramStateRef State = C.getState();
  if (State->get<ExternalStateMutated>())
    return;

  if (const SymbolicRegion *Claim = getAttributableClaim(State))
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
  if (!C.getState()->get<AnalyzingCompartmentEntry>())
    return;
  if (const auto *PreFN = PreFnMap.lookup(Call))
    (*PreFN)(this, Call, C);
}

void CheriotHeapChecker::checkPostCall(const CallEvent &Call,
                                       CheckerContext &C) const {
  ProgramStateRef OldState = C.getState();
  ProgramStateRef State = OldState;
  if (!State->get<AnalyzingCompartmentEntry>())
    return;
  if (const auto *PostFN = PostFnMap.lookup(Call)) {
    (*PostFN)(this, Call, C);
    return;
  }

  // Opaque calls to unrecognized, non-builtin functions may modify
  // state, which should cause us to enable warnings.
  bool GlobalStateMutation = false;
  const FunctionDecl *FD = dyn_cast_or_null<FunctionDecl>(Call.getDecl());
  bool IsBuiltin = FD && FD->getBuiltinID() != 0;
  if (!IsBuiltin && !SafeFnMap.contains(Call)) {
    const Decl *RD = Call.getRuntimeDefinition().getDecl();
    if (!RD || !C.getAnalysisManager().getCFG(RD)) {
      ProgramStateRef WithMutation = State->set<ExternalStateMutated>(true);
      if (WithMutation != State) {
        State = WithMutation;
        GlobalStateMutation = true;
      }
    }
  }

  llvm::SmallVector<const SymbolicRegion *, 2> Invalidated;
  if (isCrossCompartmentCall(Call, C)) {
    // All ephemeral claims are implicitly released at each cross-compartment
    // call.
    for (const auto &[ObjR, HPS] : State->get<HeapClaims>()) {
      if (!HPS.isEphemeral())
        continue;

      State = State->set<HeapClaims>(ObjR, HeapPtrState::InvalidatedEphemeral);
      Invalidated.push_back(ObjR);
    }
  }

  if (UnsealingFns.contains(Call)) {
    // Sealed pointers can't be claimed, but we still need to propagate their
    // state to the unsealed pointer so that we know they should be tracked.
    const auto *Sealed = getObjectRegion(State, Call.getArgSVal(1));
    const auto *Unsealed = getObjectRegion(State, Call.getReturnValue());
    const HeapPtrState *HPS = Sealed ? State->get<HeapClaims>(Sealed) : nullptr;
    if (Unsealed && HPS)
      State = State->set<HeapClaims>(Unsealed, *HPS);
  }

  const NoteTag *T = C.getNoteTag([this, Invalidated = std::move(Invalidated),
                                   GlobalStateMutation](
                                      PathSensitiveBugReport &BR,
                                      llvm::raw_ostream &OS) {
    if (llvm::any_of(Invalidated, [&](const SymbolicRegion *ObjR) {
          return BR.isInteresting(ObjR);
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

static std::string getObjectNameForError(SVal PtrVal, ProgramStateRef State) {
  const auto *SR = getObjectRegion(State, PtrVal);
  if (!SR)
    return "";
  const auto *SymName = dyn_cast<SymbolData>(SR->getSymbol());
  if (!SymName)
    return "";
  const auto *VR = dyn_cast_or_null<VarRegion>(SymName->getOriginRegion());
  if (!VR)
    return "";
  return "'" + VR->getDecl()->getNameAsString() + "' ";
}

static std::string getExprNameForError(const Expr *E) {
  if (!E)
    return "";
  const auto *DRE = dyn_cast<DeclRefExpr>(E->IgnoreParenImpCasts());
  if (!DRE)
    return "";
  const auto *VD = dyn_cast<VarDecl>(DRE->getDecl());
  if (!VD)
    return "";
  return "'" + VD->getNameAsString() + "' ";
}

// Get the deref expression that a checkLocation's statement corresponds to.
static const Expr *getDerefPointerExpr(const Stmt *S) {
  const auto *E = dyn_cast_or_null<Expr>(S);
  if (!E)
    return nullptr;
  return bugreporter::getDerefExpr(E->IgnoreParenLValueCasts());
}

void CheriotHeapChecker::preCheckPointer(const CallEvent &Call,
                                         CheckerContext &C) const {
  ProgramStateRef State = C.getState();
  if (!warningsEnabled(State))
    return;

  // If the pointer argument points into an object that could be heap memory,
  // check that the object is in Claimed state, and report an error if not.
  const auto *ObjR = getObjectRegion(State, Call.getArgSVal(0));
  if (!ObjR)
    return;

  const HeapPtrState *HPS = State->get<HeapClaims>(ObjR);
  if (!HPS || HPS->isEffectivelyClaimed())
    return;

  // Generate an error if the pointer doesn't have a valid claim
  ExplodedNode *N = C.generateErrorNode();
  if (!N)
    return;

  SmallString<200> buf;
  llvm::raw_svector_ostream os(buf);
  os << "check_pointer called on potential heap pointer ";
  os << getExprNameForError(Call.getArgExpr(0));

  if (HPS->isUnclaimed())
    os << "without a valid claim.";
  else if (HPS->isInvalidatedEphemeral())
    os << "after its ephemeral claim was released by a cross-compartment call.";

  auto Report =
      std::make_unique<PathSensitiveBugReport>(InvalidUseBugType, os.str(), N);
  Report->markInteresting(ObjR);
  bugreporter::trackExpressionValue(N, Call.getArgExpr(0), *Report);
  explainWarningsEnabled(*Report, C);
  C.emitReport(std::move(Report));
}

void CheriotHeapChecker::postHeapAddressIsValid(const CallEvent &Call,
                                                CheckerContext &C) const {
  ProgramStateRef State = C.getState();
  const auto *ObjR = getObjectRegion(State, Call.getArgSVal(0));
  if (!ObjR || !State->get<HeapClaims>(ObjR))
    return;

  SVal RetVal = Call.getReturnValue();
  ProgramStateRef StateTrue, StateFalse;
  std::tie(StateTrue, StateFalse) =
      State->assume(RetVal.castAs<DefinedOrUnknownSVal>());

  if (StateTrue)
    C.addTransition(StateTrue);

  if (StateFalse) {
    StateFalse = StateFalse->remove<HeapClaims>(ObjR);
    C.addTransition(StateFalse);
  }
}

void CheriotHeapChecker::postCheckTimeoutPointer(const CallEvent &Call,
                                                 CheckerContext &C) const {
  ProgramStateRef State = C.getState();
  const auto *ObjR = getObjectRegion(State, Call.getArgSVal(0));
  if (!ObjR || !State->get<HeapClaims>(ObjR))
    return;

  SVal RetVal = Call.getReturnValue();
  ProgramStateRef StateTrue, StateFalse;
  std::tie(StateTrue, StateFalse) =
      State->assume(RetVal.castAs<DefinedOrUnknownSVal>());

  if (StateTrue) {
    StateTrue = StateTrue->remove<HeapClaims>(ObjR);
    C.addTransition(StateTrue);
  }

  if (StateFalse)
    C.addTransition(StateFalse);
}

void CheriotHeapChecker::postHeapClaim(const CallEvent &Call,
                                       CheckerContext &C) const {
  const auto *ObjR = getObjectRegion(C.getState(), Call.getArgSVal(1));
  if (!ObjR)
    return;

  // If the allocation capability was also a cross-compartment argument,
  // then it's possible / likely that the caller will free the claim, so
  // we treat it as effectively escaped.
  //
  // This situation arises commonly where the callee is claiming on behalf
  // of the caller, with claim release by the caller as an explicit part
  // of the function contract.
  HeapPtrState::Kind K = HeapPtrState::Claimed;
  const auto *AllocCap = getObjectRegion(C.getState(), Call.getArgSVal(0));
  if (AllocCap && C.getState()->contains<HeapClaims>(AllocCap))
    K = HeapPtrState::Escaped;

  ProgramStateRef State = C.getState()->set<HeapClaims>(ObjR, K);

  // Assume that the claim always succeeds, which also implies that the
  // pointer wasn't null.
  BasicValueFactory &BVF = C.getSValBuilder().getBasicValueFactory();
  QualType RT = Call.getResultType();
  State = State->assumeInclusiveRange(
      Call.getReturnValue().castAs<DefinedOrUnknownSVal>(), BVF.getValue(1, RT),
      BVF.getMaxValue(RT), true);
  if (State)
    State =
        State->assume(Call.getArgSVal(1).castAs<DefinedOrUnknownSVal>(), true);
  if (!State) {
    C.generateSink(C.getState(), C.getPredecessor());
    return;
  }

  std::string Name = getExprNameForError(Call.getArgExpr(1));
  const NoteTag *T =
      C.getNoteTag([ObjR, Name = std::move(Name)](PathSensitiveBugReport &BR,
                                                  llvm::raw_ostream &OS) {
        if (!BR.isInteresting(ObjR))
          return;
        OS << "Claim " << (Name.empty() ? "" : "on " + Name) << "acquired here";
      });

  C.addTransition(State, T);
}

void CheriotHeapChecker::postHeapClaimEphemeral(const CallEvent &Call,
                                                CheckerContext &C) const {
  ProgramStateRef State = C.getState();
  const auto *Obj1 = getObjectRegion(State, Call.getArgSVal(1));
  const auto *Obj2 = getObjectRegion(State, Call.getArgSVal(2));

  // All existing ephemeral claims are released
  for (const auto &[ObjR, HPS] : State->get<HeapClaims>()) {
    if (HPS.isEphemeral())
      State = State->set<HeapClaims>(ObjR, HeapPtrState::Unclaimed);
  }

  if (Obj1)
    State = State->set<HeapClaims>(Obj1, HeapPtrState::Ephemeral);
  if (Obj2)
    State = State->set<HeapClaims>(Obj2, HeapPtrState::Ephemeral);

  // Assume that the claim always succeeds.
  State = State->assume(Call.getReturnValue().castAs<DefinedOrUnknownSVal>(),
                        false);
  if (!State) {
    C.generateSink(C.getState(), C.getPredecessor());
    return;
  }

  std::string Name1 = getExprNameForError(Call.getArgExpr(1));
  std::string Name2 = getExprNameForError(Call.getArgExpr(2));
  const NoteTag *T = C.getNoteTag(
      [Obj1, Obj2, Name1 = std::move(Name1), Name2 = std::move(Name2)](
          PathSensitiveBugReport &BR, llvm::raw_ostream &OS) {
        bool First = Obj1 && BR.isInteresting(Obj1);
        bool Second = Obj2 && BR.isInteresting(Obj2);
        if (!First && !Second)
          return;
        const std::string &Name = First ? Name1 : Name2;
        OS << "Ephemeral claim " << (Name.empty() ? "" : "on " + Name)
           << "acquired here";
      });

  C.addTransition(State, T);
}

void CheriotHeapChecker::postHeapFree(const CallEvent &Call,
                                      CheckerContext &C) const {
  ProgramStateRef State = C.getState();
  const auto *ObjR = getObjectRegion(State, Call.getArgSVal(1));
  if (!ObjR)
    return;

  const HeapPtrState *HPS = State->get<HeapClaims>(ObjR);
  if (!HPS || !HPS->isEffectivelyClaimed())
    return;

  State = State->set<HeapClaims>(ObjR, HeapPtrState::Unclaimed);
  std::string Name = getExprNameForError(Call.getArgExpr(1));
  const NoteTag *T =
      C.getNoteTag([ObjR, Name = std::move(Name)](PathSensitiveBugReport &BR,
                                                  llvm::raw_ostream &OS) {
        if (!BR.isInteresting(ObjR))
          return;
        OS << "Claim " << (Name.empty() ? "" : "on " + Name) << "dropped here";
      });

  C.addTransition(State, T);
}

void CheriotHeapChecker::postHeapFreeAll(const CallEvent &Call,
                                         CheckerContext &C) const {
  ProgramStateRef State = C.getState();

  llvm::SmallVector<const SymbolicRegion *, 2> Invalidated;
  for (const auto &[ObjR, HPS] : State->get<HeapClaims>()) {
    State = State->set<HeapClaims>(ObjR, HeapPtrState::Unclaimed);
    Invalidated.push_back(ObjR);
  }

  const NoteTag *T =
      C.getNoteTag([Invalidated = std::move(Invalidated)](
                       PathSensitiveBugReport &BR, llvm::raw_ostream &OS) {
        if (llvm::none_of(Invalidated, [&](const SymbolicRegion *ObjR) {
              return BR.isInteresting(ObjR);
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

// Print the permissions in Perms as a '|'-separated list, e.g. "LG|SD".
static void printPermissions(llvm::raw_ostream &os, uint32_t Perms) {
  bool PrintBar = false;
  auto RenderPermission = [&](uint32_t Perm, const char *S) {
    if (!(Perms & Perm))
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
}

ExplodedNode *CheriotHeapChecker::reportContradictoryCheckPtr(
    SymbolRef Sym, uint32_t Contradictions, const CallEvent &CE,
    CheckerContext &C) const {
  // Report this as a non-fatal error so that the other permission
  // bits can continue to be analyzed.
  ExplodedNode *N = C.generateNonFatalErrorNode();
  if (!N)
    return nullptr;

  SmallString<200> buf;
  llvm::raw_svector_ostream os(buf);
  os << "check_pointer called multiple times on pointer ";
  os << getExprNameForError(CE.getArgExpr(0));
  os << "with required permissions that were removed by a prior call (";
  printPermissions(os, Contradictions);
  os << ").";

  auto Report = std::make_unique<PathSensitiveBugReport>(
      CheckPointerMisuseBugType, os.str(), N);
  Report->addRange(CE.getSourceRange());
  Report->markInteresting(Sym);
  C.emitReport(std::move(Report));
  return N;
}

void CheriotHeapChecker::postCXXCheckPointer(const CallEvent &Call,
                                             CheckerContext &C) const {
  ProgramStateRef State = C.getState();
  std::optional<Loc> PtrLoc = Call.getArgSVal(0).getAs<Loc>();
  if (!PtrLoc)
    return;
  QualType PtrTy = Call.getArgExpr(0)->getType();
  if (!PtrTy->isAnyPointerType())
    return;
  SVal PtrVal = State->getSVal(*PtrLoc);
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
      .KnownZero = EnforceStrictPermissions
                       ? (~Permissions & CheckPtrState::PermissionsMask)
                       : 0,
      .KnownOne = Permissions,
      .CheckStack = CheckStack,
      .ExplicitCheck = true,
  };

  ExplodedNode *Pred = C.getPredecessor();
  if (const CheckPtrState *CPS = State->get<CheckedPointers>(Sym)) {
    uint32_t Contradictions = NewlyChecked.getContradictoryPermissions(*CPS);
    if (Contradictions)
      if (ExplodedNode *N =
              reportContradictoryCheckPtr(Sym, Contradictions, Call, C))
        Pred = N;
    NewlyChecked = NewlyChecked.composeWithPriorState(*CPS);
  }

  // In order to have separate properties on the pointe post-check_pointer,
  // we have to conjure a new value and copy over some properties.
  SVal Checked =
      C.getSValBuilder().conjureSymbolVal(Call, PtrTy, C.blockCount(), this);
  SymbolRef CheckedSym = Checked.getAsLocSymbol();
  assert(CheckedSym && "a conjured pointer value is always symbolic");

  if (State->isNonNull(PtrVal).isConstrainedTrue())
    if (ProgramStateRef NonNull =
            State->assume(Checked.castAs<DefinedOrUnknownSVal>(), true))
      State = NonNull;
  if (const auto *Obj = getObjectRegion(State, PtrVal))
    State = State->set<SameObjectAs>(
        cast<SymbolicRegion>(Checked.getAsRegion()), Obj);
  State = State->bindLoc(*PtrLoc, Checked, C.getStackFrame());
  State = State->set<CheckedPointers>(CheckedSym, std::move(NewlyChecked));
  C.addTransition(State, Pred);
}

void CheriotHeapChecker::reportDerefOfUnclaimedPointer(
    const SymbolicRegion *ObjR, bool IsLoad, const Stmt *S,
    const HeapPtrState *HPS, ExplodedNode *N, CheckerContext &C) const {
  SmallString<200> buf;
  llvm::raw_svector_ostream os(buf);
  if (IsLoad)
    os << "Read of heap pointer ";
  else
    os << "Store through heap pointer ";
  const Expr *DerefE = getDerefPointerExpr(S);
  os << getExprNameForError(DerefE);
  if (HPS->isUnclaimed())
    os << "without a valid claim.";
  else if (HPS->isInvalidatedEphemeral())
    os << "after its ephemeral claim was released by a cross-compartment "
          "call.";

  auto Report =
      std::make_unique<PathSensitiveBugReport>(InvalidUseBugType, os.str(), N);
  if (S)
    Report->addRange(S->getSourceRange());
  Report->markInteresting(ObjR);
  if (DerefE)
    bugreporter::trackExpressionValue(N, DerefE, *Report);
  explainWarningsEnabled(*Report, C);
  C.emitReport(std::move(Report));
}

void CheriotHeapChecker::reportDerefMissingPerms(SymbolRef Sym, bool IsLoad,
                                                 const Stmt *S,
                                                 const CheckPtrState *CPS,
                                                 ExplodedNode *N,
                                                 CheckerContext &C) const {
  SmallString<200> buf;
  llvm::raw_svector_ostream os(buf);
  if (IsLoad)
    os << "Load through pointer ";
  else
    os << "Store through pointer ";
  const Expr *DerefE = getDerefPointerExpr(S);
  os << getExprNameForError(DerefE);
  os << "with missing ";
  os << (IsLoad ? "LD" : "SD");
  os << " permission.";

  if (CPS->isOverDeterminedPermission(IsLoad ? CheckPtrState::PermissionLoad
                                             : CheckPtrState::PermissionStore))
    os << " Permission value is over-determined and may differ along different "
          "code paths.";

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
  if (DerefE)
    bugreporter::trackExpressionValue(N, DerefE, *Report);
  explainWarningsEnabled(*Report, C);
  C.emitReport(std::move(Report));
}

void CheriotHeapChecker::reportDerefOfUntaggedCapability(
    SVal Loc, bool IsLoad, const Stmt *S, ExplodedNode *N,
    CheckerContext &C) const {
  SmallString<200> buf;
  llvm::raw_svector_ostream os(buf);
  if (IsLoad)
    os << "Load through pointer ";
  else
    os << "Store through pointer ";
  const Expr *DerefE = getDerefPointerExpr(S);
  os << getExprNameForError(DerefE);
  os << "which may be an invalid capability because MC permission was not "
        "checked before the pointer was loaded.";

  auto Report =
      std::make_unique<PathSensitiveBugReport>(InvalidUseBugType, os.str(), N);
  if (S)
    Report->addRange(S->getSourceRange());
  if (DerefE)
    bugreporter::trackExpressionValue(N, DerefE, *Report);
  Report->markInteresting(Loc);
  explainWarningsEnabled(*Report, C);
  C.emitReport(std::move(Report));
}

// A capability loaded through a pointer known to lack LM or LG removes
// permissions from the loaded pointer.
static ProgramStateRef stripLoadedCapPermissions(ProgramStateRef State,
                                                 SymbolRef Sym,
                                                 const CheckPtrState &CPS,
                                                 SVal Loc, CheckerContext &C,
                                                 const NoteTag *&T) {
  bool MissingLM =
      CPS.isKnownZeroPermission(CheckPtrState::PermissionLoadMutable);
  bool MissingLG =
      CPS.isKnownZeroPermission(CheckPtrState::PermissionLoadGlobal);

  uint32_t Removed = 0;

  // Missing LM removes LM and SD from the loaded cap.
  if (MissingLM) {
    Removed |=
        CheckPtrState::PermissionLoadMutable | CheckPtrState::PermissionStore;
  }

  // Missing LG removes LG and GL from the loaded cap.
  if (MissingLG) {
    Removed |=
        CheckPtrState::PermissionLoadGlobal | CheckPtrState::PermissionGlobal;
  }

  if (!Removed)
    return State;

  SVal Loaded = State->getSVal(Loc.castAs<::clang::ento::Loc>());
  SymbolRef LoadedSym = Loaded.getAsLocSymbol();
  if (!LoadedSym)
    return State;

  CheckPtrState LoadedCheck = {.KnownZero = Removed, .KnownOne = 0};
  if (const CheckPtrState *Prior = State->get<CheckedPointers>(LoadedSym)) {
    // This is not a real check_pointer call, so keep the prior CheckStack.
    LoadedCheck.CheckStack = Prior->CheckStack;
    // Keep track of which bits actually changed.
    Removed &= ~Prior->KnownZero;
    LoadedCheck = LoadedCheck.composeWithPriorState(*Prior);
  }
  State = State->set<CheckedPointers>(LoadedSym, LoadedCheck);

  if (!Removed)
    return State;

  uint32_t Causes = 0;

  // Report the cause of permission removal for any permissions that were
  // actually removed.
  if (Removed &
      (CheckPtrState::PermissionLoadMutable | CheckPtrState::PermissionStore))
    Causes |= CheckPtrState::PermissionLoadMutable;
  if (Removed &
      (CheckPtrState::PermissionLoadGlobal | CheckPtrState::PermissionGlobal))
    Causes |= CheckPtrState::PermissionLoadGlobal;

  std::string SymName = getObjectNameForError(Loc, State);
  T = C.getNoteTag([LoadedSym, Sym, Removed, Causes,
                    SymName = std::move(SymName)](PathSensitiveBugReport &BR,
                                                  llvm::raw_ostream &OS) {
    if (!BR.isInteresting(LoadedSym))
      return;
    BR.markInteresting(Sym);

    OS << "Loaded pointer has permissions ";
    printPermissions(OS, Removed);
    OS << " removed because the pointer " << SymName
       << "it was loaded through lacks ";
    printPermissions(OS, Causes);
  });
  return State;
}

void CheriotHeapChecker::checkLocation(SVal Loc, bool IsLoad, const Stmt *S,
                                       CheckerContext &C) const {
  ProgramStateRef State = C.getState();
  if (!State->get<AnalyzingCompartmentEntry>())
    return;
  ProgramStateRef OldState = State;
  SymbolRef Sym = Loc.getLocSymbolInBase();
  const MemRegion *Region = Loc.getAsRegion();

  // If this is a write to non-stack memory that is not one of the
  // tracked compartment call arguments, then it is an internal state
  // change that could cause state desynchronization on compartment
  // crash.
  bool CompartmentCrashWarningsLive = warningsEnabled(State);
  const auto *ObjR = getObjectRegion(State, Loc);
  const HeapPtrState *HPS = ObjR ? State->get<HeapClaims>(ObjR) : nullptr;
  if (!IsLoad && !HPS) {
    if (!Region || !isa<StackSpaceRegion>(Region->getMemorySpace(State)))
      State = State->set<ExternalStateMutated>(true);
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

  bool LoadsCap =
      IsLoad && Sym->getType()->getPointeeType()->isPointerType() && CPS;
  if (LoadsCap)
    State = stripLoadedCapPermissions(State, Sym, *CPS, Loc, C, T);

  bool MissingMC =
      IsLoad && Sym->getType()->getPointeeType()->isPointerType() && CPS &&
      ((CPS->ExplicitCheck &&
        CPS->mayNotHavePermission(CheckPtrState::PermissionLoadStoreCap)) ||
       CPS->isKnownZeroPermission(CheckPtrState::PermissionLoadStoreCap));
  if (MissingMC) {
    SVal Loaded = State->getSVal(Loc.castAs<::clang::ento::Loc>());
    State = addTaint(State, Loaded, TaintTagCapabilityUntagged);
  }

  if (!CompartmentCrashWarningsLive) {
    // If we see a write through a tracked heap pointer at a time when we're not
    // reporting compartment crashes, then we take that as an assertion that the
    // pointer is actually claimed.
    if (HPS)
      State = State->set<HeapClaims>(ObjR, HeapPtrState::Escaped);
    if (State != OldState)
      C.addTransition(State, T);
    return;
  }

  bool Unclaimed = HPS && !HPS->isEffectivelyClaimed();

  bool MissingPerms = false;
  if (CPS) {
    bool MissingLD =
        (CPS->isKnownZeroPermission(CheckPtrState::PermissionLoad) ||
         (CPS->ExplicitCheck &&
          CPS->mayNotHavePermission(CheckPtrState::PermissionLoad))) &&
        IsLoad;
    bool MissingSD =
        (CPS->isKnownZeroPermission(CheckPtrState::PermissionStore) ||
         (CPS->ExplicitCheck &&
          CPS->mayNotHavePermission(CheckPtrState::PermissionStore))) &&
        !IsLoad;
    MissingPerms = MissingLD || MissingSD;
  }

  bool TagCleared = isTainted(State, Loc, TaintTagCapabilityUntagged);

  if (Unclaimed || MissingPerms || TagCleared) {
    // Report all errors against the same analysis node so that we don't
    // drop errors if there are more than one.
    ExplodedNode *N = C.generateErrorNode();
    if (!N)
      return;
    if (Unclaimed)
      reportDerefOfUnclaimedPointer(ObjR, IsLoad, S, HPS, N, C);
    if (MissingPerms)
      reportDerefMissingPerms(Sym, IsLoad, S, CPS, N, C);
    if (TagCleared)
      reportDerefOfUntaggedCapability(Loc, IsLoad, S, N, C);
    return;
  }

  if (State != OldState)
    C.addTransition(State, T);
  return;
}

void CheriotHeapChecker::checkBind(SVal Loc, SVal Val, const Stmt *S,
                                   bool AtDeclInit, CheckerContext &C) const {
  ProgramStateRef State = C.getState();
  if (!State->get<AnalyzingCompartmentEntry>())
    return;
  SymbolRef DestSym = Loc.getLocSymbolInBase();
  SymbolRef ValSym = Val.getAsLocSymbol(/*IncludeBaseRegions=*/true);
  if (!DestSym || !ValSym)
    return;

  // Local capabilities can't be stored through global pointers.
  // The former is indicated by !GL on the stored value, the latter by
  // !SL on the pointer.
  const CheckPtrState *DestCPS = State->get<CheckedPointers>(DestSym);
  const CheckPtrState *ValCPS = State->get<CheckedPointers>(ValSym);
  if (!DestCPS || !ValCPS ||
      !DestCPS->isKnownZeroPermission(CheckPtrState::PermissionStoreLocal) ||
      !ValCPS->isKnownZeroPermission(CheckPtrState::PermissionGlobal))
    return;

  ExplodedNode *N = C.generateNonFatalErrorNode();
  if (!N)
    return;

  SmallString<200> buf;
  llvm::raw_svector_ostream os(buf);
  os << "Store of locally-scoped capability ";
  os << getObjectNameForError(Val, State);
  os << "(lacking GL permission) through a globally-scoped pointer ";
  os << getObjectNameForError(Loc, State);
  os << "(lacking SL permission). The stored capability will be untagged.";

  auto Report = std::make_unique<PathSensitiveBugReport>(LocalCapStoreBugType,
                                                         os.str(), N);
  if (S)
    Report->addRange(S->getSourceRange());
  Report->markInteresting(DestSym);
  Report->markInteresting(ValSym);
  C.emitReport(std::move(Report));
}

ProgramStateRef CheriotHeapChecker::checkPointerEscape(
    ProgramStateRef State, const InvalidatedSymbols &Escaped,
    const CallEvent *Call, PointerEscapeKind Kind) const {
  if (!State->get<AnalyzingCompartmentEntry>())
    return State;
  if (Call) {
    if (SafeFnMap.contains(*Call) || PreFnMap.lookup(*Call) ||
        PostFnMap.lookup(*Call))
      return State;
  }

  for (const auto &[ObjR, HPS] : State->get<HeapClaims>()) {
    if (Escaped.count(ObjR->getSymbol())) {
      State = State->set<HeapClaims>(ObjR, HeapPtrState::Escaped);
    }
  }
  for (const auto &[R, ObjR] : State->get<SameObjectAs>()) {
    if (Escaped.count(R->getSymbol()) && State->contains<HeapClaims>(ObjR)) {
      State = State->set<HeapClaims>(ObjR, HeapPtrState::Escaped);
    }
  }

  return State;
}

void CheriotHeapChecker::checkBeginFunction(CheckerContext &C) const {
  const StackFrame *SF = C.getStackFrame();
  if (!SF->inTopFrame())
    return;

  // Only analyze paths starting at compartment entry points.
  const Decl *D = C.getStackFrame()->getDecl();
  const FunctionDecl *FD = dyn_cast_or_null<FunctionDecl>(D);
  if (!FD || !FD->hasAttr<CHERICompartmentNameAttr>())
    return;

  // Mark all pointer arguments as initially unclaimed
  ProgramStateRef State = C.getState();
  bool Modified = false;
  for (unsigned i = 0; i < FD->getNumParams(); ++i) {
    const ParmVarDecl *Param = FD->getParamDecl(i);
    if (!Param->getType()->isPointerType())
      continue;

    const VarRegion *VR = State->getRegion(Param, SF);
    if (const auto *ObjR = getObjectRegion(State, State->getSVal(VR))) {
      State = State->set<HeapClaims>(ObjR, HeapPtrState::Unclaimed);
      Modified = true;
    }
  }

  // Only analyze entry points with at least one pointer argument.
  if (!Modified)
    return;

  State = State->set<AnalyzingCompartmentEntry>(true);
  C.addTransition(State);
}

void CheriotHeapChecker::checkEndFunction(const ReturnStmt *RS,
                                          CheckerContext &C) const {
  ProgramStateRef State = C.getState();
  if (!State->get<AnalyzingCompartmentEntry>())
    return;

  // Only report leaks on returning from the compartment entry point the
  // analysis started at.
  if (!C.inTopFrame())
    return;

  // Report every leaked claim on a single error node
  ExplodedNode *N = nullptr;
  for (const auto &[ObjR, HPS] : State->get<HeapClaims>()) {
    if (!HPS.isClaimed())
      continue;
    if (!N) {
      N = C.generateErrorNode();
      if (!N)
        return;
    }
    reportLeak(ObjR, N, C);
  }
}

void CheriotHeapChecker::reportLeak(const SymbolicRegion *ObjR, ExplodedNode *N,
                                    CheckerContext &C) const {
  SmallString<200> buf;
  llvm::raw_svector_ostream os(buf);
  os << "Claim on pointer ";
  os << getObjectNameForError(loc::MemRegionVal(ObjR), C.getState());
  os << "must be released with heap_free or heap_free_all before returning "
        "from a compartment call.";

  auto Report =
      std::make_unique<PathSensitiveBugReport>(LeakBugType, os.str(), N);
  Report->markInteresting(ObjR);
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
