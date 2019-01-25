
#include "llvm/Transforms/Utils/UnifyFunctionExitNodes.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Dominators.h"

using namespace llvm;

#define DEBUG_TYPE "EliminateCFG"

STATISTIC(EliminateCFGCounter, "Counts number of functions");


void ElimCFG(Function& F){
 
  std::vector<BasicBlock*> BBList;

  auto SCCI = scc_begin(&F);

  if(SCCI.hasLoop())
    assert(0);

  for (; !SCCI.isAtEnd(); ++SCCI) {
    for (auto I : *SCCI)
      BBList.push_back(I);
  }

  reverse(BBList.begin(), BBList.end());

  std::vector<Instruction*> InstList;
     
     
  for(BasicBlock* BB : BBList){
    for(Instruction &Inst : *BB){
      if(!isa<BranchInst>(Inst))
        InstList.push_back(&Inst);
    }
  }

  BasicBlock* newBB = BasicBlock::Create(F.getContext(), "", &F);
  IRBuilder<> Builder(newBB);

  for(Instruction* Inst : InstList){
    Inst->removeFromParent();
    Builder.Insert(Inst, Inst->getName());
  }

  for(BasicBlock* BB : BBList){
    BB->eraseFromParent();
  }
}


void RecordReachable(BasicBlock* B, std::map<BasicBlock*, Value*>&BBMap){
  if(BBMap.find(B) == BBMap.end()){
    BBMap[B] = nullptr;
    for(BasicBlock* pred : llvm::predecessors(B))
      RecordReachable(pred, BBMap);
  }
}

Value* InsertSelect(BasicBlock* BB, std::map<BasicBlock*, Value*>&BBMap, PHINode* Phi);

Value* InsertSelectMem(BasicBlock* Parent, BasicBlock* BB, std::map<BasicBlock*, Value*>&BBMap, PHINode* Phi){
  if(BB==Phi->getParent())
    return Phi->getIncomingValueForBlock(Parent);
  else {
    auto iter = BBMap.find(BB);
    if(iter==BBMap.end())return nullptr;
    else if(iter->second != nullptr)return iter->second;
    else {
      Value* result = InsertSelect(BB, BBMap, Phi);
      BBMap[BB] = result;
      return result;
    }
  }
}

Value* InsertSelect(BasicBlock* BB, std::map<BasicBlock*, Value*>&BBMap, PHINode* Phi){
  BranchInst* br = cast<BranchInst>(BB->getTerminator());
  Value* s0 = InsertSelectMem(BB, br->getSuccessor(0), BBMap, Phi);

  if(br->isUnconditional())return s0;
  else{
    Value* s1 = InsertSelectMem(BB, br->getSuccessor(1), BBMap, Phi);
    if(s0==nullptr)return s1;
    else if(s1==nullptr)return s0;
    else if(s0==s1)return s0;
    else {
      StringRef PhiName = Phi->hasName()?Phi->getName():"phi";
      StringRef BBName = BB->hasName()?BB->getName():"";
      return SelectInst::Create(br->getCondition(), s0, s1, PhiName+"."+BBName, Phi);
    }
  }
}

void ElimPhiNode(Function &F, DominatorTree &DT){

  for(BasicBlock &BB : F){

    std::vector<PHINode*> phis;
    for(PHINode &phi : BB.phis())
      phis.push_back(&phi);

    if(phis.size()==0)continue;
 
    BasicBlock* Dom = DT.getNode(&BB)->getIDom()->getBlock();
 
    for(PHINode* phi : phis){

      std::map<BasicBlock*, Value*> BBMap;
      BBMap[Dom] = nullptr;
      RecordReachable(&BB, BBMap);

      
      Value* result = InsertSelect(Dom, BBMap, phi);
   
      phi->replaceAllUsesWith(result);
      phi->eraseFromParent();
    }
  }
}

namespace {
  struct EliminateCFGPass : public FunctionPass {
    static char ID; // Pass identification, replacement for typeid
    EliminateCFGPass() : FunctionPass(ID) {}
    bool runOnFunction(Function &F) override {
      if(F.getBasicBlockList().size()==1)
        return false;

      DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();

      ElimPhiNode(F, DT);

      ElimCFG(F);


      ++EliminateCFGCounter;
      return true;
    }



    void getAnalysisUsage(AnalysisUsage &AU) const override {

      AU.addRequired<DominatorTreeWrapperPass>();
    }
  };
}

char EliminateCFGPass::ID = 0;
static RegisterPass<EliminateCFGPass> X("elim-cfg", "Eliminate CFG");

FunctionPass* createEliminateCFGPass(){
  return new EliminateCFGPass();
}
