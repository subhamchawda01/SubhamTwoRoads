/**
    \file dvccode/Profiler/tick_to_trade_profiler.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#ifndef BASE_TICK_TO_TRADE_PROFILER_H
#define BASE_TICK_TO_TRADE_PROFILER_H

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"

namespace HFSAT {
class TickToTradeProfiler {
 public:
  static TickToTradeProfiler &GetUniqueInstance() {
    static TickToTradeProfiler *p_unique_instance_ = NULL;
    if (!p_unique_instance_) {
      p_unique_instance_ = new TickToTradeProfiler();
    }

    return *p_unique_instance_;
  }

  static inline unsigned long GetCurrentCpuCycles() {
    unsigned a, d;
    asm volatile("rdtsc" : "=a"(a), "=d"(d));
    return ((unsigned long)a) | (((unsigned long)d) << 32);
  }

  inline void pushLastTime(HFSAT::ttime_t _last_time_, unsigned long _last_cycles_) {
    last_time_ = _last_time_;
    last_cycles_ = _last_cycles_;
  }
  /*
      inline void pushLastCycleCount (unsigned long _last_cycles_) {
        last_cycles_ = _last_cycles_;
      }
  */
  inline HFSAT::ttime_t popLastTime() { return last_time_; }

  inline unsigned long popLastCycleCount() { return last_cycles_; }

  inline int getOrderCount() { return ++orderCount_; }

 private:
  HFSAT::ttime_t last_time_;
  unsigned long last_cycles_;
  int orderCount_;
  TickToTradeProfiler() : last_time_(0, 0), last_cycles_(0), orderCount_(0) {}
};  // TickToTradeProfiler
};  // HFSAT

#endif
