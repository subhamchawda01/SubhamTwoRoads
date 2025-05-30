/**
   \file Tools/get_l1events_on_day.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
 */

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
  int last_update_sec_;
  int last_update_usec_;

 public:
  int total_size_;
  int total_count_;

 public:
  TestBook(const HFSAT::SecurityMarketView &_this_smv_, HFSAT::Watch &_watch_, const int t_start_unix_time_ = 0,
           const int t_end_unix_time_ = 0)
      : this_smv_(_this_smv_),
        watch_(_watch_),
        day_over_(false),
        start_unix_time_(t_start_unix_time_),
        end_unix_time_(t_end_unix_time_),
        last_update_sec_(-1),
        last_update_usec_(-1),
        total_size_(0),
        total_count_(0) {}

  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo &_market_update_info_) {
    if (watch_.tv().tv_sec % 86400 > start_unix_time_ && watch_.tv().tv_sec % 86400 < end_unix_time_ &&
        !(last_update_sec_ == watch_.tv().tv_sec && last_update_usec_ == watch_.tv().tv_usec)) {
      if (this_smv_.bestask_size() > 0 && this_smv_.bestbid_size() > 0) {
        total_size_ += (this_smv_.bestask_size() + this_smv_.bestbid_size()) / 2;
        total_count_++;
        last_update_sec_ = watch_.tv().tv_sec;
        last_update_usec_ = watch_.tv().tv_usec;
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

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " shortcode input_date_YYYYMMDD [ start_tm_utc_hhmm ] [ end_tm_utc_hhmm ]"
              << std::endl;
    exit(0);
  }

  std::string _this_shortcode_ = argv[1];
  int tradingdate_ = atoi(argv[2]);
  int start_unix_time_ = 0;
  int end_unix_time_ = 24 * 60 * 60;

  if (argc > 3) {
    start_unix_time_ = (atoi(argv[3]) / 100) * 60 * 60 + (atoi(argv[3]) % 100) * 60;
  }
  if (argc > 4) {
    end_unix_time_ = (atoi(argv[4]) / 100) * 60 * 60 + (atoi(argv[4]) % 100) * 60;
  }

  std::vector<std::string> shortcode_list_;
  shortcode_list_.push_back(_this_shortcode_);

  // --- Make object of CommonSMVSource and use it as an API -------------------------------------------
  CommonSMVSource *common_smv_source = new CommonSMVSource(shortcode_list_, tradingdate_);

  // Initialize the smv source after setting the required variables
  common_smv_source->Initialize();

  // Get the smv and watch after creating the source
  HFSAT::SecurityMarketView *p_smv_ = common_smv_source->getSMV();
  HFSAT::Watch &watch_ = common_smv_source->getWatch();

  TestBook test_book_(*p_smv_, watch_, start_unix_time_, end_unix_time_);
  test_book_.run();

  // Subscribe yourself to the smv
  p_smv_->subscribe_L1_Only(&test_book_);

  // start event loop : Runs historical dispatcher, callbacks of MktUpdate, and TradePrint
  common_smv_source->Run();

  test_book_.DayOver();

  const char *exchange_symbol_ = common_smv_source->getExchangleSymbol();

  // --------------------------------- API use over------------------------------------------------------

  // to replace spaces in security name by ~
  char print_secname_[24] = {0};
  strcpy(print_secname_, exchange_symbol_);
  for (size_t i = 0; i < strlen(print_secname_); ++i) {
    if (print_secname_[i] == ' ') {  // Liffe symbols have space, which is bad for the post processing script
      print_secname_[i] = '~';
    }
  }

  // Display the results
  if (test_book_.total_count_ < 1) {
    std::cout << _this_shortcode_ << ' ' << print_secname_ << ' ' << 0 << std::endl;
  } else {
    std::cout << _this_shortcode_ << ' ' << print_secname_ << ' ' << test_book_.total_count_ << std::endl;
    // std::cout << test_book_.total_count_ << std::endl;
  }

  test_book_.stop();
  return 0;
}
