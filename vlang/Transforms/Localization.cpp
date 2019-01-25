#include "llvm/ADT/Statistic.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

#include "DepandentGraph.h"

using namespace llvm;

#define DEBUG_TYPE "Localization"

STATISTIC(GlobalVariableCounter, "Counts number of GlobalVariable");

static void LocalizationVariable(GlobalVariable* GV){
  std::map<Function*, AllocaInst*> AllocaMap;
  std::vector<User*> Users(GV->user_begin(), GV->user_end());
  for(User* U : Users){

    Instruction* Inst = cast<Instruction>(U);
    Function* F = Inst->getFunction();
    auto iter = AllocaMap.find(F);
    AllocaInst* Alloca;
    if(iter!=AllocaMap.end())Alloca = iter->second;
    else{
      StringRef Name = GV->hasName()?GV->getName():"";
      Alloca = new AllocaInst(GV->getType()->getPointerElementType(), 0, Name, &F->getEntryBlock().front());

      if(GV->hasInitializer()){
        StoreInst* Store = new StoreInst(GV->getInitializer(), Alloca);
        Store->insertAfter(Alloca);
      }
      
      AllocaMap[F] = Alloca;
    }
    U->replaceUsesOfWith(GV, Alloca);
  }

  GV->eraseFromParent();
}

struct LocalizationPass : public llvm::ModulePass {
  static char ID; // Pass identification, replacement for typeid
  LocalizationPass() : ModulePass(ID) {}
  bool runOnModule(llvm::Module &) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<DepandentGraphPass>();
    AU.setPreservesAll();
  }
};

bool LocalizationPass::runOnModule(Module &M) {
  std::vector<GlobalVariable*> GVs;
  
  DepandentGraphPass &DGP = getAnalysis<DepandentGraphPass>();

  for(auto iter:DGP.SCCVector){
    for(DepandentNode* DN:*iter){
      if(GlobalVariable* GV = dyn_cast<GlobalVariable>(DN->Value))
        GVs.push_back(GV);
    }
  }

  reverse(GVs.begin(), GVs.end());

  for(GlobalVariable* GV : GVs){
    LocalizationVariable(GV);
  }

  GlobalVariableCounter += GVs.size();
  return GVs.size()!=0;
}



char LocalizationPass::ID = 0;

static RegisterPass<LocalizationPass> X("Localization", "Localization all global variable");

ModulePass* createLocalizationPass (){
  return new LocalizationPass();
}
