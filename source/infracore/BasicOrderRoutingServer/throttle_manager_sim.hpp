/**
    \file BasicOrderRoutingServer/throttle_manager.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */

#ifndef BASE_BASICORDERROUTINGSERVER_THROTTLEMANAGERSIM_H
#define BASE_BASICORDERROUTINGSERVER_THROTTLEMANAGERSIM_H

#include "dvccode/CDef/debug_logger.hpp"

//#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/lock.hpp"
#include "dvccode/CommonDataStructures/circular_buffer.hpp"
#include "infracore/BasicOrderRoutingServer/defines.hpp"
#include "dvccode/Utils/rdtsc_timer.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include <chrono>
#include <thread>
#define TROTTLE_DEBUG_LOGGING 0
#define TROTTLE_LAG_MULTIPLIER 1.05

namespace HFSAT {
namespace ORS {

/// Class that is used by Client thread to stop / manage how many orders
/// go through
class ThrottleManagerSIM {

 public:
  virtual bool reject_for_time_throttle(const ttime_t &this_timestamp) {
    int oldest_time_stamp_indx = (curr_indx_ == size_minus_1_) ? 0 : curr_indx_ + 1;

    if (oldest_time_stamp_indx >= size_) {
      std::ostringstream t_temp_oss;
      t_temp_oss << "Corruption In Throttle Manager : "
                 << " Current Access Index : " << oldest_time_stamp_indx << " Vector Size : " << size_ << "\n";

      DBGLOG_CLASS_FUNC_LINE_INFO << t_temp_oss.str() << DBGLOG_ENDL_FLUSH ;
//      EmailForMemoryCorruption(t_temp_oss.str());

      oldest_time_stamp_indx = 0;
    }

#if TROTTLE_DEBUG_LOGGING
    DBGLOG_CLASS_FUNC_LINE_INFO << "reject_for_time_throttle::this_timestamp: " << this_timestamp
                                << " throttle_timestamp_buffer_[oldest_time_stamp_indx]: "
                                << throttle_timestamp_buffer_[oldest_time_stamp_indx] << "\ndifference: "
                                << (this_timestamp - throttle_timestamp_buffer_[oldest_time_stamp_indx])
                                << "min_throttle_lag_: " << min_throttle_lag_ << DBGLOG_ENDL_FLUSH;
#endif
    if (this_timestamp - throttle_timestamp_buffer_[oldest_time_stamp_indx] > min_throttle_lag_) {
      curr_indx_ = oldest_time_stamp_indx;
      throttle_timestamp_buffer_[curr_indx_] = this_timestamp;
      min_throttle_wait.tv_sec=0;
      min_throttle_wait.tv_usec=0;
#if TROTTLE_DEBUG_LOGGING
      DBGLOG_CLASS_FUNC_LINE_INFO << "THROTTLE PASS: throttle_timestamp_buffer_[curr_indx_]:"
                                  << throttle_timestamp_buffer_[curr_indx_] << DBGLOG_ENDL_FLUSH;
#endif
      return false;
    }
    min_throttle_wait = (this_timestamp - throttle_timestamp_buffer_[oldest_time_stamp_indx]);
#if TROTTLE_DEBUG_LOGGING
    DBGLOG_CLASS_FUNC_LINE_INFO << "THROTTLE ERROR: throttle_timestamp_buffer_[curr_indx_]:"
                                << throttle_timestamp_buffer_[curr_indx_] << DBGLOG_ENDL_FLUSH;
#endif
    return true;
  }
  ttime_t min_cycle_throttle_wait(){
  	return min_throttle_wait;
  }

  void throttle_value_update(int new_throttle_size){
    size_ = new_throttle_size;
    size_minus_1_ = size_ - 1;
    curr_indx_ = 0;
    min_throttle_wait.tv_sec=0;
    min_throttle_wait.tv_usec=0;

    throttle_timestamp_buffer_ = (ttime_t*)calloc(new_throttle_size, sizeof(ttime_t));
    DBGLOG_CLASS_FUNC_LINE_INFO << "THROTTLE UPDATING. SIZE: " << size_ << " TIME: " << DBGLOG_ENDL_FLUSH;
    DBGLOG_CLASS_FUNC_LINE_INFO << "THROTTLE UPDATED. SIZE: " << size_ << " TIME: " << min_throttle_wait.ToString() << DBGLOG_ENDL_FLUSH;
  }

