/**
    \file testbed/cpptest13.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include <iostream>
#include <string.h>
#include <deque>
#include <algorithm>

int main() {
  char buf[] = "abcdefghijklmnop";
  memcpy(buf, buf + 4, 10);
  std::cout << buf << std::endl;

  std::deque<int> abc;
  for (size_t i = 0; i < 20; i++) {
    abc.push_back(20 - i);
  }
  std::stable_sort(abc.begin(), abc.end());

  int i = 0;
  for (std::deque<int>::iterator abciter = abc.begin(); abciter != abc.end();) {
    i++;
    std::cout << "topvalue " << *abciter << std::endl;
    if (i == 4) {
      *abciter = *abciter + 20;
      std::stable_sort(abc.begin(), abc.end());
      abciter = abc.begin();
      i = 0;
    } else {
      abciter = abc.erase(abciter);
    }
  }
  return 0;
}
