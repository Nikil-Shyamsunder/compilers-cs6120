#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

namespace
{

    struct SkeletonPass : public PassInfoMixin<SkeletonPass>
    {
        // PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
        //     for (auto &F : M) {
        //         errs() << "I saw a function called " << F.getName() << "!\n";
        //     }
        //     return PreservedAnalyses::all();
        // };
        PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM)
        {
            for (auto &F : M)
            {
                for (auto &B : F)
                {
                    for (auto &I : B)
                    {
                        // errs() << "I saw an instruction: " << I << "\n";
                        if (auto *op = dyn_cast<BinaryOperator>(&I))
                        {
                            if (!(op->getOpcode() == Instruction::Sub || op->getOpcode() == Instruction::FSub)){
                                continue;
                            }

                            // Insert at the point where the instruction `op` appears.
                            IRBuilder<> builder(op);

                            // Make a multiply with the same operands as `op`.
                            Value *lhs = op->getOperand(0);
                            Value *rhs = op->getOperand(1);
                            Value *swappedSub = builder.CreateSub(rhs, lhs);

                            errs() << "Replaced a sub with a swapped sub!\n";

                            // Everywhere the old instruction was used as an operand, use our
                            // new multiply instruction instead.
                            for (auto &U : op->uses())
                            {
                                User *user = U.getUser(); // A User is anything with operands.
                                user->setOperand(U.getOperandNo(), swappedSub);
                            }

                            // We modified the code.
                            // return PreservedAnalyses::none();
                        }
                    }
                }
            };
            return PreservedAnalyses::none();
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
