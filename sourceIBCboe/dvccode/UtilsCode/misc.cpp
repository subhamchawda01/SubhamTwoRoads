#include "dvccode/Utils/misc.hpp"
#include <fstream>
#include <unistd.h>
#include <sstream>
#include "dvccode/Utils/CPUAffinity.hpp"
#include <sys/syscall.h>

using namespace std;

namespace Misc {
int ReadSequenceNo(const std::string& fname, ulong* s1, ulong* s2) {
  ifstream fin(fname);
  if (!fin.good()) return -1;
  fin >> (*s1) >> (*s2);
  return 0;
}
int WriteSequenceNo(const std::string& fname, ulong s1, ulong s2) {
  ofstream fin(fname, std::ofstream::out | std::ofstream::trunc);
  if (!fin.good()) return -1;
  fin << s1 << " " << s2;
  return 0;
}
std::string ReadHostName() {
  char hostname[128];
  return gethostname(hostname, 127) ? "unknown-host" : hostname;
}
void AffineMe2Init() {
  int threadId = ((int)(syscall(SYS_gettid)));
  /*int core_alloced_ = */ CPUManager::AffinToInitCores(threadId);  // unused variable commented
}
}
