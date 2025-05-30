/**
    \file test_unsorted_vector.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include "dvccode/CommonDataStructures/unordered_vector.hpp"
#include <iostream>

struct dummy {
  int i;
  int j;
  int k;

  dummy(int x) : i(x), j(x * x), k(x * x * x) {}
};

void print(HFSAT::UnOrderedVec<int>& vec) {
  for (unsigned int i = 0; i < vec.size(); i++) {
    std::cout << vec.at(i) << ", ";
  }
  std::cout << "\n";
}

void print(HFSAT::UnOrderedVec<dummy>& vec) {
  for (unsigned int i = 0; i < vec.size(); i++) {
    std::cout << "(" << vec.at(i).i << ", " << vec.at(i).j << "," << vec.at(i).k << "), ";
  }
  std::cout << "\n";
}

int main() {
  {
    HFSAT::UnOrderedVec<int> vec;

    for (int i = 0; i < 21; i++) vec.push_back(i);

    std::cout << "\nOriginal vec \n\t:";
    print(vec);

    for (int i = 0; vec.size(); i++) {
      unsigned int j = (i * i + 1) % vec.size();  // some pseudo ramdom index
      vec.remove_and_delete(j);
      std::cout << "\ndeleted index " << j << "\n\t:";
      print(vec);
    }
  }
  std::cout << "===============\n";
  {
    HFSAT::UnOrderedVec<dummy> vec;

    for (int i = 0; i < 18; i++) vec.push_back(dummy(i));
    std::cout << "\nOriginal vec \n\t:";
    print(vec);

    for (int i = 0; vec.size(); i++) {
      unsigned int j = (i * i * i + 3443) % vec.size();  // some pseudo ramdom index
      vec.remove_and_delete(j);
      std::cout << "\ndeleted index " << j << "\n\t:";
      print(vec);
    }
  }

  return 0;
}
// command to compile
// g++ -o test_vec test_unordered_vector.cpp -I../../infracore_install/  -L../../infracore_install/lib/
