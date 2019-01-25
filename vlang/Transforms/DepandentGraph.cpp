#include <map>
#include <set>

#include "llvm/ADT/Statistic.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/ADT/SCCIterator.h"

#include "GraphNode.h"
#include "DepandentGraph.h"

using namespace llvm;
using namespace std;
/*
template<typename T>
class Graph {
public:
  T Value;
  std::vector<Graph<T>*> Edges;
};
*/

void getGlobalUsers(Value* V, std::set<GlobalValue*> &GlobalUsers){
  if(GlobalValue* GV = dyn_cast<GlobalValue>(V))
    GlobalUsers.insert(GV);
  else if(Instruction* Inst = dyn_cast<Instruction>(V))
    GlobalUsers.insert(Inst->getFunction());
  else {
    for(User* U : V->users())
      getGlobalUsers(U, GlobalUsers);
  }
}




bool DepandentGraphPass::runOnModule(Module &M) {

  for(llvm::GlobalValue &GV : M.global_values()){
    DepandentNode* DN = new DepandentNode;
    DN->Value = &GV;
    NodeMap[&GV] = DN;
  }

  for(llvm::GlobalValue &GV : M.global_values()){

    std::set<GlobalValue*> GlobalUsers;
    for(User* U : GV.users())
      getGlobalUsers(U, GlobalUsers);
    
    for(GlobalValue* U : GlobalUsers){
     // NodeMap[&GV]->Edges.push_back(NodeMap[U]);
      NodeMap[U]->Edges.push_back(NodeMap[&GV]);
    }
  }

  DepandentNode External;

  for(llvm::GlobalValue &GV : M.global_values()){
    External.Edges.push_back(NodeMap[&GV]);
  }


  for(auto iter=scc_begin(External); (*iter)[0] != &External; ++iter){
    SCCVector.push_back(iter);
  }

  return false;
}

void DepandentGraphPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}



ModulePass* createDepandentGraphPass (){
  return new DepandentGraphPass();
}


char DepandentGraphPass::ID = 0;

static RegisterPass<DepandentGraphPass> X("depandent-graph", "analysis depandent graph", false, true);

