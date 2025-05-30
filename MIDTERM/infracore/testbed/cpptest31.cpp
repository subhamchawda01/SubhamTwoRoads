#include <stdlib.h>
#include <iostream>

int main(int argc, char** argv) {
  int abc = atoi(argv[1]);

  std::cout << abc << std::endl;
  abc = abc << 16;
  std::cout << abc << std::endl;

  return 0;
}
