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
