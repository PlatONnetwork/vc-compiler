#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "llvm/ADT/StringRef.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SystemUtils.h"
#include "llvm/Support/Timer.h"
#include "llvm/Support/ToolOutputFile.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/Sema/Sema.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/CodeGen/BackendUtil.h"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/CodeGen/ModuleBuilder.h"
#include "clang/Driver/Options.h"
#include "clang/Frontend/ASTUnit.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Tooling/Tooling.h"

#include "Option.h"

using namespace std;
using namespace llvm;
using namespace clang;
using namespace tooling;
using namespace opt;

bool ParseArgs(int, char *[], VlangOption &);

void VCPass(llvm::Module &);

bool GenerateKeypair(Function &F, string &pkey, string &vkey);

bool Verify(Function &F, std::string strPKey, std::string strVKey, std::vector<std::string> debugArgs);

void GenVCC(llvm::Function &, std::string, std::string, std::string);

class FunctionTemplateCGVisitor : public RecursiveASTVisitor<FunctionTemplateCGVisitor> {
  CodeGenerator *CG;
public:
  FunctionTemplateCGVisitor(CodeGenerator *CG):CG(CG){}
  bool VisitFunctionTemplateDecl(FunctionTemplateDecl* FTD){
    for(FunctionDecl* FD : FTD->specializations())
      CG->HandleTopLevelDecl(DeclGroupRef(FD));
    return true;
  }
};

class BuilderAction : public ToolAction {
public:
  Linker &Link;
  LLVMContext &llvmContext;

  BuilderAction(Linker &Link, LLVMContext &llvmContext)
      : Link(Link), llvmContext(llvmContext) {}

  bool runInvocation(std::shared_ptr<CompilerInvocation> Invocation,
                     FileManager *Files,
                     std::shared_ptr<PCHContainerOperations> PCHContainerOps,
                     DiagnosticConsumer *DiagConsumer) override {

    std::unique_ptr<llvm::Module> M;

    FrontendInputFile &Input = Invocation->getFrontendOpts().Inputs[0];

    IntrusiveRefCntPtr<clang::DiagnosticsEngine> Diags =
        CompilerInstance::createDiagnostics(&Invocation->getDiagnosticOpts(),
                                            DiagConsumer, false);

    if (Input.getKind().getLanguage() == InputKind::LLVM_IR) {

      SMDiagnostic Err;

      M = parseIRFile(Input.getFile(), Err, llvmContext);

    } else {
      std::unique_ptr<ASTUnit> AST = ASTUnit::LoadFromCompilerInvocation(
          Invocation, std::move(PCHContainerOps), Diags, Files);

      if (!AST)
        return false;

      ASTContext &astContext = AST->getASTContext();
      TranslationUnitDecl *UnitDecl = astContext.getTranslationUnitDecl();


      CodeGenerator *CG = CreateLLVMCodeGen(
          *Diags, "module0", Invocation->getHeaderSearchOpts(),
          Invocation->getPreprocessorOpts(), Invocation->getCodeGenOpts(),
          llvmContext);

      CG->Initialize(astContext);

      for (Decl *D : UnitDecl->decls())
        CG->HandleTopLevelDecl(DeclGroupRef(D));

      FunctionTemplateCGVisitor Visitor(CG);
      Visitor.TraverseDecl(UnitDecl);

      for (Decl *D : AST->getSema().WeakTopLevelDecls())
        CG->HandleTopLevelDecl(DeclGroupRef(D));


      CG->HandleTranslationUnit(AST->getASTContext());

      M = std::unique_ptr<llvm::Module>(CG->GetModule());
    }

    std::unique_ptr<raw_pwrite_stream> sss;

    clang::EmitBackendOutput(
          *Diags, *Invocation->HeaderSearchOpts, Invocation->getCodeGenOpts(),
          Invocation->getTargetOpts(), *Invocation->getLangOpts(),
          M->getDataLayout(), M.get(), Backend_EmitNothing, std::move(sss));


    bool Err = Link.linkInModule(std::move(M), 0);

    return !Err;
  }
};

void OutputIR(llvm::Module &M, VlangOption &Option) {
  if (!Option.OutputIRFile.empty()){

    std::error_code EC;
    ToolOutputFile Out(Option.OutputIRFile, EC, sys::fs::F_None);
    if (EC) {
      errs() << EC.message() << '\n';
      return;
    }
  
    if (verifyModule(M, &errs())) {
      errs() << ": error: linked module is broken!\n";
      return;
    }
  
    M.print(Out.os(), nullptr, false);
  
    Out.keep();
  }
}

int main(int argc, char **argv) {
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  llvm::PrettyStackTraceProgram X(argc, argv);

  VlangOption Option;

  if (!ParseArgs(argc, argv, Option))
    return 0;

  FixedCompilationDatabase Compilations(".", Option.clangArgs);

  ClangTool Tool(Compilations, Option.InputFilenames);

  LLVMContext Context;
  llvm::Module M("module", Context);
  Linker Link(M);

  BuilderAction Builder(Link, Context);

  Tool.run(&Builder);


  VCPass(M);

  OutputIR(M, Option);


  Function &F = M.getFunctionList().front();

  string pkey, vkey;

  GenerateKeypair(F, pkey, vkey);

  if(!Option.debugArgs.empty())
    Verify(F, pkey, vkey, Option.debugArgs);

  GenVCC(F, pkey, vkey, Option.OutputVCCFile);

  return 0;
}
