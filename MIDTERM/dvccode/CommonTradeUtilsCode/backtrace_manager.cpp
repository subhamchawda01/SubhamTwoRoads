#include "dvccode/CommonTradeUtils/backtrace_manager.hpp"

namespace HFSAT {

BackTraceManager::BackTraceManager(unsigned int max_frames) : max_frames_(max_frames) {}

void BackTraceManager::collect_stacktrace(std::ostringstream& t_temp_oss) {
  // storage array for stack trace address data
  void* addrlist[max_frames_ + 1];

  int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));
  if (addrlen == 0) {
    t_temp_oss << "Backtrace failed. \n";
  } else {
    // resolve addresses into strings containing "filename(function+address)",
    //     //     // this array must be free()-ed
    char** symbollist = backtrace_symbols(addrlist, addrlen);

    // allocate string which will be filled with the demangled function name
    size_t funcnamesize = 256;
    char* funcname = (char*)malloc(funcnamesize);
    // iterate over the returned symbol lines. skip the first, it is the
    //     // address of this function.
    for (int i = 1; i < addrlen; i++) {
      char *begin_name = 0, *begin_offset = 0, *end_offset = 0;
      // find parentheses and +address offset surrounding the mangled name:
      //         //      // ./module(function+0x15c) [0x8048a6d]

      for (char* p = symbollist[i]; *p; ++p) {
        if (*p == '(')
          begin_name = p;
        else if (*p == '+')
          begin_offset = p;
        else if (*p == ')' && begin_offset) {
          end_offset = p;
          break;
        }
      }
      if (begin_name && begin_offset && end_offset && begin_name < begin_offset) {
        *begin_name++ = '\0';
        *begin_offset++ = '\0';
        *end_offset = '\0';

        int status;
        char* ret = abi::__cxa_demangle(begin_name, funcname, &funcnamesize, &status);
        if (status == 0) {
          funcname = ret;  // use possibly realloc()-ed string
          t_temp_oss << symbollist[i] << " : " << funcname << "+" << begin_offset << "\n";
        } else {
          t_temp_oss << symbollist[i] << " : " << begin_name << "+" << begin_offset << "\n";
        }
      } else {  // couldn't parse the line? print the whole line.
        t_temp_oss << symbollist[i] << "\n";
      }
    }
    free(funcname);
    free(symbollist);
  }
}
}
