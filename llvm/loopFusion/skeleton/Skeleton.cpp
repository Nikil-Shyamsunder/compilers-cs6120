#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Transforms/Scalar/IndVarSimplify.h"
#include "llvm/Transforms/Utils/LoopSimplify.h"
#include "llvm/Transforms/Utils/LCSSA.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace
{

    const SCEV *getTripCount(Loop *L, ScalarEvolution &SE)
    {
        return SE.getBackedgeTakenCount(L);
    }

    void printTripCount(const SCEV *TripCount)
    {
        errs() << *TripCount << "\n";
    }

    static bool hasDirectEdge(BasicBlock *From, BasicBlock *To)
    {
        for (BasicBlock *Succ : successors(From))
            if (Succ == To)
                return true;
        return false;
    }

    static bool blockHasNoSideEffectsExceptTerminator(BasicBlock *BB)
    {
        for (Instruction &I : *BB)
        {
            if (I.isTerminator())
                continue;

            if (isa<PHINode>(I) || isa<ICmpInst>(I) || isa<FCmpInst>(I))
                continue;

            // Allow lifetime intrinsics (llvm.lifetime.start/end)
            if (auto *II = dyn_cast<IntrinsicInst>(&I))
            {
                if (II->getIntrinsicID() == Intrinsic::lifetime_start ||
                    II->getIntrinsicID() == Intrinsic::lifetime_end)
                    continue;
            }

            if (I.mayHaveSideEffects() || I.mayReadOrWriteMemory())
                return false;
        }
        return true;
    }

    // simple fusion: merge L2's body into L1
    bool fuseLoops(Loop *L1, Loop *L2, LoopInfo &LI)
    {
        errs() << "  Attempting to fuse loops...\n";

        BasicBlock *L1Header = L1->getHeader();
        BasicBlock *L2Header = L2->getHeader();

        // Get the latch (backedge block) of L1
        BasicBlock *L1Latch = L1->getLoopLatch();
        if (!L1Latch)
        {
            errs() << "  Cannot fuse: L1 has no unique latch\n";
            return false;
        }

        SmallVector<BasicBlock *, 8> L2Blocks(L2->blocks().begin(), L2->blocks().end());
        ValueToValueMapTy VMap;

        PHINode *L1IndVar = nullptr;
        PHINode *L2IndVar = nullptr;

        for (PHINode &PHI : L1Header->phis())
        {
            L1IndVar = &PHI;
            break;
        }

        for (PHINode &PHI : L2Header->phis())
        {
            L2IndVar = &PHI;
            break;
        }

        // map L2's induction variable to L1's - they have the same iteration space
        if (L1IndVar && L2IndVar)
        {
            VMap[L2IndVar] = L1IndVar;
        }

        // clone L2's body instructions into L1, right before L1's backedge/latch
        for (BasicBlock *BB : L2Blocks)
        {
            for (Instruction &I : *BB)
            {
                // skip phi nodes + terminators
                if (isa<PHINode>(I))
                    continue;
                if (I.isTerminator())
                    continue;

                // clone the instruction
                Instruction *ClonedInst = I.clone();
                ClonedInst->insertBefore(L1Latch->getTerminator()->getIterator());

                // remap operands to use L1's values
                RemapInstruction(ClonedInst, VMap, RF_IgnoreMissingLocals | RF_NoModuleLevelChanges);

                // add to value map for future remapping
                VMap[&I] = ClonedInst;
            }
        }

        errs() << "  Successfully fused loops (instructions merged)!\n";
        errs() << "  Note: L2 still exists but its work is now done in L1\n";
        return true;
    }

    struct SkeletonPass : public PassInfoMixin<SkeletonPass>
    {
        PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM)
        {
            errs() << "=== Loop Fusion Candidate Detector (SkeletonPass) ===\n";

            auto &FAM =
                AM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

            for (Function &F : M)
            {
                if (F.isDeclaration())
                    continue;
                errs() << "Analyzing function: " << F.getName() << "\n";

                LoopInfo &LI = FAM.getResult<LoopAnalysis>(F);
                ScalarEvolution &SE = FAM.getResult<ScalarEvolutionAnalysis>(F);

                SmallVector<Loop *, 8> Top;
                for (Loop *L : LI)
                    Top.push_back(L);

                errs() << "  Found " << Top.size() << " top-level loops\n";

                if (Top.size() < 2)
                    continue;

                // scan all pairs of top-level loops in both orders
                for (size_t i = 0; i < Top.size(); ++i)
                {
                    for (size_t j = 0; j < Top.size(); ++j)
                    {
                        if (i == j)
                            continue;

                        Loop *L1 = Top[i];
                        Loop *L2 = Top[j];

                        BasicBlock *Hdr1 = L1->getHeader();
                        BasicBlock *Hdr2 = L2->getHeader();
                        BasicBlock *Pre2 = L2->getLoopPreheader();

                        if (!Pre2)
                            continue;

                        // loop adjacency test: L1's exit should lead to L2's entry (preheader)
                        BasicBlock *L1Exit = L1->getUniqueExitBlock();

                        // if there's a unique exit block, check if it connects to L2's preheader
                        bool candidate = false;
                        if (L1Exit && L1Exit == Pre2)
                        {
                            candidate = true;
                        }
                        else if (L1Exit && hasDirectEdge(L1Exit, Pre2))
                        {
                            // L1's exit has a direct edge to L2's preheader
                            if (blockHasNoSideEffectsExceptTerminator(L1Exit))
                            {
                                candidate = true;
                            }
                        }

                        if (candidate)
                        {
                            errs() << "  Possible fusion candidate in " << F.getName() << ":\n";
                            errs() << "    Loop 1 header: " << Hdr1->getName() << "\n";
                            errs() << "    Loop 2 header: " << Hdr2->getName() << "\n";

                            const SCEV *TC1 = getTripCount(L1, SE);
                            const SCEV *TC2 = getTripCount(L2, SE);

                            errs() << "    Loop 1 trip count: ";
                            printTripCount(TC1);
                            errs() << "    Loop 2 trip count: ";
                            printTripCount(TC2);

                            if (TC1 == TC2)
                            {
                                errs() << "  Trip counts are equal! Performing fusion...\n";
                                fuseLoops(L1, L2, LI);
                            }
                            else
                            {
                                errs() << "  Trip counts differ, skipping fusion\n";
                            }
                            errs() << "\n";
                        }
                    }
                }
            }

            return PreservedAnalyses::none();
        }
    };

}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo()
{
    return {
        LLVM_PLUGIN_API_VERSION,
        "Skeleton pass",
        "v0.1",
        [](PassBuilder &PB)
        {
            PB.registerOptimizerLastEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level, ThinOrFullLTOPhase)
                {
                    FunctionPassManager FPM;
                    FPM.addPass(LoopSimplifyPass());
                    FPM.addPass(LCSSAPass());
                    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
                    MPM.addPass(SkeletonPass());
                });
        }};
}
