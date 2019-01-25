
#include <vector>
#include <cstdlib>
#include <cstdio>

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h" 
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"

#include "goLayer.h"

using namespace llvm;
using namespace std;

extern "C" void keypairGen(unsigned primary_input_size, std::string &pkey, std::string &vkey);

unsigned char createGadget(void* input0, void* input1, void* input2, void* result, int32 Type){
  return gadget_createGadget((int64_t)input0, (int64_t)input1, (int64_t)input2, (int64_t)result, Type); 
}

E_GType getBOGType(llvm::BinaryOperator::BinaryOps Opcode){
  switch(Opcode){
    case llvm::BinaryOperator::Add: return G_ADD;
    case llvm::BinaryOperator::Sub: return G_SUB;
    case llvm::BinaryOperator::Mul: return G_MUL;
    case llvm::BinaryOperator::SDiv: return G_SDIV;
    case llvm::BinaryOperator::SRem: return G_SREM;
    case llvm::BinaryOperator::UDiv: return G_UDIV;
    case llvm::BinaryOperator::URem: return G_UREM;
    case llvm::BinaryOperator::And: return G_BITW_AND;
    case llvm::BinaryOperator::Or: return G_BITW_OR;
    case llvm::BinaryOperator::Xor: return G_BITW_XOR;
    default:
      llvm_unreachable("never handle Opcode");
  }
}

E_GType getICmpGType(llvm::ICmpInst::Predicate P){
  switch(P){
    case llvm::ICmpInst::ICMP_EQ: return G_EQ;
    case llvm::ICmpInst::ICMP_NE: return G_NEQ;
    case llvm::ICmpInst::ICMP_SGT: return G_SGT;
    case llvm::ICmpInst::ICMP_SGE: return G_SGE;
    case llvm::ICmpInst::ICMP_UGT: return G_UGT;
    case llvm::ICmpInst::ICMP_UGE: return G_UGE;
    default:
      llvm_unreachable("never handle Opcode");
  }
}

E_GType getCastGType(CastInst::CastOps CO){
  switch(CO){
    case CastInst::Trunc: return G_TRUNC;
    case CastInst::ZExt: return G_ZEXT;
    case CastInst::SExt: return G_SEXT;
    default:
      llvm_unreachable("never handle Opcode");
  }
}

void* getIntegerBitWidth(Type* T){
  return (void*)(long long unsigned)(T->getIntegerBitWidth());
}

bool GenerateKeypair(Function &F, string &pkey, string &vkey){
    // gadget init
    gadget_initEnv();

    for(Argument &A : F.args())
      gadget_createPBVar((int64_t)&A);

    ReturnInst* Ret = cast<ReturnInst>(F.getEntryBlock().getTerminator());

    gadget_createPBVar((int64_t)(Ret->getReturnValue()));


    for(Instruction &Inst : F.getEntryBlock()){
      Inst.print(llvm::outs());
      llvm::outs() << "\n";

      bool Success; 
      
      if(isa<ReturnInst>(&Inst))
        continue;
      else if(SelectInst *Select = dyn_cast<SelectInst>(&Inst))
        Success = createGadget(Select->getCondition(), Select->getTrueValue(), Select->getFalseValue(), Select, G_SELECT);
      else if(BinaryOperator* BO = dyn_cast<BinaryOperator>(&Inst))
        Success = createGadget(BO->getOperand(0), BO->getOperand(1), nullptr, BO, getBOGType(BO->getOpcode()));
      else if(ICmpInst* ICmp = dyn_cast<ICmpInst>(&Inst))
        Success = createGadget(ICmp->getOperand(0), ICmp->getOperand(1), nullptr, ICmp, getICmpGType(ICmp->getPredicate()));
      else if(CastInst* CI = dyn_cast<CastInst>(&Inst))
        Success = createGadget(CI->getOperand(0), getIntegerBitWidth(CI->getSrcTy()), getIntegerBitWidth(CI->getDestTy()), CI, getCastGType(CI->getOpcode()));
      else
        Success = false;
      
      if(!Success){
        llvm::errs() << "create gadget fail.\n";
        return false;
      }
    }
    
    // generate constraints.
    gadget_generateConstraints();

    keypairGen(F.arg_size()+1, pkey,  vkey);

    return true;
}

bool Verify(Function &F, std::string strPKey, std::string strVKey, std::vector<std::string> debugArgs){

    assert(debugArgs.size() == F.arg_size());

  
    for(Instruction &Inst : F.getEntryBlock())
      for(Value* V : Inst.operands())
        if(ConstantInt* CI = dyn_cast<ConstantInt>(V))
          gadget_setVar((int64_t)CI, CI->getValue().getZExtValue());
  
    for(unsigned i=0; i<F.arg_size(); i++)
      gadget_setVar((int64_t)(F.arg_begin()+i), strtol(debugArgs[i].c_str(), nullptr, 10));
    

	  gadget_generateWitness();

    ReturnInst* RetInst = cast<ReturnInst>(F.getEntryBlock().getTerminator());
    int64_t RetIndex = (int64_t)RetInst->getReturnValue();


#define PROOR_BUF_SIZE 0x400
#define RES_BUF_SIZE 0x100

    char Proof[PROOR_BUF_SIZE + 1] = {0};
    char Result[RES_BUF_SIZE + 1] = {0};
    

    int Success = true;
    Success = Success && GenerateProof(strPKey.c_str(), Proof, PROOR_BUF_SIZE);
    Success = Success && GenerateResult(RetIndex, Result, RES_BUF_SIZE);


    llvm::outs() << "Gen proof " <<  (Success ? "success" : "fail") << "!\n";
    llvm::outs() << "Result is " << Result << "\n";

    string inputs;
    
    for(unsigned i=0; i<F.arg_size(); i++)
      inputs = inputs + debugArgs[i] + '#';

    inputs.pop_back();


    Success = Success && Verify(strVKey.c_str(), Proof, inputs.c_str(), Result);
    
    llvm::outs() << "verify " <<  (Success ? "pass" : "fail") << "!\n";

    return Success;
}

