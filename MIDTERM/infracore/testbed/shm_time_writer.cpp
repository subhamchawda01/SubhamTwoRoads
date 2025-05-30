/**
    \file testbed/shm_time_writer.hpp

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
#include <signal.h>

inline long long time_of_day() {
  timeval tv;
  gettimeofday(&tv, NULL);
  return (long long)tv.tv_sec * 1000000 + (long long)tv.tv_usec;
}

/// signal handler
void sighandler(int signum) {
  fprintf(stderr, "Received signal %d \n", signum);
  int shmid = shmget(1234, 0, 0);
  if (shmid != -1) {
    shmctl(shmid, IPC_RMID, 0);
    fprintf(stderr, "freed memory %d\n", shmid);
  }
  exit(0);
}

int main() {
  /// set signal handler .. add other signals later
  struct sigaction sigact;
  sigact.sa_handler = sighandler;
  sigaction(SIGINT, &sigact, NULL);

  int shmid = shmget(1234, 24, 0666 | IPC_CREAT);
  std::cout << "shmid" << shmid;
  long long* t;
  t = (long long*)shmat(shmid, 0, 0);
  while (true) {
    long long time = time_of_day();
    // std::cout<<"writing time "<<time << "\n";
    t[0] = time;
    usleep(33);
    t[1] = time;
    usleep(33);
    t[2] = time;
    usleep(33);
  }
  return 0;
}
