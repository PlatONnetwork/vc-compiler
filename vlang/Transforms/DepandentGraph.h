#include <map>

#include "llvm/ADT/Statistic.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/ADT/SCCIterator.h"

#include "GraphNode.h"

typedef GraphNode<llvm::GlobalValue*> DepandentNode;


struct DepandentGraphPass : public llvm::ModulePass {
  static char ID; // Pass identification, replacement for typeid
  DepandentGraphPass() : ModulePass(ID){}

  bool runOnModule(llvm::Module &) override;

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

public:

  std::vector<llvm::scc_iterator<DepandentNode> > SCCVector;
  
  std::map<llvm::GlobalValue*, DepandentNode*> NodeMap;

};


llvm::ModulePass* createDepandentGraphPass ();


