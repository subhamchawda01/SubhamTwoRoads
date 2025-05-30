#include <iostream>
#include <vector>
#include <map>
#include <tr1/unordered_map>
#include <algorithm>
#include <boost/lexical_cast.hpp>

#define LEN 100

typedef unsigned long long ticks;

static __inline__ ticks GetTicks(void) {
  unsigned a, d;
  asm volatile("rdtsc" : "=a"(a), "=d"(d));
  return ((ticks)a) | (((ticks)d) << 32);
}

int main(int argc, char** argv) {
  int i = 0;
  int j = 0;
  int k = 0;
  std::string abc;
  std::string ONE = boost::lexical_cast<std::string>(1);

  std::map<std::string, int> __B__map_;

  std::tr1::unordered_map<std::string, int> __C__map_;

  ticks p1 = GetTicks();
  /** start */
  for (int i = 0; i < LEN; i++) {
    abc = boost::lexical_cast<std::string>(i);
  }
  /** end takes 1033 cycles */
  ticks p2 = GetTicks();
  /** start */
  for (int i = 0; i < LEN; i++) {
    __C__map_[boost::lexical_cast<std::string>(i)] = i;
  }
  /** end takes 2803 cycles */
  ticks p3 = GetTicks();
  /** start */

  for (int i = 0; i < LEN; i++) {
    __B__map_[boost::lexical_cast<std::string>(i)] = i;
  }

  /** end takes 6920 cycles */
  ticks p4 = GetTicks();
  /** start */

  i += __B__map_[ONE];

  /** end takes 2600 cycles */
  ticks p5 = GetTicks();
  /** start */

  i += __C__map_[ONE];

  /** end takes 1950 cycles */
  ticks p6 = GetTicks();

  std::cout << (p2 - p1) << " " << (p3 - p2) << " " << (p4 - p3) << " " << (p5 - p4) << " " << (p6 - p5) << std::endl;
}
