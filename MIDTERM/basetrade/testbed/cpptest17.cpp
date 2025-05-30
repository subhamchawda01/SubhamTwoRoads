/**
    \file testbed/cpptest17.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include <stdlib.h>
#include <iostream>

int main() {
  double prob_success_ = 0.85;
  int max_allowed_val_ = prob_success_ * RAND_MAX;

  int passed_ = 0;
  unsigned int useed = (unsigned long)(&passed_);
  srand(useed);
  for (auto i = 0u; i < 100; i++) {
    int tint = rand();
    passed_ += (tint < max_allowed_val_);
  }

  std::cout << " passed " << passed_ << std::endl;

  return 0;
}
