/**
    \file testbed/cpptest11.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

#include <iostream>
#include <sys/time.h>
#include <time.h>

typedef unsigned long long ticks;

static __inline__ ticks GetTicks(void) {
  unsigned a, d;
  // asm volatile("cpuid":"=a"(a),"=d"(d):"0"(0):"ecx","ebx");
  asm volatile("rdtsc" : "=a"(a), "=d"(d));
  return ((ticks)a) | (((ticks)d) << 32);
}

typedef unsigned long long ttime_t;
struct itime_t {
  int i_sec_;
  int i_usec_;

  itime_t& operator=(const timeval& other) {
    i_sec_ = other.tv_sec;
    i_usec_ = other.tv_usec;
    return *this;
  }
};

int main() {
  std::cout << " s(int): " << sizeof(int) << std::endl
            << " s(uint): " << sizeof(unsigned int) << std::endl
            << " s(size_t): " << sizeof(size_t) << std::endl
            << " s(long): " << sizeof(long) << std::endl
            << " s(ulong): " << sizeof(unsigned long) << std::endl
            << " s(uint64_t): " << sizeof(unsigned long long int) << std::endl
            << " s(double): " << sizeof(double) << std::endl
            << " s(timeval): " << sizeof(timeval) << std::endl
            << " s(time_t): " << sizeof(time_t) << std::endl
            << " s(suseconds_t): " << sizeof(suseconds_t) << std::endl
            << " s(itime_t): " << sizeof(itime_t) << std::endl
            << std::endl;

  //  suseconds_t tv_usec_;
  timeval tv_;
  ttime_t tv_ttt;
  time_t tv_sec_ = 0;
  timeval tv2_;
  ttime_t tv2_ttt;
  itime_t itv_;

  ticks p0;
  p0 = GetTicks();  // start
  // for ( unsigned int i = 0 ; i < 1000 ; i ++ ) {
  // tv_.tv_sec = 0;
  // tv_.tv_usec = 0;
  // }
  ticks p1 = GetTicks();  // start
  for (auto i = 0u; i < 1000; i++) {
    tv_.tv_sec = 0;
    tv_.tv_usec = 0;
  }
  ticks p2 = GetTicks();  //   0 cycs
  for (auto i = 0u; i < 1000; i++) {
    gettimeofday(&tv_, NULL);
  }
  ticks p3 = GetTicks();  // 135 cycs
  for (auto i = 0u; i < 1000; i++) {
    tv_ttt = (tv_.tv_sec << 32) | tv_.tv_usec;
  }
  ticks p4 = GetTicks();  //   0 cycs
  for (auto i = 0u; i < 1000; i++) {
    time(&tv_sec_);
  }
  ticks p5 = GetTicks();  // 133 cycs
  for (auto i = 0u; i < 1000; i++) {
    itv_ = tv_;
  }
  ticks p6 = GetTicks();  // 7 cycs
  // sleep(0);
  // ticks p6=GetTicks(); // 4268 cycs

  tv2_.tv_sec = 0;
  tv2_.tv_usec = 0;
  gettimeofday(&tv2_, NULL);

  tv2_ttt = (tv2_.tv_sec << 32) | tv2_.tv_usec;

  std::cout << "cycles: " << (p1 - p0) << " " << (p2 - p1) << " " << (p3 - p2) << " " << (p4 - p3) << " " << (p5 - p4)
            << " " << (p6 - p5) << std::endl;
  //  std::cout << "sleep ( 0 ) cycles: " << (p6-p5) << std::endl;

  std::cout << "From gettimeofday() " << tv_.tv_sec << "." << tv_.tv_usec << " ttime_t " << tv_ttt << std::endl;
  std::cout << "From gettimeofday() " << tv2_.tv_sec << "." << tv2_.tv_usec << " ttime_t " << tv2_ttt << std::endl;
  std::cout << "From time() " << tv_sec_ << " ctime () " << ctime(&tv_sec_) << std::endl;

  struct tm today_midnight_;
  struct tm* _p_today_now_;
  ticks q1 = GetTicks();  // start
  for (auto i = 0u; i < 1000; i++) {
    _p_today_now_ = localtime(&tv_sec_);
  }
  ticks q2 = GetTicks();  // 4400 cycs
  for (auto i = 0u; i < 1000; i++) {
    today_midnight_ = *_p_today_now_;
  }
  ticks q3 = GetTicks();  //    6 cycs
  for (auto i = 0u; i < 1000; i++) {
    today_midnight_.tm_sec = 0;
    today_midnight_.tm_min = 0;
    today_midnight_.tm_hour = 0;
  }
  ticks q4 = GetTicks();  //    3 cycs
  for (auto i = 0u; i < 1000; i++) {
    tv_sec_ = mktime(&today_midnight_);
  }
  ticks q5 = GetTicks();  //  5228 cycs
  std::cout << "From localtime() - midnight " << tv_sec_ << " ctime () " << ctime(&tv_sec_) << std::endl;

  std::cout << "cycles: " << (q2 - q1) << " " << (q3 - q2) << " " << (q4 - q3) << " " << (q5 - q4) << std::endl;

  return 0;
}
