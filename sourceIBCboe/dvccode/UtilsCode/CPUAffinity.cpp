#include "dvccode/Utils/CPUAffinity.hpp"

std::map<std::string, int> CPUManager::proc_name_to_core_id_;

int CPUManager::GetCoreForProc(std::string proc) {
  if (proc_name_to_core_id_.find(proc) != proc_name_to_core_id_.end()) {
    return proc_name_to_core_id_[proc];
  }
  return -1;
}
