/**
   \file Tools/price_diff.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
 */

#include "baseinfra/Tools/common_smv_source.hpp"
#include <inttypes.h>

#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 22201225

class TestBook : public HFSAT::Thread, public HFSAT::SecurityMarketViewChangeListener {
  HFSAT::SecurityMarketView *this_smv_;
  HFSAT::Watch &watch_;

  /// 0 : keep changing book and showing current book
  /// 1 : event by event
  /// 2 : keep changing book and refreshing screen till the specified time is reached
  int mode_;
  int start_unix_time_;
  int end_unix_time_;
  int pause_mfm_;
  int max_levels_;
  bool stuck_for_go_;
  bool day_over_;
  int traded_volume_;
  int last_msecs_recorded_;
  int count_type_;
  int update_so_far_;
  bool started_;
  double start_px_;
  double end_px_;
  long last_logged_time = 0;

 public:
  TestBook(HFSAT::SecurityMarketView *_this_smv_, HFSAT::Watch &_watch_, const int r_max_levels_,
           const int start_unix_time_, const int end_unix_time_, const int _count_type_)
      : this_smv_(_this_smv_),
        watch_(_watch_),
        start_unix_time_(start_unix_time_),
        end_unix_time_(end_unix_time_),
        pause_mfm_(0),
        max_levels_(r_max_levels_),
        stuck_for_go_(false),
        day_over_(false),
        traded_volume_(0),
        count_type_(_count_type_),
        update_so_far_(0),
        started_(false),
        start_px_(0),
        end_px_(0)

  {
    this_smv_->ComputeMktPrice();
    // printf ( "\033[3;1H" );  // move to 3rd line //Rahul
    // printf ( "%2s %5s %3s %11s %7s X %7s %11s %3s %5s %2s\n", "BL", "BS", "BO", "BP", "BIP", "AIP", "AP", "AO", "AS",
    // "AL" );
  }

  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo &_market_update_info_) {
    if (update_so_far_ == 0 || (int)watch_.msecs_from_midnight() - last_logged_time >= 1000) {
      last_logged_time = watch_.msecs_from_midnight();
      std::cout << update_so_far_++ << " " << this_smv_->mid_price() << std::endl;
      last_msecs_recorded_ = watch_.msecs_from_midnight();
    }
  }

  void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo &_trade_print_info_,
                    const HFSAT::MarketUpdateInfo &_market_update_info_) {}

  void thread_main() {
    std::cout << this_smv_->min_price_increment() << std::endl;
    while (!day_over_) {
      sleep(1);
    }
  }

  void DayOver() {
    day_over_ = true;
    // double px_change_in_ticks_ =  ( end_px_ - start_px_ ) / this_smv_->min_price_increment( );
    // printf ( "%.2f\n", px_change_in_ticks_ );
  }

  void CheckBook() {
    if (this_smv_->IsError()) {
      //  printf ( "ERROR \n" );
      //  exit ( 0 );
    }
  }
};

int main(int argc, char **argv) {
  // Assume that we get the filename of a file that only has all logged EBS data pertaining to one symbol.
  // Load up the File Source

  if (argc < 4) {
    std::cerr << "expecting :\n"
              << " $EXEC SHORTCODE TRADINGDATE START_UNIX_TIME_STAMP END_UNIX_TIME_STAMP" << '\n';
    exit(0);
  }

  std::string _this_shortcode_ = argv[1];

  int tradingdate_ = 0;
  int max_levels_ = 15;

  // bool non_self_book_enabled_ = true;

  int start_unix_time_ = 0;
  int end_unix_time_ = 0;
  int count_type_ = 0;
  std::string network_account_info_filename_ = HFSAT::FileUtils::AppendHome(
      std::string(BASESYSINFODIR) + "TradingInfo/NetworkInfo/network_account_info_filename.txt");

  tradingdate_ = atoi(argv[2]);
  if ((tradingdate_ < MIN_YYYYMMDD) || (tradingdate_ > MAX_YYYYMMDD)) {
    std::cerr << tradingdate_ << " not logical date" << std::endl;
    exit(0);
  }
  start_unix_time_ = atoi(argv[3]);
  end_unix_time_ = atoi(argv[4]);

  std::vector<std::string> shortcode_list_;
  shortcode_list_.push_back(_this_shortcode_);

  // --- Make object of CommonSMVSource and use it as an API -------------------------------------------
  bool livetrading_ = false;
  CommonSMVSource *common_smv_source = new CommonSMVSource(shortcode_list_, tradingdate_, livetrading_);

  // Set the required book type and network info filename
  common_smv_source->SetNetworkAccountInfoFilename(network_account_info_filename_);

  // Initialize the smv source after setting the required variables
  common_smv_source->Initialize();

  // Get the smv and watch after creating the source
  HFSAT::SecurityMarketView *p_smv_ = common_smv_source->getSMV();
  HFSAT::Watch &watch_ = common_smv_source->getWatch();

  TestBook test_book_(p_smv_, watch_, max_levels_, start_unix_time_, end_unix_time_, count_type_);
  test_book_.run();

  // Subscribe yourself to the smv
  p_smv_->subscribe_price_type(&test_book_, HFSAT::kPriceTypeMktSizeWPrice);
  if (max_levels_ > 1) {
    p_smv_->subscribe_L2(&test_book_);
  }

  // start event loop : Runs historical dispatcher, callbacks of MktUpdate, and TradePrint
  common_smv_source->Run();

  test_book_.DayOver();

  // --------------------------------- API use over------------------------------------------------------

  test_book_.stop();
  return 0;
}
