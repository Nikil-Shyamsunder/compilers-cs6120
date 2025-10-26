#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace
{

    // Recursively print a loop and its nested loops with their headers
    void findLoopBound(Loop *L, ScalarEvolution &SE)
    {
        // check if there's a canonical induction variable for us to find bounds over
        PHINode *IV = L->getCanonicalInductionVariable();
        if (!IV)
        {
            errs() << "Found a loop! NOT suitable (No Canonical Induction Variable).\n";
            // recurse and return
            for (Loop *SubL : *L)
            {
                findLoopBound(SubL, SE);
            }
            return;
        }
        else
        {
            errs() << "Found a loop with IV!" << *IV << "\n";
        }
        std::optional<Loop::LoopBounds> Bounds = Loop::LoopBounds::getBounds(*L, *IV, SE);
        if (Bounds)
        {
            // start bound
            Value &StartValue = Bounds->getInitialIVValue();
            errs() << "  Initial IV Value (Start): " << StartValue << "\n";

            // end bound
            Value &EndValue = Bounds->getFinalIVValue();
            // This is the limit expression used in the comparison (e.g., N in i < N)
            errs() << "  Final IV Value (Limit): " << EndValue << "\n";

            // step value
            Value *StepValue = Bounds->getStepValue();
            errs() << "  Step Value: " << (StepValue ? *StepValue : *ConstantInt::get(Type::getInt32Ty(SE.getContext()), 1)) << "\n";

            // trip count
            const SCEV *TripCountSCEV = SE.getBackedgeTakenCount(L);
            errs() << "  Loop Range Length (Trip Count): " << *TripCountSCEV << "\n";
        }
        else
        {
            errs() << "Found a loop! NOT SUITABLE (LoopBounds analysis failed).\n";
        }

        // Recurse into sub-loops
        for (Loop *SubL : *L)
        {
            findLoopBound(SubL, SE);
        }
    }

    struct SkeletonPass : public PassInfoMixin<SkeletonPass>
    {
        PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM)
        {
            for (auto &F : M)
            {
                runFunction(F, AM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager());
            }
            return PreservedAnalyses::all();
        };

        // Check if a basic block is effectively empty (only PHI nodes and terminator)
        bool isEmptyBlock(BasicBlock *BB)
        {
            for (Instruction &I : *BB)
            {
                if (!isa<PHINode>(&I) && !I.isTerminator())
                {
                    return false;
                }
            }
            return true;
        }

        // Check if two loops are adjacent (no meaningful code between them)
        bool areLoopsAdjacent(Loop *L1, Loop *L2, LoopInfo &LI)
        {
            // Loops must be at the same nesting level
            if (L1->getLoopDepth() != L2->getLoopDepth())
                return false;

            // Get exit blocks of the first loop
            SmallVector<BasicBlock *, 4> ExitBlocks;
            L1->getExitBlocks(ExitBlocks);

            // For simplicity, handle only single exit block case
            if (ExitBlocks.size() != 1)
                return false;

            BasicBlock *ExitBlock = ExitBlocks[0];

            // Check if the exit block of L1 is the preheader of L2
            BasicBlock *L2Preheader = L2->getLoopPreheader();
            if (!L2Preheader)
                return false;

            // Case 1: Exit block IS the preheader
            if (ExitBlock == L2Preheader)
            {
                return isEmptyBlock(L2Preheader);
            }

            // Case 2: Exit block flows directly to the preheader
            if (ExitBlock->getTerminator()->getNumSuccessors() == 1 &&
                ExitBlock->getTerminator()->getSuccessor(0) == L2Preheader)
            {
                return isEmptyBlock(ExitBlock);
            }

            return false;
        }

        // Find adjacent loops within a loop nest
        void findAdjacentLoopsInNest(Loop *L, LoopInfo &LI, SmallVectorImpl<std::pair<Loop *, Loop *>> &AdjacentPairs)
        {
            // Get all subloops at the current level
            SmallVector<Loop *, 4> SubLoops(L->begin(), L->end());

            // Check pairs of sibling loops
            for (size_t i = 0; i < SubLoops.size(); ++i)
            {
                for (size_t j = i + 1; j < SubLoops.size(); ++j)
                {
                    Loop *L1 = SubLoops[i];
                    Loop *L2 = SubLoops[j];

                    // Check if L1 -> L2 are adjacent
                    if (areLoopsAdjacent(L1, L2, LI))
                    {
                        AdjacentPairs.emplace_back(L1, L2);
                    }
                    // Also check L2 -> L1
                    else if (areLoopsAdjacent(L2, L1, LI))
                    {
                        AdjacentPairs.emplace_back(L2, L1);
                    }
                }
            }

            // Recursively process nested loops
            for (Loop *SubLoop : SubLoops)
            {
                findAdjacentLoopsInNest(SubLoop, LI, AdjacentPairs);
            }
        }

        void runFunction(Function &F, FunctionAnalysisManager &FAM)
        {
            // ignore simple declarations
            if (F.isDeclaration())
                return;

            // Request LoopInfo pass for this function
            LoopInfo &LI = FAM.getResult<LoopAnalysis>(F);
            ScalarEvolution &SE = FAM.getResult<ScalarEvolutionAnalysis>(F);

            SmallVector<std::pair<Loop *, Loop *>, 4> AdjacentPairs;

            // Process each top-level loop and find adjacent pairs
            for (Loop *L : LI)
            {
                findAdjacentLoopsInNest(L, LI, AdjacentPairs);
            }

            // Also check top-level loops for adjacency
            SmallVector<Loop *, 4> TopLevelLoops(LI.begin(), LI.end());
            for (size_t i = 0; i < TopLevelLoops.size(); ++i)
            {
                for (size_t j = i + 1; j < TopLevelLoops.size(); ++j)
                {
                    Loop *L1 = TopLevelLoops[i];
                    Loop *L2 = TopLevelLoops[j];

                    if (areLoopsAdjacent(L1, L2, LI))
                    {
                        AdjacentPairs.emplace_back(L1, L2);
                    }
                    else if (areLoopsAdjacent(L2, L1, LI))
                    {
                        AdjacentPairs.emplace_back(L2, L1);
                    }
                }
            }

            // Print results
            if (!AdjacentPairs.empty())
            {
                errs() << "Found " << AdjacentPairs.size() << " adjacent loop pairs in function "
                       << F.getName() << ":\n";
                for (const auto &Pair : AdjacentPairs)
                {
                    errs() << "  Loop at " << Pair.first->getHeader()->getName()
                           << " -> Loop at " << Pair.second->getHeader()->getName() << "\n";
                }
            }
        };
    };
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo()
{
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "Skeleton pass",
        .PluginVersion = "v0.1",
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB)
        {
            PB.registerOptimizerEarlyEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level, auto)
                {
                    MPM.addPass(SkeletonPass());
                });
        }};
}
