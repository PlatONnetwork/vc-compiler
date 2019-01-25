#include <vector>

#include "llvm/ADT/Statistic.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Function.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/IR/IRBuilder.h"


using namespace llvm;
using namespace std;

#define DEBUG_TYPE "VerifyUnroll"

StringRef getSimpleName(StringRef SymbolName);

bool VerifyGlobalVariable(Module &M){
  for(GlobalVariable &GV : M.globals()){
    llvm::errs() << "Error: Inline value failure\n";
    GV.print(llvm::outs());
    return true;
  }
  return false;
}

bool VerifyFunctions(Module &M){
  for(Function &F : M.functions()){
    if(getSimpleName(F.getName())!="vc_main"){
     llvm::errs() << "Error: Inline function '@" << F.getName() << "' failure\n";
     return true;
    }
  }
  return false;
}

bool VerifyMains(Module &M){
  unsigned n = M.getFunctionList().size();

  if(n==0){
    llvm::errs() << "Error: vc_main function not found\n";
    return true;
  } else if(n>1){
    llvm::errs() << "Error: vc_main function more than one:\n";
    for(Function &F : M.functions())
      llvm::errs() << F.getName() << '\n';
    return true;
  } else {
    return false;
  }
}

bool VerifyReturnType(Function &Main){
  if(Main.getReturnType()->isIntegerTy())
    return false;
  else {
    llvm::errs() << "Error: vc_main return type is not integer type\n";
    return true;
  }
}

bool VerifyParamType(Function &Main){
  for(Argument &A : Main.args()){
    if(!A.getType()->isIntegerTy()){
      llvm::errs() << "Error: vc_main param type is not integer type\n";
      return true;
    }
  }
  return false;
}

bool VerifyBasicBlock(BasicBlock &Entry){
  if(isa<ReturnInst>(Entry.getTerminator()))
    return false;
  else {
    llvm::errs() << "Error: Control Flow Eliminate Failure\n";
    return true;
  }
}

bool VerifyInsts(BasicBlock &Entry){
  for(Instruction &Inst : Entry) {
    for(Use &Op : Inst.operands()){
      if(!Op->getType()->isIntegerTy()){
        llvm::errs() << "Error: Operand is not integer Type\n";
        Inst.print(llvm::outs());
        return true;
      }
    }
    bool isLegal = isa<BinaryOperator>(Inst) || isa<ICmpInst>(Inst) || isa<SelectInst>(Inst)
                 || isa<TruncInst>(Inst) || isa<ZExtInst>(Inst) || isa<SExtInst>(Inst)
                 || isa<ReturnInst>(Inst);
             
    if(!isLegal){
      llvm::errs() << "Error: Illegal Instruction\n";
      Inst.print(llvm::outs());
      return true;
    }
  }
  return false;
}

bool VerifyModule(Module &M){

  if(VerifyGlobalVariable(M) || VerifyFunctions(M) || VerifyMains(M))
    return true;

  Function &Main = M.getFunctionList().front();

  if(VerifyReturnType(Main) || VerifyParamType(Main))
    return true;

  BasicBlock &BB = Main.getEntryBlock();

  return VerifyBasicBlock(BB) || VerifyInsts(BB);
}

struct VerifyUnrollPass : public llvm::ModulePass {
  static char ID; // Pass identification, replacement for typeid
  VerifyUnrollPass() : ModulePass(ID) {}
  bool runOnModule(llvm::Module &M) override {

    if(VerifyModule(M))
      report_fatal_error("Broken module found, compilation aborted!");

    return true;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
  }

};

char VerifyUnrollPass::ID = 0;

static RegisterPass<VerifyUnrollPass> X("verify-unroll", "Verify Unroll");

ModulePass* createVerifyUnrollPass (){
  return new VerifyUnrollPass();
}
