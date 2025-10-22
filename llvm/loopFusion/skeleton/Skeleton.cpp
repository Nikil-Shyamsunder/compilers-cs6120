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
    void printLoopAndSubLoops(Loop *L)
    {
        // Header h = L->getHeader();
        errs() << "Found a loop!\n";

        // Recurse over subloops
        for (Loop *SubLoop : L->getSubLoops())
        {
            printLoopAndSubLoops(SubLoop);
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

            // call recursive print functon for each top level loop
            for (Loop *TLL : LI)
            {
                printLoopAndSubLoops(TLL);
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
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level)
                {
                    MPM.addPass(SkeletonPass());
                });
        }};
}
