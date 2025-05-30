#include <iostream>
#include <vector>
#include <map>
#include <tr1/unordered_map>
#include <algorithm>

#define LEN 1024

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
  std::map<int, int, std::greater<int> > __A__map_;
  std::map<int, int, std::less<int> > __B__map_;

  std::tr1::unordered_map<int, int> __C__map_;

  ticks p1 = GetTicks();
  /** start */
  for (int i = 0; i < LEN; i++) {
    __A__map_[i] = i;
  }
  /** end takes 1684 cycles per LEN */
  ticks p2 = GetTicks();

  /** start */
  for (int i = 0; i < LEN; i++) {
    __B__map_[i] = i;
  }
  /** end takes 1570 cycles per LEN */
  ticks p3 = GetTicks();
  /** start */

  for (int i = 0; i < LEN; i++) {
    __C__map_[i] = i;
  }

  /** end takes 969 cycles per LEN */
  ticks p4 = GetTicks();
  /** start */

  std::map<int, int, std::greater<int> >::iterator alpha = __A__map_.begin();

  /** end takes 110 cycles */
  ticks p5 = GetTicks();
  /** start */

  std::map<int, int, std::greater<int> >::iterator beta = __B__map_.begin();

  /** end takes 90 cycles */
  ticks p6 = GetTicks();
  /** start */

  i += __A__map_[1];

  /** end takes 1373 cycles */
  ticks p7 = GetTicks();
  /** start */

  i += __B__map_[1];

  /** end takes 1385 cycles */
  ticks p8 = GetTicks();
  /** start */

  i += __C__map_[1];

  /** end takes 308 cycles */
  ticks p9 = GetTicks();
  /** start */

  j = alpha->second;

  /** end takes 63 cycles */
  ticks p10 = GetTicks();
  /** start */

  k = beta->second;

  /** end takes 56 cycles */
  ticks p11 = GetTicks();
  /** start */

  std::cout << (p2 - p1 + .0) / LEN << " " << (p3 - p2) << " " << (p4 - p3) << " " << (p5 - p4) << " " << (p6 - p5)
            << " " << (p7 - p6) << " " << (p8 - p7) << " " << (p9 - p8) << " " << (p10 - p9) << " " << (p11 - p10)
            << std::endl;
}
