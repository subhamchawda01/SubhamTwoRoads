#include <iostream>
#include <cstring>

#define NUM_DISKS 16

unsigned long hash(char* str) {
  unsigned long hash = 5381;
  int c;

  while (c = *str++) hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return (hash % NUM_DISKS);
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cout << "USAGE: <exec> <NAS folder path>\n";
    return 0;
  }
  std::cout << "/media/ephemeral" << hash(argv[1]) << "\n";
  return 0;
}
