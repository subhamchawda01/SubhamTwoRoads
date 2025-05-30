/**
   \file dvccode/Utils/ors_rejection_alert_thread.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#pragma once

#include <iostream>
#include <sys/time.h>
#include <sstream>
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/send_alert.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/Utils/lock.hpp"
#include <map>
#include <string.h>

namespace HFSAT {

// @brief a thread to send alert on ORS_Reject, will aggregate the rejection messages if we have already sent an alert
// within last minute.

class ORSRejectionAlerts : public HFSAT::Thread {
#define NORMAL_SLEEP_TIME 10000        // 1 millisec
#define SLEEP_TIME_AFTER_LAST_ALERT 3  // 3 seconds

  std::map<std::string, int>
      symbol_reason_id;            // map to store concatenated symbol and ORS rejection reason, and assign a uniq id
  std::map<int, int> alert_count;  // count to store number of alerts of that id
  bool stop_thread;
  bool has_alert;  // variable to quickly identify if an alert is reported or not

  std::map<std::string, int> self_trade_count;
  int32_t last_stc_summary_time;

  HFSAT::Lock lock;

 public:
  ORSRejectionAlerts()
      : symbol_reason_id(),
        alert_count(),
        stop_thread(false),
        has_alert(false),
        self_trade_count(),
        last_stc_summary_time(0),
        lock(Lock()) {}

  void stopThread() { stop_thread = true; }

  void flushAlerts() {
    std::stringstream ss;
    std::map<std::string, int>::iterator iter;

    lock.LockMutex();
    for (iter = symbol_reason_id.begin(); has_alert && iter != symbol_reason_id.end(); iter++) {
      int id = iter->second;
      if (alert_count[id] > 0) {
        if (alert_count[id] > 1)
          ss << "ORS rejection alert: " << iter->first << "( " << alert_count[id] << " more... )\n";
        else
          ss << "ORS rejection alert: " << iter->first << "\n";
      }
      alert_count[id] = 0;
    }
    has_alert = false;

    lock.UnlockMutex();

    //      std::cout << ss.str() << "\n";
    HFSAT::SendAlert::sendAlert(ss.str());
    sleep(SLEEP_TIME_AFTER_LAST_ALERT);
  }

  void summarizeSelfTrade() {
    std::stringstream ss;
    timeval tv;
    gettimeofday(&tv, NULL);
    //      bool send_stc_summry = false;
    if (last_stc_summary_time + 900 > tv.tv_sec)  // 15 mins = 900 secs
      return;

    lock.LockMutex();
    //      send_stc_summry = true;
    last_stc_summary_time = tv.tv_sec;

    bool zero_self_trade = true;
    std::map<std::string, int>::iterator iter;
    for (iter = self_trade_count.begin(); iter != self_trade_count.end(); iter++) {
      if (iter->second > 0) {
        if (iter->second > 1)
          ss << "ORS Self Trade for " << iter->first << " ( " << iter->second << " times )\n";
        else
          ss << "ORS Self Trade for " << iter->first << "\n";
        zero_self_trade = false;
        self_trade_count[iter->first] = 0;  // reset to 0
      }
    }
    lock.UnlockMutex();

    if (zero_self_trade)
      last_stc_summary_time =
          0;  // this is done so that we dont wait for 15 minutes before looking for self trade rejects
    else {
      //        std::cout << ss.str() << "\n";
      HFSAT::SendAlert::sendAlert(ss.str());
      sleep(SLEEP_TIME_AFTER_LAST_ALERT);
    }
  }

  void thread_main() {
    while (!stop_thread) {
      if (has_alert) {
        flushAlerts();
      }
      summarizeSelfTrade();
      usleep(NORMAL_SLEEP_TIME);
    }
  }

  void addToRejectionAlertQueue(HFSAT::ORSRejectionReason_t _rejection_reason_, std::string symbol_) {
    lock.LockMutex();

    if (kORSRejectSelfTradeCheck == _rejection_reason_) {
      if (self_trade_count.find(symbol_) == self_trade_count.end()) self_trade_count[symbol_] = 0;

      self_trade_count[symbol_]++;
      lock.UnlockMutex();
      return;
    }

    std::string symbol_reason = std::string(ORSRejectionReasonStr(_rejection_reason_)) + " for " + symbol_;
    int id = -1;
    if (symbol_reason_id.find(symbol_reason) == symbol_reason_id.end()) {
      id = symbol_reason_id.size();
      symbol_reason_id[symbol_reason] = id;
      alert_count[id] = 0;
    } else
      id = symbol_reason_id[symbol_reason];
    alert_count[id]++;
    has_alert = true;

    lock.UnlockMutex();
  }
};
}
