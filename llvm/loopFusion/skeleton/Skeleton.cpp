#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace
{

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
                        }
                    }
                }
            }

            return PreservedAnalyses::all();
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
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level)
                {
                    FunctionPassManager FPM;
                    FPM.addPass(LoopSimplifyPass());
                    FPM.addPass(LCSSAPass());
                    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
                    MPM.addPass(SkeletonPass());
                });
        }};
}
