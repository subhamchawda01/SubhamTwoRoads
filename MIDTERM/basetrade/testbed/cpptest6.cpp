/**
    \file testbed/cpptest6.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include <stdio.h>
#include <sys/time.h>

int main() {
  printf("%d %d %d %d\n", sizeof(unsigned long long), sizeof(int), sizeof(long), sizeof(long long));
  return 0;
}
