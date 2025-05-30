/**
   \file Tools/get_avg_l1sz_on_day.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
*/
#ifndef GET_AVG_L1SZ_ON_DAY_HPP
#define GET_AVG_L1SZ_ON_DAY_HPP

#include <inttypes.h>
#include "baseinfra/Tools/common_smv_source.hpp"

class TestBook : public HFSAT::Thread, public HFSAT::SecurityMarketViewChangeListener {
  const HFSAT::SecurityMarketView &this_smv_;
  HFSAT::Watch &watch_;

  /// 0 : keep changing book and showing current book
  /// 1 : event by event
  /// 2 : keep changing book and refreshing screen till the specified time is reached
  bool day_over_;

  int start_unix_time_;
  int end_unix_time_;

 public:
  TestBook(const HFSAT::SecurityMarketView &_this_smv_, HFSAT::Watch &_watch_, const int t_start_unix_time_ = 0,
           const int t_end_unix_time_ = 0)
      : this_smv_(_this_smv_),
        watch_(_watch_),
        day_over_(false),
        start_unix_time_(t_start_unix_time_),
        end_unix_time_(t_end_unix_time_),
        total_size_(0),
        total_count_(0) {}

  uint64_t total_size_;
  int total_count_;

  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo &_market_update_info_) {
    if (watch_.tv().tv_sec % 86400 > start_unix_time_ && watch_.tv().tv_sec % 86400 < end_unix_time_) {
      if (this_smv_.bestask_size() > 0 && this_smv_.bestbid_size() > 0) {
        total_size_ += (this_smv_.bestask_size() + this_smv_.bestbid_size()) / 2;
        total_count_++;
      }
    }
  }

  inline void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo &_trade_print_info_,
                           const HFSAT::MarketUpdateInfo &_market_update_info_) {
    // OnMarketUpdate ( _security_id_, _market_update_info_ );
  }

  void thread_main() {
    while (!day_over_) {
      sleep(1);
    }
  }

  void DayOver() { day_over_ = true; }
};

int getAvgL1Sz(std::string shortcode_, int tradingdate_, int begin_secs_from_midnight_, int end_secs_from_midnight_) {
  std::vector<std::string> shortcode_list_;
  shortcode_list_.push_back(shortcode_);

  // --- Make object of CommonSMVSource and use it as an API -------------------------------------------
  CommonSMVSource *common_smv_source = new CommonSMVSource(shortcode_list_, tradingdate_);

  // Initialize the smv source after setting the required variables
  common_smv_source->Initialize();

  // Get the smv and watch after creating the source
  HFSAT::SecurityMarketView *p_smv_ = common_smv_source->getSMV();
  HFSAT::Watch &watch_ = common_smv_source->getWatch();

  TestBook test_book_(*p_smv_, watch_, begin_secs_from_midnight_, end_secs_from_midnight_);
  test_book_.run();

  // Subscribe yourself to the smv
  p_smv_->subscribe_L1_Only(&test_book_);

  // start event loop : Runs historical dispatcher, callbacks of MktUpdate, and TradePrint
  common_smv_source->Run();

  test_book_.DayOver();

  test_book_.stop();
  int avg_l1_sz_ = (test_book_.total_count_ >= 1) ? (test_book_.total_size_ / test_book_.total_count_) : 0;

  return avg_l1_sz_;
}

#endif
