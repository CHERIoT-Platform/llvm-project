#include "llvm/CodeGen/CheriBoundAllocas.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Analysis/CheriBounds.h"
#include "llvm/Analysis/Utils/Local.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/Cheri.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/Utils/CheriSetBounds.h"
#include "llvm/Transforms/Utils/Local.h"

#include <string>
#include <utility>
#include <unordered_map>

#include "llvm/IR/Verifier.h"

#define DEBUG_TYPE "cheri-bound-allocas"
using namespace llvm;
#define DBG_MESSAGE(...) LLVM_DEBUG(dbgs() << DEBUG_TYPE ": " << __VA_ARGS__)

namespace {

// Loading/storing from constant stack indices does not need to use a small
// tightly bounded capability and can use $csp instead
// TODO: remove these options once we know what the best stragegy is?
// TODO: change this to an integer threshold (more than N uses -> reuse the same one)
static cl::opt<unsigned> SingleIntrinsicThreshold(
    "cheri-stack-bounds-single-intrinsic-threshold", cl::init(5),
    cl::desc("Reuse the result of a single CHERI bounds intrinsic if there are "
             "more than N uses (default=5). A value of 0 means always."),
    cl::Hidden);

// single option instead of the booleans?
enum class StackBoundsMethod {
  Never,
  ForAllUsesIfOneNeedsBounds, // This is not particularly useful, just for
                              // comparison
  IfNeeded,
};

static cl::opt<StackBoundsMethod> BoundsSettingMode(
    "cheri-stack-bounds",
    cl::desc("Strategy for setting bounds on stack capabilities:"),
    cl::init(StackBoundsMethod::IfNeeded),
    cl::values(clEnumValN(StackBoundsMethod::Never, "never",
                          "Do not add bounds on stack allocations (UNSAFE!)"),
               clEnumValN(StackBoundsMethod::ForAllUsesIfOneNeedsBounds,
                          "all-or-none",
                          "Set stack allocation bounds for all uses if at "
                          "least one use neededs bounds, otherwise omit"),
               clEnumValN(StackBoundsMethod::IfNeeded, "if-needed",
                          "Set stack allocation bounds for all uses except for "
                          "loads/stores to statically known in-bounds offsets")));

enum class StackBoundsAnalysis {
  Default,
  None,
  Simple,
  Full,
};

static cl::opt<StackBoundsAnalysis> BoundsSettingAnalysis(
    "cheri-stack-bounds-analysis",
    cl::desc("Strategy for analysing bounds for stack capabilities:"),
    cl::init(StackBoundsAnalysis::Default),
    cl::values(clEnumValN(StackBoundsAnalysis::Default, "default",
                          "Use the default strategy (simple for "
                          "-O0/optnone, full otherwise)"),
               clEnumValN(StackBoundsAnalysis::None, "none",
                          "Assume all uses require bounds"),
               clEnumValN(StackBoundsAnalysis::Simple, "simple",
                          "Perform a simplified analysis for whether bounds are required"),
               clEnumValN(StackBoundsAnalysis::Full, "full",
                          "Fully analyse whether bounds are required")));

STATISTIC(NumProcessed,  "Number of allocas that were analyzed for CHERI bounds");
STATISTIC(NumDynamicAllocas,  "Number of dyanmic allocas that were analyzed"); // TODO: skip them
STATISTIC(NumUsesProcessed, "Total number of alloca uses that were analyzed");
STATISTIC(NumCompletelyUnboundedAllocas, "Number of allocas where CHERI bounds were completely unncessary");
STATISTIC(NumUsesWithBounds, "Number of alloca uses that had CHERI bounds added");
STATISTIC(NumUsesWithoutBounds, "Number of alloca uses that did not needed CHERI bounds");
STATISTIC(NumSingleIntrin, "Number of times that a single intrinisic was used instead of per-use");

class CheriBoundAllocasImpl {
public:
  StringRef getPassName() const { return "CHERI bound stack allocations"; }
  bool runOnModule(Module &Mod, const TargetMachine &TM) {

    bool Modified = false;
    for (Function &F : Mod)
      Modified |= runOnFunction(F, TM);

    return Modified;
  }

