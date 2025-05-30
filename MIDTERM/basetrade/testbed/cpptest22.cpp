// range heap example
#include <iostream>
#include <algorithm>
#include <vector>
#include "basetrade/testbed/ticks.hpp"

#define LEN 10240

template <class T>
void showvec(std::vector<T>& v) {
  std::cout << "sorted range :";
  for (unsigned i = 0; i < v.size(); i++) std::cout << " " << v[i];
  std::cout << std::endl;
}

int main() {
  int checkfreq = 0x10;
  int abc = 16;
  ticks p1 = GetTicks();
  for (int i = 0; i < LEN; i++) {
    abc = 2 * 2 * 2 * 2;
    if (abc == checkfreq) {
      abc++;
    }
    // if ( abc & checkfreq ) {
    //   abc ++;
    // }
  }
  ticks p2 = GetTicks();
  std::cout << (p2 - p1) / LEN << std::endl;
  return 0;
}
