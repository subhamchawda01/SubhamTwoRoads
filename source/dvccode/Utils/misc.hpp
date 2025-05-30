#ifndef UTILS_HPP
#define UTILS_HPP
#include <string>
#include <dvccode/CDef/defines.hpp>

namespace Misc {
template <typename T>
static size_t deleteAll(T &xs) {
  size_t count = 0;
  for (auto x : xs) {
    delete x;
    count++;
  }
  return count;
}

int ReadSequenceNo(const std::string &fname, ulong *s1, ulong *s2);
int WriteSequenceNo(const std::string &fname, ulong s1, ulong s2);
std::string ReadHostName();
void AffineMe2Init();
inline bool PowerOf2(unsigned long long x) { return !(x & (x - 1)); }
}
#endif
