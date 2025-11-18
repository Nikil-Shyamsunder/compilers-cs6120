#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SmallVector.h"

using namespace llvm;

namespace {

struct SkeletonPass : public PassInfoMixin<SkeletonPass> {

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
        errs() << "SkeletonPass running on function: " << F.getName() << "\n";

        // Skip instrumenting the counter function itself to avoid infinite recursion
        if (F.getName() == "incrementCounter") {
            return PreservedAnalyses::all();
        }

        LLVMContext &C = F.getContext();
        Module *M = F.getParent();

        // Declare/lookup: void incrementCounter(unsigned)
        FunctionCallee CountFn = M->getOrInsertFunction(
            "incrementCounter",
            Type::getVoidTy(C),
            Type::getInt32Ty(C)
        );

        bool modified = false;

        // Collect instructions first to avoid iterator invalidation
        SmallVector<Instruction*> Instructions;
        for (BasicBlock &BB : F) {
            for (Instruction &I : BB) {
                Instructions.push_back(&I);
            }
        }

        // Now insert calls before each instruction
        for (Instruction *I : Instructions) {
            // Skip PHI nodes - they must be at the start of basic blocks
            if (isa<PHINode>(I)) {
                continue;
            }

            // Insert call before this instruction
            IRBuilder<> Builder(I);
            Builder.SetInsertPoint(I);
            Value *opcodeVal = Builder.getInt32(I->getOpcode());
            Builder.CreateCall(CountFn, {opcodeVal});
            modified = true;
        }

        errs() << "SkeletonPass modified: " << modified << "\n";
        return modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }
};

} // end anonymous namespace


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
                    MPM.addPass(createModuleToFunctionPassAdaptor(SkeletonPass()));
                });
        }};
}