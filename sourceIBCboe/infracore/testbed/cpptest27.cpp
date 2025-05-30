#include <stdio.h>
#include <iostream>

struct Abc {
  int a;
  int b;
  int c;
  explicit Abc(int _a_, int _b_, int _c_) : a(_a_), b(_b_), c(_c_) {}
};

inline void incrA(Abc& _abc_) { _abc_.b *= 7; }

int main() {
  Abc abc_(0, 2, 4);
  Abc xyz_(4, 2, 0);
  incrA(abc_);
  // abc_.b *= 7;
  if (abc_.b == 14) {
    std::cout << "HW" << std::endl;
  }
  return 0;
}
