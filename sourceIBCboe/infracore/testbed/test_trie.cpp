/**
    \file test.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

     Created on: Feb 7, 2012
        Author: piyush

 */

#include <string.h>
#include <iostream>
#include <inttypes.h>
#include <stdlib.h>
#include "dvccode/CommonDataStructures/trie.hpp"

static __inline__ uint64_t GetCpucycleCount() {
  uint32_t lo, hi;
  __asm__ __volatile__(  // serialize
      "xorl %%eax,%%eax \n        cpuid" ::
          : "%rax", "%rbx", "%rcx", "%rdx");
  /* We cannot use "=A", since this would use %rax on x86_64 and return only the lower 32bits of the TSC */
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return (uint64_t)hi << 32 | lo;
}

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout << "incorrect usage\n";
    std::cout << "usage: " << argv[0]
              << " <mode 1: SimpleTrie-Accuracy, 2: SimpleTrie-Speed, 3: CompactTrie-Accuracy, 4: CompactTrie-Speed\n";
    std::cout << "example: " << argv[0] << " 1\n";
    exit(-1);
  }

  std::string words[15] = {"a",   "aa",   "aaa", "bbbb", "bbb", "bb", "b",  "a",
                           "aaa", "bbbb", "b",   "xab",  "xa",  "xb", "xba"};
  std::string words_absent[15] = {"ax",   "aax",   "aaax", "bbxbb", "bbbx", "bbx", "bx",  "ax",
                                  "aaax", "bbbbx", "bx",   "xabx",  "xax",  "xbx", "xbax"};

  int mode = atoi(argv[1]);
  int max_iter = 1000000;

  switch (mode) {
    case 1: {
      std::cout << " testing functionalities add and find in simple tries. 1st set all present, 2nd set all absent\n";

      HFSAT::SimpleTrie* p = new HFSAT::SimpleTrie();
      for (int i = 0; i < 15; ++i) p->add((char*)words[i].c_str());

      for (int i = 0; i < 15; ++i) std::cout << words[i] << " " << p->find((char*)words[i].c_str()) << "\n";
      for (int i = 0; i < 15; ++i)
        std::cout << words_absent[i] << " " << p->find((char*)words_absent[i].c_str()) << "\n";
    } break;
    case 2: {
      std::cout << " testing performance of simple tries. time taken for 15 words search. first set all present, 2nd "
                   "set all absent\n";
      HFSAT::SimpleTrie* p = new HFSAT::SimpleTrie();
      for (int i = 0; i < 15; ++i) p->add((char*)words[i].c_str());

      uint64_t st = GetCpucycleCount();
      int sum = 0;
      for (int numRep = 0; numRep < max_iter; numRep++)
        for (int i = 0; i < 15; ++i) sum += p->find((char*)words[i].c_str());
      st = GetCpucycleCount() - st;
      std::cout << "time taken in + search " << st / max_iter << "\n";

      st = GetCpucycleCount();
      for (int numRep = 0; numRep < max_iter; numRep++)
        for (int i = 0; i < 15; ++i) sum += p->find((char*)words_absent[i].c_str());
      st = GetCpucycleCount() - st;
      std::cout << "time taken in - search " << st / max_iter << "\n";
    } break;
    case 3: {
      std::cout << " testing functionalities add and find in compact tries. 1st set all present, 2nd set all absent\n";

      HFSAT::CompactTrie* p = new HFSAT::CompactTrie();
      for (int i = 0; i < 15; ++i) p->add((char*)words[i].c_str());

      for (int i = 0; i < 15; ++i) std::cout << words[i] << " " << p->find((char*)words[i].c_str()) << "\n";
      for (int i = 0; i < 15; ++i)
        std::cout << words_absent[i] << " " << p->find((char*)words_absent[i].c_str()) << "\n";
    } break;
    case 4: {
      std::cout << " testing performance of compact tries. time taken for 15 words search. first set all present, 2nd "
                   "set all absent\n";
      HFSAT::CompactTrie* p = new HFSAT::CompactTrie();
      for (int i = 0; i < 15; ++i) p->add((char*)words[i].c_str());

      uint64_t st = GetCpucycleCount();
      int sum = 0;
      for (int numRep = 0; numRep < max_iter; numRep++)
        for (int i = 0; i < 15; ++i) sum += p->find((char*)words[i].c_str());
      st = GetCpucycleCount() - st;
      std::cout << "time taken in + search " << st / max_iter << "\n";

      st = GetCpucycleCount();
      for (int numRep = 0; numRep < max_iter; numRep++)
        for (int i = 0; i < 15; ++i) sum += p->find((char*)words_absent[i].c_str());
      st = GetCpucycleCount() - st;
      std::cout << "time taken in - search " << st / max_iter << "\n";
    } break;
  }

  HFSAT::CompactTrie* p = new HFSAT::CompactTrie();
  //  HFSAT::SimpleTrie* p = new HFSAT::SimpleTrie();
}

// compile command: g++ -O2  -o test test_trie.cpp -I../../infracore_install/ -L../../infracore_install/lib/
