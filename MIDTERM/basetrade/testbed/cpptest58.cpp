#include <math.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <memory>
#include <string>
#include <numeric>

int main(int argc, char** argv) {
  std::vector<const char*> vec;
  vec.push_back("I");
  vec.push_back("am");
  vec.push_back("Sam");

  std::string a = std::accumulate(vec.begin(), vec.end(), std::string(""));

  std::cout << a << std::endl;
}
