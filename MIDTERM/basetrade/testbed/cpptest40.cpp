/// Aim is to test optimization of U64FieldDefinition

#include <stdint.h>
#include <stdlib.h>
#include <iostream>

int main(int argc, char** argv) {
  if (argc < 2) return 0;

  uint64_t abc = 20081003628331008;  // 20081007091208008  ;
  uint64_t xyz = abc + atol(argv[1]);

  uint64_t stabval = 0xffffffffffff0000;

  std::cout << ((xyz & stabval) - (abc & stabval)) << std::endl;

  return 0;
}
