/**
    \file testbed/cpptest12.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include <stdio.h>
#include <iostream>
#include <ctime>
#include <time.h>

#define LEN 10240

typedef unsigned long long ticks;
static __inline__ ticks GetTicks(void) {
  unsigned a, d;
  // asm volatile("cpuid":"=a"(a),"=d"(d):"0"(0):"ecx","ebx");
  asm volatile("rdtsc" : "=a"(a), "=d"(d));
  return ((ticks)a) | (((ticks)d) << 32);
}

typedef unsigned long long stime_t;

struct ttime_t {
  union {
    struct {
      unsigned int t_sec;
      unsigned int t_usec;
    };
    unsigned long long val;
  };
};

int main() {
  timeval tv_;
  time_t tv_sec_ = 0;
  stime_t tv_stt = 0;
  ttime_t tv_ttt;
  int msecs_fm1, msecs_fm2;
  stime_t justusec_ = 0xffffffff;
  struct tm today_midnight_;
  struct tm* _p_today_now_;

  ticks p1, p2, p3, p4, p5, p6;
  // std::cout << sizeof(ttime_t) << std::endl;
  tv_ttt.val = 0;
  time(&tv_sec_);
  _p_today_now_ = localtime(&tv_sec_);
  today_midnight_ = *_p_today_now_;
  today_midnight_.tm_sec = 0;
  today_midnight_.tm_min = 0;
  today_midnight_.tm_hour = 0;
  tv_sec_ = mktime(&today_midnight_);

  p1 = GetTicks();
  for (unsigned int i = 0; i < LEN; i++) {
    tv_.tv_sec = 0;
    tv_.tv_usec = 0;
    gettimeofday(&tv_, NULL);
  }
  p2 = GetTicks();

  //#define USING_SST
  for (unsigned int i = 0; i < LEN; i++) {
    tv_.tv_sec = 0;
    tv_.tv_usec = 0;
    gettimeofday(&tv_, NULL);
// 155
#ifdef USING_SST
    tv_stt = (tv_.tv_sec << 32) | tv_.tv_usec;
    // 1
    msecs_fm1 = (((tv_stt >> 32) - tv_sec_) * 1000) + ((tv_stt & justusec_) / 1000);
// 30
#else
    tv_ttt.val = (tv_.tv_sec) | (tv_.tv_usec << 32);  // same as tv_ttt.t_sec = tv_.tv_sec; tv_ttt.t_usec = tv_.tv_usec;
    // 1
    msecs_fm2 = ((tv_ttt.t_sec - tv_sec_) * 1000) + (tv_ttt.t_usec / 1000);
// 25
#endif
  }
#undef USING_SST
  p3 = GetTicks();

  // printf ( "\n tv_ %ld %ld \n tv_ttt %d %d %d \n tv_stt %ld %ld %d\n", tv_.tv_sec, tv_.tv_usec, tv_ttt.t_sec,
  // tv_ttt.t_usec, msecs_fm2, (tv_stt >>32 ), ( tv_stt & justusec_ ), msecs_fm1) ;
  std::cout << " "
            << " " << (p2 - p1) / LEN << " " << (p3 - p2) / LEN
            // 	    << " " << (p4-p3)/LEN
            // 	    << " " << (p5-p4)/LEN
            // 	    << " " << (p6-p5)/LEN
            << std::endl;

  return 0;
}
