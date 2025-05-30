#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

/// to compile g++ -o exec_test24 cpptest24.cpp

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cout << "Usage: " << argv[0] << " filename F_OK|R_OK|RW_OK" << std::endl;
    exit(0);
  }

  if (strcmp(argv[2], "F_OK") == 0) {
    std::cout << (access(argv[1], F_OK) == 0) << std::endl;
  } else if (strcmp(argv[2], "R_OK") == 0) {
    std::cout << (access(argv[1], R_OK) == 0) << std::endl;
  } else if (strcmp(argv[2], "RW_OK") == 0) {
    std::cout << (access(argv[1], R_OK | W_OK) == 0) << std::endl;
  }

  return 0;
}
