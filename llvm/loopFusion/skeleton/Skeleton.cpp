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
        // Header h = L->getHeader();
        // errs() << "Found a loop!\n";
        // unsigned TripCount = SE.getSmallConstantTripCount(L);
        // Check if the loop has a Canonical Induction Variable (CIV)

        PHINode *IV = L->getCanonicalInductionVariable();
        if (!IV) {
            errs() << "Found a loop! NOT suitable (No Canonical Induction Variable).\n";
            // Recurse and return
            for (Loop *SubL : *L) { findLoopBound(SubL, SE); }
            return; 
        } else {
            errs() << "Found a loop with IV!" << *IV << "\n";
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

        void runFunction(Function &F, FunctionAnalysisManager &FAM)
        {
            // ignore simple declarations
            if (F.isDeclaration())
                return;

            // Request LoopInfo pass for this function
            LoopInfo &LI = FAM.getResult<LoopAnalysis>(F);
            ScalarEvolution &SE = FAM.getResult<ScalarEvolutionAnalysis>(F);

            // errs() << "Function: " << F.getName() << "\n";
            // errs() << "Number of top-level loops: " << LI << "\n";

            // call recursive print functon for each top level loop
            for (Loop *L : LI)
            {
                // errs() << "Loop!\n";
                findLoopBound(L, SE);
            }
        }
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
