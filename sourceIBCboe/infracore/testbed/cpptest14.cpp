#include <iostream>
#include <stdio.h>

template <typename T>
inline T signum(T t) {
  return ((t < 0) ? -1 : 1);
}

int main() {
  int a;
  a = -42;
  std::cout << signum<int>(a) << std::endl;

  int sc = 1291099800;
  int usc = 18486;
  fprintf(stderr, "%d.%06d", sc, usc);
  return 0;
}
