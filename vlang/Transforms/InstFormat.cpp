#include<vector>

#include "llvm/ADT/Statistic.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"

using namespace llvm;
using namespace std;

#define DEBUG_TYPE "InstFormat"

STATISTIC(InstFormatCounter, "Counts number of Attribute");

struct InstFormatPass : public llvm::BasicBlockPass {
  static char ID; // Pass identification, replacement for typeid
  InstFormatPass() : BasicBlockPass(ID) {}
  bool runOnBasicBlock(llvm::BasicBlock &) override;
};

BinaryOperator* ReplaceBinaryOperator(BinaryOperator* BO){
  LLVMContext &Context = BO->getContext();
  Type* i32 = Type::getInt32Ty(Context);
  Constant* int2 = ConstantInt::get(i32, 2);

  switch(BO->getOpcode()){
    case BinaryOperator::Shl:
      return BinaryOperator::CreateMul(BO->getOperand(0), int2, BO->getName());
    case BinaryOperator::AShr:
      return BinaryOperator::CreateSDiv(BO->getOperand(0), int2, BO->getName());
    case BinaryOperator::LShr:
      return BinaryOperator::CreateUDiv(BO->getOperand(0), int2, BO->getName());
    default:
      return nullptr;
  }
}

ICmpInst* ReplaceICmpInst(ICmpInst* ICmp){
  switch(ICmp->getPredicate()){
    case ICmpInst::ICMP_SLT:
      return new ICmpInst(ICmpInst::ICMP_SGT, ICmp->getOperand(1), ICmp->getOperand(0));
    case ICmpInst::ICMP_SLE:
      return new ICmpInst(ICmpInst::ICMP_SGE, ICmp->getOperand(1), ICmp->getOperand(0));
    case ICmpInst::ICMP_ULT:
      return new ICmpInst(ICmpInst::ICMP_UGT, ICmp->getOperand(1), ICmp->getOperand(0));
    case ICmpInst::ICMP_ULE:
      return new ICmpInst(ICmpInst::ICMP_UGE, ICmp->getOperand(1), ICmp->getOperand(0));
    default:
      return nullptr;
  }
}

void FillReturnValue(BasicBlock &BB){
  ReturnInst* RetInst = cast<ReturnInst>(BB.getTerminator());
  Value* RetValue = RetInst->getReturnValue();
  Function* F = BB.getParent();

  bool RetArg = false;
  for(Argument &A : F->args()){
    if(&A==RetValue) RetArg = true;
  }

  if(RetArg){
    BinaryOperator* Add = BinaryOperator::CreateAdd(RetValue, ConstantInt::getNullValue(RetValue->getType()));
    Add->insertBefore(RetInst);
    ReturnInst* newRetInst = ReturnInst::Create(BB.getContext(), Add, RetInst);
    
    RetInst->replaceAllUsesWith(newRetInst);
    RetInst->eraseFromParent();
  }
}

bool InstFormatPass::runOnBasicBlock(BasicBlock &BB) {

  vector<pair<Instruction*, Instruction*>> InstPairs;

  for(Instruction &Inst : BB){
    Instruction* Alter = nullptr;
    if(BinaryOperator* BO = dyn_cast<BinaryOperator>(&Inst))
      Alter = ReplaceBinaryOperator(BO);
    else if(ICmpInst* ICmp = dyn_cast<ICmpInst>(&Inst))
      Alter = ReplaceICmpInst(ICmp);
    else 
      Alter = nullptr;

    if(Alter)
      InstPairs.push_back(make_pair(&Inst, Alter));
  }
 
  for(auto iter : InstPairs){
    Instruction* Inst = iter.first;
    Instruction* Alter = iter.second;

    Alter->insertAfter(Inst);
    Inst->replaceAllUsesWith(Alter);
    Inst->eraseFromParent();
  } 

  FillReturnValue(BB);
  
  InstFormatCounter += InstPairs.size();
  return true;
}


char InstFormatPass::ID = 0;

static RegisterPass<InstFormatPass> X("inst-format", "Instruction Format");

BasicBlockPass* createInstFormatPass (){
  return new InstFormatPass();
}
