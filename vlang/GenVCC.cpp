
#include <vector>

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

E_GType getBOGType(llvm::BinaryOperator::BinaryOps);

E_GType getICmpGType(llvm::ICmpInst::Predicate);

E_GType getCastGType(CastInst::CastOps); 
void* getIntegerBitWidth(Type*);

void put_vc_createGadget(raw_fd_ostream &CodeStream, void *input0, void *input1, void* input2, void *result, int32 Type) {		
  CodeStream << format("  CreateRes = CreateRes && vc_CreateGadget(%p, %p, %p, %p, %d);", (int64_t)input0, (int64_t)input1, (int64_t)input2, (int64_t)result, Type) << "\n";
}

void GenerateGenBody(Function &F, raw_fd_ostream &CodeStream){

  for(Instruction &Inst : F.getEntryBlock()){

    if(isa<ReturnInst>(Inst))
      continue;
    else if(SelectInst *Select = dyn_cast<SelectInst>(&Inst))
      put_vc_createGadget(CodeStream, Select->getCondition(), Select->getTrueValue(), Select->getFalseValue(), Select, G_SELECT);
    else if(BinaryOperator* BO = dyn_cast<BinaryOperator>(&Inst))
      put_vc_createGadget(CodeStream, BO->getOperand(0), BO->getOperand(1), nullptr, BO, getBOGType(BO->getOpcode()));
    else if(ICmpInst* ICmp = dyn_cast<ICmpInst>(&Inst))
      put_vc_createGadget(CodeStream, ICmp->getOperand(0), ICmp->getOperand(1), nullptr, ICmp, getICmpGType(ICmp->getPredicate()));
    else if(CastInst* CI = dyn_cast<CastInst>(&Inst))
      put_vc_createGadget(CodeStream, CI->getOperand(0), getIntegerBitWidth(CI->getSrcTy()), getIntegerBitWidth(CI->getDestTy()), CI, getCastGType(CI->getOpcode()));
    else {
      llvm::errs() << "create vcc gadget fail.\n";
      return;
    }
  }

  for(Instruction &Inst : F.getEntryBlock())
    for(Value* V : Inst.operands())
      if(ConstantInt* CI = dyn_cast<ConstantInt>(V))
        CodeStream << format("  vc_SetVar(%p, %d, true);", CI, CI->getValue().getZExtValue()) << "\n";

}

void GenerateGenHead(Function &F, raw_fd_ostream &CodeStream){
  for(Argument &A : F.args())
    CodeStream << format("  vc_CreatePBVar(%p);", &A) << "\n";

  ReturnInst* Ret = cast<ReturnInst>(F.getEntryBlock().getTerminator());
  CodeStream << format("  vc_CreatePBVar(%p);", Ret->getReturnValue()) << "\n";
  CodeStream << format("  vc_SetRetIndex(%p);", Ret->getReturnValue()) << "\n";

  for(unsigned i=0; i<F.arg_size(); i++)
    CodeStream << format("  vc_SetVar(%p, atoi(vectInput[%u].c_str()), true);", F.arg_begin()+i, i) << "\n";
}

void GenerateGenGadget(Function &F, raw_fd_ostream &CodeStream){
  CodeStream << "bool GenGadget(std::vector<std::string> &vectInput){\n";
  CodeStream << "  const int64_t nil = 0;\n\n";
  CodeStream << "  // 2) create pb variable(all input & output variable)\n";
  CodeStream << "  // $create_pb_var$\n\n";

  GenerateGenHead(F, CodeStream);

  CodeStream << "\n";
  CodeStream << "  // 3) create gadget\n";
  CodeStream << "  // $create_gadget$\n";
  CodeStream << "  int64_t CreateRes = 1;\n\n";

  GenerateGenBody(F, CodeStream);
 
  CodeStream << "\n";
  CodeStream << "  if (!CreateRes){\n";
  CodeStream << "    platon::println(\"create gadget fail...\");\n";
  CodeStream << "    return false;\n";
  CodeStream << "  }\n\n";
  CodeStream << "  // 4) set all input pb value\n";
  CodeStream << "  // $set_pb_var$\n\n";
  CodeStream << "  return true;\n";
  CodeStream << "}\n";
}

void GenerateKey(std::string &pkey, std::string &vkey, raw_fd_ostream &CodeStream){

  CodeStream << "char PKEY_VALUE[] = \"";

  for(unsigned char c : pkey){
    //HexStream << format("%.2x", c);
    if(c=='\n')CodeStream << "\\n";
    else CodeStream << c;
  }

  CodeStream << "\";\n\n";

  CodeStream << "char VKEY_VALUE[] = \"";

  for(unsigned char c : vkey){
    //HexStream << format("%.2x", c);
    if(c=='\n')CodeStream << "\\n";
    else CodeStream << c;
  }

  CodeStream << "\";\n\n";
}


bool GenVCC(llvm::Function &F, std::string pkey, std::string vkey, std::string VCCOutputPath){

  std::error_code EC;
  ToolOutputFile VCCOutput(VCCOutputPath, EC, sys::fs::F_None);
  if(EC){ 
    SMDiagnostic(VCCOutputPath, SourceMgr::DK_Error,
                       "Could not open output file: " + EC.message());
    return false;
  }

  raw_fd_ostream &CodeStream = VCCOutput.os();

  CodeStream << "#include <vector>\n";
  CodeStream << "#include <string>\n";
  CodeStream << "#include <platon/platon.hpp>\n";
  CodeStream << "#include <platon/statevc.hpp>\n";
  CodeStream << "\n";

  GenerateKey(pkey, vkey, CodeStream);

  GenerateGenGadget(F, CodeStream);

  VCCOutput.keep();

  return true;
}
