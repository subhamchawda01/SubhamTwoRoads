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
#include "dvccode/CDef/math_utils.hpp"

class TestBook : public HFSAT::Thread, public HFSAT::SecurityMarketViewChangeListener {
  const HFSAT::SecurityMarketView &this_smv_;
  HFSAT::Watch &watch_;

  /// 0 : keep changing book and showing current book
  /// 1 : event by event
  /// 2 : keep changing book and refreshing screen till the specified time is reached
  bool day_over_;

  int start_unix_time_;
  int end_unix_time_;
  bool per_sec_;
  int last_update_sec_;
  int last_update_usec_;
  int next_period_msecs_;
  int last_period_cumulative_count_;
  int div_factor_;  // We are returning time in HHMMSS. For case in which
                    // don't want to print SS (if we are printing per minute only) ,
                    // we have to divide the value by 100
                    // or else by 10000 if we are printing per hour only.
                    // This factor store that factor.

 public:
  int total_size_;
  int total_count_;
  int period_msecs_;
  double period_secs_;

 public:
  TestBook(const HFSAT::SecurityMarketView &_this_smv_, HFSAT::Watch &_watch_, const int t_start_unix_time_ = 0,
           const int t_end_unix_time_ = 0, const bool t_per_sec_ = 0, const double t_period_secs_ = 15 * 60,
           const int t_print_secs_ = false)
      : this_smv_(_this_smv_),
        watch_(_watch_),
        day_over_(false),
        start_unix_time_(t_start_unix_time_),
        end_unix_time_(t_end_unix_time_),
        per_sec_(t_per_sec_),
        last_update_sec_(-1),
        last_update_usec_(-1),
        next_period_msecs_(0),
        last_period_cumulative_count_(0),
        div_factor_(100000),
        total_size_(0),
        total_count_(0),
        period_msecs_(t_period_secs_ * 1000),
        period_secs_(t_period_secs_) {
    if (t_print_secs_ == 1) {
      div_factor_ = 1000;
    } else if (t_print_secs_ == 2) {
      div_factor_ = 1;
    }
  }

  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo &_market_update_info_) {
    if (watch_.tv().tv_sec % 86400 > start_unix_time_ && watch_.tv().tv_sec % 86400 < end_unix_time_ &&
        !(last_update_sec_ == watch_.tv().tv_sec && last_update_usec_ == watch_.tv().tv_usec)) {
      if (this_smv_.bestask_size() > 0 && this_smv_.bestbid_size() > 0) {
        if (watch_.msecs_from_midnight() > next_period_msecs_) {
          if (next_period_msecs_ > 0) {
            if (per_sec_) {
              double avg_events_per_sec_ = (total_count_ - last_period_cumulative_count_) / (double)period_secs_;
              std::cout << GetHHMMSSLLLfromMsecs(next_period_msecs_) / div_factor_ << " " << avg_events_per_sec_
                        << std::endl;
            } else {
              std::cout << GetHHMMSSLLLfromMsecs(next_period_msecs_) / div_factor_ << " "
                        << (total_count_ - last_period_cumulative_count_) << std::endl;
            }
          }
          next_period_msecs_ = HFSAT::MathUtils::GetCeilMultipleOf(watch_.msecs_from_midnight(), period_msecs_);
          last_period_cumulative_count_ = total_count_;
        }
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

  void DayOver() {
    // if data has been received for more than 2/3 of the period for the last unattended sample
    if (total_count_ > last_period_cumulative_count_ &&
        ((next_period_msecs_ / 1000) - (last_update_sec_ % 86400)) < period_secs_ / 3) {
      if (per_sec_) {
        double t_period_secs_ = period_secs_ - ((next_period_msecs_ / 1000) - (last_update_sec_ % 86400));
        double avg_events_per_sec_ = (total_count_ - last_period_cumulative_count_) / (double)t_period_secs_;
        std::cout << GetHHMMSSLLLfromMsecs(next_period_msecs_) / div_factor_ << " " << avg_events_per_sec_ << std::endl;
      } else {
        std::cout << GetHHMMSSLLLfromMsecs(next_period_msecs_) / div_factor_ << " "
                  << (total_count_ - last_period_cumulative_count_) << std::endl;
      }
      last_period_cumulative_count_ = total_count_;
    }
    day_over_ = true;
  }

  int GetHHMMSSLLLfromMsecs(int msecs_) {
    int hrs_ = (int)(msecs_ / 3600000);
    int mins_ = (int)((msecs_ % 3600000) / 60000);
    int secs_ = (int)((msecs_ % 60000) / 1000);
    int millisecs_ = (int)(msecs_%1000);
    return (hrs_ * 10000000 + mins_ * 100000 + secs_*1000 + millisecs_);
  }
};

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " shortcode input_date_YYYYMMDD [PerSec[0/1] (default:0)] [ start_tm_utc_hhmm "
                                         "] [ end_tm_utc_hhmm ] [PeriodSecs] [PrintSecs (0/1)]" << std::endl;
    exit(0);
  }

  std::string _this_shortcode_ = argv[1];
  int tradingdate_ = atoi(argv[2]);
  int start_unix_time_ = 0;
  int end_unix_time_ = 24 * 60 * 60;
  bool per_sec_ = 0;
  double period_secs_ = 15 * 60;
  int print_secs_ = 0;

  if (argc > 3) {
    if (atoi(argv[3]) > 0) {
      per_sec_ = 1;
    }
  }
  if (argc > 4) {
    start_unix_time_ = (atoi(argv[4]) / 100) * 60 * 60 + (atoi(argv[4]) % 100) * 60;
  }
  if (argc > 5) {
    end_unix_time_ = (atoi(argv[5]) / 100) * 60 * 60 + (atoi(argv[5]) % 100) * 60;
  }
  if (argc > 6) {
    period_secs_ = (atof(argv[6]));
  }
  if (argc > 7) {
    print_secs_ = (atoi(argv[7]));;
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

  TestBook test_book_(*p_smv_, watch_, start_unix_time_, end_unix_time_, per_sec_, period_secs_, print_secs_);
  test_book_.run();

  // Subscribe yourself to the smv
  p_smv_->subscribe_L1_Only(&test_book_);

  // start event loop : Runs historical dispatcher, callbacks of MktUpdate, and TradePrint
  common_smv_source->Run();

  test_book_.DayOver();

  test_book_.stop();
  return 0;
}