  bool runOnFunction(Function &F, const TargetMachine &TM) {
    const DataLayout &DL = F.getParent()->getDataLayout();
    unsigned AllocaAS = DL.getAllocaAddrSpace();
    // Early abort if we aren't using capabilities on the stack
    if (!DL.isFatPointer(AllocaAS))
      return false;

    // always set bounds with optnone
    bool IsOptNone = F.hasFnAttribute(Attribute::OptimizeNone);
    // FIXME: should still ignore lifetime-start + lifetime-end intrinsics even at -O0
    const TargetLowering *TLI = TM.getSubtargetImpl(F)->getTargetLowering();

    LLVMContext &C = F.getContext();
    llvm::SmallVector<AllocaInst *, 4> Allocas;
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *AI = dyn_cast<AllocaInst>(&I))
          Allocas.push_back(AI);
      }
    }

    // Give up if this function has no allocas
    if (Allocas.empty())
      return false;

    auto *SizeTy = Type::getIntNTy(C, DL.getIndexSizeInBits(AllocaAS));

    LLVM_DEBUG(dbgs() << "\nChecking function " << F.getName() << "\n");

    StackBoundsMethod BoundsMode = BoundsSettingMode;
    StackBoundsAnalysis BoundsAnalysis = BoundsSettingAnalysis;
    if (BoundsAnalysis == StackBoundsAnalysis::Default)
      BoundsAnalysis = IsOptNone ? StackBoundsAnalysis::Simple
                                 : StackBoundsAnalysis::Full;

    // This intrinsic both helps for rematerialising and acts as a marker so
    // isIntrinsicReturningPointerAliasingArgumentWithoutCapturing can safely
    // peek through it for GetUnderlyingObjects in order to not break lifetime
    // markers. Otherwise, if we have:
    //   %0 = alloca
    //   %1 = bitcast ... %0 to i8 addrspace(200)*
    //   %2 = @llvm.lifetime.start.p200i8(..., %1)
    //   ...
    //   unsafe use of %1
    // then we'll have marked the bitcast itself as unsafe and replaced its %0
    // with the bounded capability, and in general having GetUnderlyingObjects
    // return true for bounds-setting intrinsics is not safe.
    //
    // TODO: We should probably be more aggressive at sinking, which might
    // render the above no longer an issue, though likely still fragile, as
    // we'd need to stay in sync with ValueTracking.
    //
    // TODO: csetboundsexact and round up sizes
    Function *BoundedStackFn = Intrinsic::getOrInsertDeclaration(
        F.getParent(), Intrinsic::cheri_bounded_stack_cap, SizeTy);

    IRBuilder<> B(C);

    for (AllocaInst *AI : Allocas) {
      const uint64_t TotalUses = AI->getNumUses();
      NumProcessed++;
      Function *SetBoundsIntrin = BoundedStackFn;

      // Insert immediately after the alloca, but inherit its debug loc rather
      // than the next instruction's which is entirely unrelated
      B.SetInsertPoint(AI->getNextNode());
      B.SetCurrentDebugLocation(AI->getDebugLoc());

      assert(isCheriPointer(AI->getType(), &DL));

      // For imprecise capabilities, we need to increase the alignment for
      // on-stack allocations to ensure that we can create precise bounds.
      // If not a constant then definitely a DYNAMIC_STACKALLOC; alignment
      // requirements will be added later during legalisation.
      std::optional<TypeSize> AllocaSize = AI->getAllocationSize(DL);
      Align ForcedAlignment;
      if (AllocaSize)
        ForcedAlignment = TLI->getAlignmentForPreciseBounds(*AllocaSize);
      if (ForcedAlignment > AI->getAlign())
        AI->setAlignment(ForcedAlignment);

      // Only set bounds for allocas that escape this function
      bool NeedBounds = true;
      // Always set bounds if the function has the optnone attribute
      SmallVector<Use *, 8> UsesThatNeedBounds;
      // If one of the bounded alloca users is a PHI we must reuse the single
      // intrinsic since PHIs must be the first instruction in the basic block
      // and we can't insert anything before. Theoretically we could still
      // use separate intrinsics for the other users but if we are already
      // saving a bounded stack slot we might as well reuse it.
      if (BoundsMode == StackBoundsMethod::Never) {
        NeedBounds = false;
      } else {
        CheriNeedBoundsChecker BoundsChecker(AI, DL);
        // With None we assume bounds are needed on every stack allocation use
        bool BoundAll = BoundsAnalysis == StackBoundsAnalysis::None;
        bool Simple = BoundsAnalysis == StackBoundsAnalysis::Simple;
        BoundsChecker.findUsesThatNeedBounds(&UsesThatNeedBounds, BoundAll,
                                             Simple);
        NeedBounds = !UsesThatNeedBounds.empty();
        NumUsesProcessed += TotalUses;
        DBG_MESSAGE(F.getName()
                        << ": " << UsesThatNeedBounds.size() << " of "
                        << TotalUses << " users need bounds for ";
                    AI->dump());
        // TODO: remove the all-or-nothing case
        if (NeedBounds &&
            BoundsMode == StackBoundsMethod::ForAllUsesIfOneNeedsBounds) {
          // We are compiling with the all-or-nothing case and found at least
          // one use that needs bounds -> set bounds on all uses
          UsesThatNeedBounds.clear();
          LLVM_DEBUG(dbgs() << "Checking if alloca needs bounds: "; AI->dump());

          BoundsChecker.findUsesThatNeedBounds(&UsesThatNeedBounds,
                                               /*BoundAllUses=*/true,
                                               Simple);
        }
      }
      if (!NeedBounds) {
        NumCompletelyUnboundedAllocas++;
        DBG_MESSAGE("No need to set bounds on stack alloca"; AI->dump());
        continue;
      }

      bool MustUseSingleIntrinsic = false;
      if (!AI->isStaticAlloca()) {
        NumDynamicAllocas++;
        // TODO: skip bounds on dynamic allocas (maybe add a TLI hook to check
        // whether the backend already adds bounds to the dynamic_stackalloc)
        DBG_MESSAGE("Found dynamic alloca: must use single intrinisic and "
                    "cheri.bounded.stack.cap.dynamic intrinisic");
        MustUseSingleIntrinsic = true;
        SetBoundsIntrin = Intrinsic::getOrInsertDeclaration(
            F.getParent(), Intrinsic::cheri_bounded_stack_cap_dynamic, SizeTy);
      }

      // Reuse the result of a single csetbounds intrinisic if we are at -O0 or
      // there are more than N users of this bounded stack capability.
      const bool ReuseSingleIntrinsicCall =
          MustUseSingleIntrinsic || IsOptNone ||
          UsesThatNeedBounds.size() >= SingleIntrinsicThreshold;

      NumUsesWithBounds += UsesThatNeedBounds.size();
      NumUsesWithoutBounds += TotalUses - UsesThatNeedBounds.size();

      // Get the size of the alloca
      TypeSize ElementSize = DL.getTypeAllocSize(AI->getAllocatedType());
      Value *Size = ConstantInt::get(SizeTy, ElementSize);
      if (AI->isArrayAllocation()) {
        Value *ArraySize = B.CreateZExtOrTrunc(AI->getArraySize(), SizeTy);
        Size = B.CreateMul(Size, ArraySize);
      }

      if (AI->isStaticAlloca() && ForcedAlignment != Align()) {
        // Pad to ensure bounds don't overlap adjacent objects
        TailPaddingAmount TailPadding =
            TLI->getTailPaddingForPreciseBounds(*AllocaSize);
        if (TailPadding != TailPaddingAmount::None) {
          Type *TypeWithPadding =
              ArrayType::get(Type::getInt8Ty(C),
                             *AllocaSize + static_cast<uint64_t>(TailPadding));
          // Instead of cloning the alloca, mutate it in-place to avoid missing
          // some important metadata (debug info/attributes/etc.).
          AI->setAllocatedType(TypeWithPadding);
          if (AI->getNumOperands() > 0)
            AI->setOperand(0, ConstantInt::get(Type::getInt32Ty(C), 1));
          Size = ConstantInt::get(
              SizeTy, *AllocaSize + static_cast<uint64_t>(TailPadding));
        }
      }

      if (cheri::ShouldCollectCSetBoundsStats) {
        cheri::addSetBoundsStats(AI->getAlign(), Size, getPassName(),
                                 cheri::SetBoundsPointerSource::Stack,
                                 "set bounds on " +
                                     cheri::inferLocalVariableName(AI),
                                 cheri::inferSourceLocation(AI));
      }
      LLVM_DEBUG(auto S = cheri::inferConstantValue(Size);
                 dbgs() << AI->getFunction()->getName()
                        << ": setting bounds on stack alloca to "
                        << (S ? Twine(*S) : Twine("<unknown>"));
                 AI->dump());

      if (ReuseSingleIntrinsicCall) {
        // If we use a single instrinsic for all uses, we can simply update
        // all uses to point at the newly inserted intrinsic.
        NumSingleIntrin++;
        Value *SingleBoundedAlloc = B.CreateCall(SetBoundsIntrin, {AI, Size});
        for (Use *U : UsesThatNeedBounds) {
          U->set(SingleBoundedAlloc);
        }
      } else {
        // Otherwise, we create new intrinsics for every use. This can avoid
        // stack spills but will result in additional instructions.
        // When we encounter multiple uses within the same instruction, we need
        // to ensure that we reuse the same bounded alloca intrinsic, except
        // for PHI uses with different incoming blocks where we must have
        // separate intrinsics in each block. If we don't do this, we could end
        // up creating invalid PHI nodes where the PHI node has multiple
        // entries for the same basic block and uses different incoming values,
        // or uses a value in one block for a different incoming block.
        // (for examples, see multiple-uses-in-same-instr.ll). This not only
        // avoids invalid IR, but also avoids unnecessarily creating multiple
        // intrinsic calls for non-PHIs e.g. in cases where a call instruction
        // passes the same IR variable twice.
        SmallDenseMap<std::pair<User *, BasicBlock *>, Value *> ReplacedUses;
        for (Use *U : UsesThatNeedBounds) {
          Instruction *I = cast<Instruction>(U->getUser());

          BasicBlock *IncomingBB;
          if (auto PHI = dyn_cast<PHINode>(I)) {
            IncomingBB = PHI->getIncomingBlock(*U);
          } else {
            IncomingBB = nullptr;
          }

          Value *BoundedAlloca;
          auto It = ReplacedUses.find({I, IncomingBB});
          if (It == ReplacedUses.end()) {
            // First use in this instruction -> create a new intrinsic call.
            if (IncomingBB) {
              // For PHI nodes we can't insert just before the PHI, instead we
              // must insert it just before the end of the incoming BB.
              // LLVM_DEBUG(dbgs() << "PHI use coming from"; IncomingBB->dump());
              B.SetInsertPoint(IncomingBB->getTerminator());
            } else {
              // Insert just before the use. This should avoid spilling
              // registers when using an alloca in a different basic block.
              B.SetInsertPoint(I);
            }
            // Bounds should have debug loc of the alloca, not the instruction
            // that happens to use them
            B.SetCurrentDebugLocation(AI->getDebugLoc());
            BoundedAlloca = B.CreateCall(SetBoundsIntrin, {AI, Size});
            ReplacedUses.insert({{I, IncomingBB}, BoundedAlloca});
          } else {
            // Multiple uses in the same instruction -> reuse existing call.
            BoundedAlloca = It->second;
          }
          U->set(BoundedAlloca);
        }
      }
    }
    return true;
  }
};

