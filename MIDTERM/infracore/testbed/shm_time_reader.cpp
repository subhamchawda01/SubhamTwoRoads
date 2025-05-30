/**
    \file testbed/shm_time_reader.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

*/
#include <ctime>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>

#include <iostream>

int e1 = 0;
long long *t;

inline long long time_of_day() {
  // lets say b was assigned when t[1] was simultaneously being written.
  // so b is perhaps corrupt.
  // so it can not match t[0]
  // it can match t[2] if t[2] was corrupted bcause at the time of comparision, t[2] was being written onto.
  // the final condition of t[1] == b ensures that t[1] has not changed ever since.
  long long b = t[1];
  if ((t[0] == b || t[2] == b) && t[1] == b) return b;
  e1++;

  // fall back to default or may be we can try the above trick once more,
  // but since this failure is already rare, less than 3 in a million falling back to default is justified

  timeval tv;
  gettimeofday(&tv, NULL);
  return (long long)tv.tv_sec * 1000000 + (long long)tv.tv_usec;
}

inline long long slow_time_of_day() {
  timeval tv;
  gettimeofday(&tv, NULL);
  return (long long)tv.tv_sec * 1000000 + (long long)tv.tv_usec;
}

int main() {
  int shmid = shmget(1234, 0, 0);
  std::cout << "shmid" << shmid << "\n";

  t = (long long *)shmat(shmid, 0, 0);

  {
    long long sum = 0;
    long long start = time_of_day();
    int num_iter = 1000 * 1000 * 1000;
    for (int i = 0; i < num_iter; i++) {
      long long time = time_of_day();
      sum += time;
      // std::cout<<"time read " << time <<"\n";
    }
    long long end = time_of_day();
    std::cout << " time_taken " << (end - start) * 1000 / num_iter << "ns";
    std::cout << " \n num error " << e1 << " " << e2 << " " << e3 << "\n";
    std::cout << "\n junk: " << sum << "\n";
  }

  {
    long long sum = 0;
    long long start = slow_time_of_day();
    int num_iter = 1000 * 1000;
    for (int i = 0; i < num_iter; i++) {
      long long time = slow_time_of_day();
      sum += time;
      // std::cout<<"time read " << time <<"\n";
    }
    long long end = slow_time_of_day();
    std::cout << " time_taken " << (end - start) * 1000 / num_iter << "ns";
    std::cout << "\n junk: " << sum << "\n";
  }
  return 0;
}