  // forward lookup whether placing no_of_orders can cause throttle or not
  // used only by ORS to place orders on itself when STC triggers
  // TODO assumes that all the orders are going to take place at the same timestamp
  virtual bool peek_reject_for_time_throttle(int no_of_orders_, const ttime_t &this_timestamp) {
    int peek_indx = curr_indx_ + no_of_orders_;
    if (peek_indx > size_minus_1_) {
      peek_indx = peek_indx % (size_minus_1_ +
                               1);  // peek_indx = peek_indx - (size_minus_1_+1), but we want to be sure hence modulo
    }

    if (peek_indx >= size_) {
      std::ostringstream t_temp_oss;
      t_temp_oss << "Corruption In Throttle Manager Peek Reject : "
                 << " Current Access Index : " << peek_indx << " Vector Size : " << size_ << "\n";

      DBGLOG_CLASS_FUNC_LINE_INFO << t_temp_oss.str() << DBGLOG_ENDL_FLUSH ;
//      EmailForMemoryCorruption(t_temp_oss.str());
    }
#if TROTTLE_DEBUG_LOGGING
    DBGLOG_CLASS_FUNC_LINE_INFO << "peek_reject_for_time_throttle::this_timestamp: " << this_timestamp
                                << " throttle_timestamp_buffer_[peek_indx]: " << throttle_timestamp_buffer_[peek_indx]
                                << "\ndifference: " << (this_timestamp - throttle_timestamp_buffer_[peek_indx])
                                << "min_throttle_lag_: " << min_throttle_lag_ << DBGLOG_ENDL_FLUSH;
#endif

    if (this_timestamp - throttle_timestamp_buffer_[peek_indx] > min_throttle_lag_) {
// we are free to send message
#if TROTTLE_DEBUG_LOGGING
      DBGLOG_CLASS_FUNC_LINE_INFO << "THROTTLE PASS: throttle_timestamp_buffer_[peek_indx]:"
                                  << throttle_timestamp_buffer_[peek_indx] << DBGLOG_ENDL_FLUSH;
#endif
      return false;  // do not reject
    }

#if TROTTLE_DEBUG_LOGGING
    DBGLOG_CLASS_FUNC_LINE_INFO << "THROTTLE ERROR: throttle_timestamp_buffer_[peek_indx]:"
                                << throttle_timestamp_buffer_[peek_indx] << DBGLOG_ENDL_FLUSH;
#endif
    return true;  // reject
  }

  ThrottleManagerSIM(DebugLogger& dbg, int t_default_size = THROTTLE_MSG_LIMIT, bool use_lock = true)
      : dbglogger_(dbg),
        size_(t_default_size),
        curr_indx_(0),
        size_minus_1_(t_default_size - 1),
        throttle_timestamp_buffer_((ttime_t*)calloc(t_default_size, sizeof(ttime_t))),
        min_throttle_lag_(),
        min_throttle_wait() {

        // TROTTLE_LAG_MULTIPLIER = 1.05, signifies 1050 milliseconds.
        min_throttle_lag_.tv_sec=1;
        min_throttle_lag_.tv_usec=05000;

    DBGLOG_CLASS_FUNC_LINE_INFO << "THROTTLE min_throttle_lag_: " << min_throttle_lag_.ToString() << DBGLOG_ENDL_FLUSH;
  }

 private:
  ThrottleManagerSIM(const ThrottleManagerSIM&);  // no-copy
  // CircularBuffer <long long unsigned int> throttle_timestamp_buffer_;
  DebugLogger& dbglogger_;
  int size_;
  int curr_indx_;
  int size_minus_1_;

  ttime_t* throttle_timestamp_buffer_;

  //! This field is machine dependent(number of CPU cycles per second).
  ttime_t min_throttle_lag_;
  ttime_t min_throttle_wait;
};
}
}

#endif  // BASE_BASICORDERROUTINGSERVER_THROTTLEMANAGER_H