class CheriBoundAllocasLegacy : public FunctionPass {
public:
  static char ID; // Pass identification, replacement for typeid

  CheriBoundAllocasLegacy() : FunctionPass(ID) {
    initializeCheriBoundAllocasLegacyPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &F) override {
    auto *TPC = getAnalysisIfAvailable<TargetPassConfig>();
    if (!TPC)
      return false;
    auto &TM = TPC->getTM<TargetMachine>();
    CheriBoundAllocasImpl CBA;
    return CBA.runOnFunction(F, TM);
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<TargetPassConfig>();
    AU.setPreservesCFG();
  }
};

} // anonymous namespace

FunctionPass *llvm::createCheriBoundAllocasLegacyPass(void) {
  return new CheriBoundAllocasLegacy();
}

char CheriBoundAllocasLegacy::ID = 0;
INITIALIZE_PASS(CheriBoundAllocasLegacy, DEBUG_TYPE,
                "CHERI add bounds to alloca instructions", false, false)

PreservedAnalyses
llvm::CheriBoundAllocasPass::run(Function &F, FunctionAnalysisManager &AM) {
  CheriBoundAllocasImpl CBA;
  CBA.runOnFunction(F, *TM);
  return PreservedAnalyses::allInSet<CFGAnalyses>();
}
