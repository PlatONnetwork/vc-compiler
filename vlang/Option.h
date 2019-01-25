#include <string>
#include <vector>

struct VlangOption {

  std::vector<std::string> InputFilenames;

  bool Help = false;

  std::string OutputVCCFile;

  std::string OutputIRFile;

  std::vector<std::string> clangArgs;

  std::vector<std::string> debugArgs;
};
