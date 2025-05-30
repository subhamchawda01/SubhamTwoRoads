/**
    \file testbed/cpptest53.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
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

int main(int argc, char** argv) {
  if (argc < 2) {
    exit(0);
  }

  int num_secids_ = atoi(argv[1]);

#ifndef __APPLE__
  std::cout << num_secids_ << std::endl;
#else
  std::cout << -num_secids_ << std::endl;
#endif
}
