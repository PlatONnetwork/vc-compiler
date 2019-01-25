#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
using namespace llvm;

#define DEBUG_TYPE "AddUnrollFull"

STATISTIC(LoopCounter, "Number of loop");

struct AddUnrollFullPass : public LoopPass {
  static char ID; // Pass identification, replacement for typeid
  AddUnrollFullPass() : LoopPass(ID) {}

  bool runOnLoop(Loop *L, LPPassManager &LPM) override {
    LLVMContext &Context = L->getHeader()->getContext();

    MDString* FullString = MDString::get(Context, "llvm.loop.unroll.full");
    MDNode* MD = MDNode::get(Context, ArrayRef<Metadata*>(FullString));

    auto TempNode = MDNode::getTemporary(Context, None);
    Metadata* MDs[] = {TempNode.get(), MD};

    MDNode* LoopID = MDNode::get(Context, MDs);

    LoopID->replaceOperandWith(0, LoopID);

    L->setLoopID(LoopID);

    LoopCounter++;
    return true;
  }
};

char AddUnrollFullPass::ID = 0;

static RegisterPass<AddUnrollFullPass> X("add-unroll-full", "add loop unroll full");

LoopPass* createAddUnrollFullPass (){
  return new AddUnrollFullPass();
}
