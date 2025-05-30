#include <iostream>
#include <cstring>

#define NUM_DISKS 16

unsigned long hash(char* str) {
  unsigned long hash = 5381;
  int c;

  while ((c = *str++) != 0) hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return (hash % NUM_DISKS);
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cout << "USAGE: <exec> <NAS folder path>\n";
    std::cout << "Returns allocated disk after hashing filename (./NAS1/...)";
    return 0;
  }
  std::cout << "/media/shared/ephemeral" << hash(argv[1]) << "/s3_cache/" << argv[1]
            << "\n";
  return 0;
}
