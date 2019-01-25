#include <vector>

#include "llvm/Option/Arg.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Option/OptTable.h"
#include "llvm/Option/Option.h"
#include "llvm/Support/raw_ostream.h"

#include "clang/Driver/Options.h"

#include "Option.h"

using namespace std;
using namespace llvm;
using namespace opt;

static const char *const prefix_0[] = {nullptr};
static const char *const prefix_1[] = {"-", nullptr};
static const char *const prefix_3[] = {"--", nullptr};

enum ID {
  OPT_INVALID,
  OPT_INPUT,
  OPT_UNKNOWN,
  OPT_args,
  OPT_ir,
};

static const OptTable::Info InfoTable[] = {
    //    struct Info { Prefixes, Name, HelpText, MetaVar, ID, Kind, Param,
    //    Flags, GroupID, AliasID, AliasArgs, Values};
    {prefix_0, "<input>", nullptr, nullptr, OPT_INPUT, Option::InputClass, 0, 0,
     OPT_INVALID, OPT_INVALID, nullptr, nullptr},

    {prefix_0, "<unknown>", nullptr, nullptr, OPT_UNKNOWN, Option::UnknownClass,
     0, 0, OPT_INVALID, OPT_INVALID, nullptr, nullptr},

    {prefix_1, "args=", "Pass the comma separated arguments in <arg> to debug", "<arg>", OPT_args, Option::CommaJoinedClass, 0, 0,
     OPT_INVALID, OPT_INVALID, nullptr, nullptr},

    {prefix_1, "ir", "Write ir to <file>", "<file>", OPT_ir, Option::JoinedOrSeparateClass, 0, 0,
     OPT_INVALID, OPT_INVALID, nullptr, nullptr},
};

class VlangOptTable : public OptTable {
public:
  VlangOptTable() : OptTable(InfoTable) {}
};

std::unique_ptr<OptTable> createVlangOptTable() {
  return llvm::make_unique<VlangOptTable>();
}

bool ParseArgs(int argc, char *argv[], VlangOption &vlangOpts) {

  unique_ptr<OptTable> Opts = createVlangOptTable();

  unsigned MissingArgIndex, MissingArgCount;

  InputArgList Args = Opts->ParseArgs(makeArrayRef(argv + 1, argc - 1),
                                      MissingArgIndex, MissingArgCount);

  vector<const char*> clangArgv;

  vlangOpts.OutputVCCFile = "vcc.cpp";

  for (const Arg *A : Args) {
    const Option &Option = A->getOption();
    if (Option.matches(OPT_ir))
      vlangOpts.OutputIRFile = A->getValue();
    else if(Option.matches(OPT_args)){
      for(const char* arg : A->getValues())
        vlangOpts.debugArgs.push_back(arg);
    }
    else
      clangArgv.push_back(A->getSpelling().data());
  }

  unique_ptr<OptTable> clangOpts = clang::driver::createDriverOptTable();

  InputArgList clangArgs = clangOpts->ParseArgs(clangArgv, MissingArgIndex, MissingArgCount);

  for (const Arg *A : clangArgs) {
    const Option &Option = A->getOption();
    if (Option.matches(clang::driver::options::OPT_INPUT))
      vlangOpts.InputFilenames.push_back(A->getValue());
    else if(Option.matches(clang::driver::options::OPT_help))
      vlangOpts.Help = true;
    else if(Option.matches(clang::driver::options::OPT_o))
      vlangOpts.OutputVCCFile = A->getValue();
    else {
      // PlangOpts.clangArgs.push_back(A->getSpelling().data());
      ArgStringList ASL;
      A->render(clangArgs, ASL);
      for (auto it : ASL)
        vlangOpts.clangArgs.push_back(it);
    }
  }

  if(vlangOpts.Help){
    Opts->PrintHelp(
        llvm::outs(), "vlang [clang args]", 
        "PlatON C++ Verifiable Computation compiler",
        0, 0, false);
    return false;
  }

  return true;
}
