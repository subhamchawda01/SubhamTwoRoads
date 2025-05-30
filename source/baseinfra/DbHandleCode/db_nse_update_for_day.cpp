/**
   \file DbHandleCode/db_nse_update_for_day.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
*/
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "baseinfra/Tools/common_smv_source.hpp"
#include "baseinfra/MinuteBar/db_update_nse.hpp"

#include <inttypes.h>

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
        min_px_(1000000.0),
        max_px_(0.0),
        avg_px_(0.0),
        avg_px2_(0.0),
        avg_px_trades_(0.0),
        volume_weighted_avg_px_(0.0),
        volume_weighted_avg_px2_(0.0),
        total_volume_(0),
        total_trades_(0),
        total_buy_volume_(0),
        total_sell_volume_(0),
        total_size_(0),
        total_count_(0),
        min_trade_sz_(1000000.0),
        max_trade_sz_(0),
        avg_trade_sz_(0),
        max_spread_(0),
        min_spread_(10000000.0),
        avg_spread_(0) {
    this_smv_.ComputeMidPrice();
    for (int i = 0; i < 2024; i++) {
      ag_max_px_[i] = ag_avg_px_[i] = ag_avg_px2_[i] = ag_avg_px_trades_[i] = ag_volume_weighted_avg_px2_[i] =
          ag_volume_weighted_avg_px_[i] = 0.0;
      ag_total_volume_[i] = ag_total_trades_[i] = ag_total_buy_volume_[i] = ag_total_sell_volume_[i] =
          ag_total_size_[i] = ag_total_count_[i] = 0;
      ag_min_trade_sz_[i] = ag_max_trade_sz_[i] = ag_avg_trade_sz_[i] = ag_max_spread_[i] = ag_avg_spread_[i] =
          ag_min_px_increment_[i] = ag_price_stdev_[i] = 0;
      ag_min_px_[i] = 1000000.0;
      ag_min_trade_sz_[i] = 1000000.0;
      ag_min_spread_[i] = 10000000.0;
    }
  }
  double min_px_;
  double max_px_;
  double avg_px_;
  double avg_px2_;
  double avg_px_trades_;
  double volume_weighted_avg_px_;
  double volume_weighted_avg_px2_;
  int total_volume_;
  int total_trades_;
  int total_buy_volume_;
  int total_sell_volume_;
  uint64_t total_size_;
  int total_count_;
  double min_trade_sz_;
  double max_trade_sz_;
  double avg_trade_sz_;
  double max_spread_;
  double min_spread_;
  double avg_spread_;
  double min_px_increment_;
  double price_stdev_;

  double ag_min_px_[2024];
  double ag_max_px_[2024];
  double ag_avg_px_[2024];
  double ag_avg_px2_[2024];
  double ag_avg_px_trades_[2024];
  double ag_volume_weighted_avg_px_[2024];
  double ag_volume_weighted_avg_px2_[2024];
  int ag_total_volume_[2024];
  int ag_total_trades_[2024];
  int ag_total_buy_volume_[2024];
  int ag_total_sell_volume_[2024];
  uint64_t ag_total_size_[2024];
  int ag_total_count_[2024];
  double ag_min_trade_sz_[2024];
  double ag_max_trade_sz_[2024];
  double ag_avg_trade_sz_[2024];
  double ag_max_spread_[2024];
  double ag_min_spread_[2024];
  double ag_avg_spread_[2024];
  double ag_min_px_increment_[2024];
  double ag_price_stdev_[2024];
  int ag_tv_sec[2024];

  int getIndexFromSec(int seconds) {
    int index = (seconds - start_unix_time_) / 60;  // start unix time is day start time
    return index;
  }
  int getIndex(int timestamp) {
    int index = (timestamp - start_unix_time_) / 60;
    return index;
  }

  int getTimestamp(int index) {
    int timestamp = start_unix_time_ + index * 60;
    return timestamp;
  }

  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo &_market_update_info_) {
    if (watch_.tv().tv_sec % 86400 > start_unix_time_ &&
        watch_.tv().tv_sec % 86400 < end_unix_time_) {  // if u want to filter based on time
      if (this_smv_.bestask_size() > 0 && this_smv_.bestbid_size() > 0) {
        int index_ = getIndexFromSec(watch_.tv().tv_sec % 86400);
        // std::cout<<"GET INDEX: "<< index_ <<std::endl;
        ag_tv_sec[index_] = watch_.tv().tv_sec - watch_.tv().tv_sec % 60;
        total_size_ += (this_smv_.bestask_size() + this_smv_.bestbid_size()) / 2;
        ag_total_size_[index_] += (this_smv_.bestask_size() + this_smv_.bestbid_size()) / 2;

        avg_px_ = (avg_px_ * total_count_ + this_smv_.mid_price()) / (total_count_ + 1);
        ag_avg_px_[index_] =
            (ag_avg_px_[index_] * ag_total_count_[index_] + this_smv_.mid_price()) / (ag_total_count_[index_] + 1);

        avg_px2_ = (avg_px2_ * total_count_ + this_smv_.mid_price() * this_smv_.mid_price()) / (total_count_ + 1);
        ag_avg_px2_[index_] =
            (ag_avg_px2_[index_] * ag_total_count_[index_] + this_smv_.mid_price() * this_smv_.mid_price()) /
            (ag_total_count_[index_] + 1);

        total_count_++;
        ag_total_count_[index_]++;
        if (this_smv_.mid_price() > max_px_) {
          max_px_ = this_smv_.mid_price();
        }
        if (this_smv_.mid_price() > ag_max_px_[index_]) {
          ag_max_px_[index_] = this_smv_.mid_price();
        }

        if (this_smv_.mid_price() < min_px_) {
          min_px_ = this_smv_.mid_price();
        }

        if (this_smv_.mid_price() < ag_min_px_[index_]) {
          ag_min_px_[index_] = this_smv_.mid_price();
        }
        double spread = this_smv_.bestask_price() - this_smv_.bestbid_price();

        if (spread > max_spread_) max_spread_ = spread;
        if (spread > ag_max_spread_[index_]) ag_max_spread_[index_] = spread;

        if (spread < min_spread_) min_spread_ = spread;
        if (spread < ag_min_spread_[index_]) ag_min_spread_[index_] = spread;

        avg_spread_ = (avg_spread_ * (total_count_ - 1) + spread) / total_count_;
        ag_avg_spread_[index_] =
            (ag_avg_spread_[index_] * (ag_total_count_[index_] - 1) + spread) / ag_total_count_[index_];
        min_px_increment_ = this_smv_.min_price_increment();
        ag_min_px_increment_[index_] = this_smv_.min_price_increment();
      }
    }
  }

  inline void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo &_trade_print_info_,
                           const HFSAT::MarketUpdateInfo &_market_update_info_) {
    if (watch_.tv().tv_sec % 86400 > start_unix_time_ &&
        watch_.tv().tv_sec % 86400 < end_unix_time_) {  // if u want to filter based on time
      if (this_smv_.bestask_size() > 0 && this_smv_.bestbid_size() > 0) {
        int index_ = getIndexFromSec(watch_.tv().tv_sec % 86400);
        // std::cout<<"GET INDEX: "<< index_ <<std::endl;
        ag_tv_sec[index_] = watch_.tv().tv_sec - watch_.tv().tv_sec % 60;
        avg_px_trades_ = (avg_px_trades_ * total_trades_ + _trade_print_info_.trade_price_) / (total_trades_ + 1);
        ag_avg_px_trades_[index_] =
            (ag_avg_px_trades_[index_] * ag_total_trades_[index_] + _trade_print_info_.trade_price_) /
            (ag_total_trades_[index_] + 1);

        avg_trade_sz_ = (avg_trade_sz_ * total_trades_ + _trade_print_info_.size_traded_) / (total_trades_ + 1);
        ag_avg_trade_sz_[index_] =
            (ag_avg_trade_sz_[index_] * ag_total_trades_[index_] + _trade_print_info_.size_traded_) /
            (ag_total_trades_[index_] + 1);

        total_trades_ += 1;
        ag_total_trades_[index_] += 1;

        if (_trade_print_info_.buysell_ == HFSAT::kTradeTypeBuy) {
          total_buy_volume_ += _trade_print_info_.size_traded_;
          ag_total_buy_volume_[index_] += _trade_print_info_.size_traded_;
        } else if (_trade_print_info_.buysell_ == HFSAT::kTradeTypeSell) {
          total_sell_volume_ += _trade_print_info_.size_traded_;
          ag_total_sell_volume_[index_] += _trade_print_info_.size_traded_;
        }

        if (_trade_print_info_.size_traded_ > max_trade_sz_) {
          max_trade_sz_ = _trade_print_info_.size_traded_;
        } else if (_trade_print_info_.size_traded_ < min_trade_sz_) {
          min_trade_sz_ = _trade_print_info_.size_traded_;
        }

        if (_trade_print_info_.size_traded_ > ag_max_trade_sz_[index_]) {
          ag_max_trade_sz_[index_] = _trade_print_info_.size_traded_;
        } else if (_trade_print_info_.size_traded_ < ag_min_trade_sz_[index_]) {
          ag_min_trade_sz_[index_] = _trade_print_info_.size_traded_;
        }

        volume_weighted_avg_px_ = (volume_weighted_avg_px_ * total_volume_ +
                                   _trade_print_info_.trade_price_ * _trade_print_info_.size_traded_) /
                                  (total_volume_ + _trade_print_info_.size_traded_);
        ag_volume_weighted_avg_px_[index_] = (ag_volume_weighted_avg_px_[index_] * ag_total_volume_[index_] +
                                              _trade_print_info_.trade_price_ * _trade_print_info_.size_traded_) /
                                             (ag_total_volume_[index_] + _trade_print_info_.size_traded_);

        volume_weighted_avg_px2_ =
            (volume_weighted_avg_px2_ * total_volume_ +
             (_trade_print_info_.trade_price_ * _trade_print_info_.trade_price_ * _trade_print_info_.size_traded_)) /
            (total_volume_ + _trade_print_info_.size_traded_);
        ag_volume_weighted_avg_px2_[index_] =
            (ag_volume_weighted_avg_px2_[index_] * ag_total_volume_[index_] +
             (_trade_print_info_.trade_price_ * _trade_print_info_.trade_price_ * _trade_print_info_.size_traded_)) /
            (ag_total_volume_[index_] + _trade_print_info_.size_traded_);

        total_volume_ += _trade_print_info_.size_traded_;
        ag_total_volume_[index_] += _trade_print_info_.size_traded_;
      }
    }
  }

  void thread_main() {
    while (!day_over_) {
      sleep(1);
    }
  }

  void DayOver() { day_over_ = true; }
};

int main(int argc, char **argv) {
  // Assume that we get the filename of a file that only has all logged EBS data pertaining to one symbol.
  // Load up the File Source

  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " shortcode_file input_date_YYYYMMDD " << std::endl;
    exit(0);
  }

  bool is_TMX_OF = false;
  if (argc == 4) {
    is_TMX_OF = strcmp(argv[argc - 1], "OF") == 0;
    argc--;
  }
  std::string shortcode_file = argv[1];
  int tradingdate_ = atoi(argv[2]);
  int start_unix_time_ = 0;
  int end_unix_time_ = 10 * 60 * 60;
  std::cout<<"END TIME VALUE " << end_unix_time_ << " LAST INDEX: "<< ((end_unix_time_ % 86400)/60) <<std::endl;
  HFSAT::DbUpdateNse &db_update_nse = HFSAT::DbUpdateNse::GetUniqueInstance(argv[2], false, false, false);
  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);

  std::ifstream shortcode_list_file_(shortcode_file);
  if (shortcode_list_file_.is_open()) {
    std::string _this_shortcode_;
    while (getline(shortcode_list_file_, _this_shortcode_)) {
      if (_this_shortcode_ == "") continue;
      std::cout << "Running for shortcode " << _this_shortcode_ << std::endl;
      std::vector<std::string> shortcode_list_;
      shortcode_list_.push_back(_this_shortcode_);
      // --- Make object of CommonSMVSource and use it as an API -------------------------------------------
      CommonSMVSource *common_smv_source = new CommonSMVSource(shortcode_list_, tradingdate_);

      // book buidling modification purpose
      common_smv_source->SetTMXBookType(is_TMX_OF);

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
      std::string exch_sym = common_smv_source->getExchangleSymbol();
      std::string exp = std::to_string(HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(_this_shortcode_));
      std::string expiry_ = exp.substr(0, 4) + "-" + exp.substr(4, 2) + "-" + exp.substr(6, 2);
      int strike_price = HFSAT::NSESecurityDefinitions::GetStrikePriceFromShortCode(_this_shortcode_);
      int lotsize = HFSAT::SecurityDefinitions::GetContractMinOrderSize(_this_shortcode_, tradingdate_);
      std::cout << "Exch_sym " << exch_sym << " EXP: " << expiry_ << " SP: " << strike_price << " LOT: " << lotsize
                << std::endl;

      // --------------------------------- API use over------------------------------------------------------

      test_book_.DayOver();
      char print_secname_[24] = {0};
      strcpy(print_secname_, exchange_symbol_);
      for (size_t i = 0; i < strlen(print_secname_); ++i) {
        if (print_secname_[i] == ' ') {  // Liffe symbols have space, which is bad for the post processing script
          print_secname_[i] = '~';
        }
      }
      // for each minute
      for (int i = 0; i < 2024; i++) {
        if (test_book_.ag_total_count_[i] < 1) {
          // std::cout << _this_shortcode_ << ' ' << print_secname_ << ' ' << 0 << std::endl;
        } else {
          double range = test_book_.ag_max_px_[i] - test_book_.ag_min_px_[i];
          double px_stdev_ = sqrt(test_book_.ag_avg_px2_[i] - test_book_.ag_avg_px_[i] * test_book_.ag_avg_px_[i]);
          double volume_weighted_px_stdev_ = sqrt(
              test_book_.volume_weighted_avg_px2_ + test_book_.ag_avg_px_trades_[i] * test_book_.ag_avg_px_trades_[i] -
              2 * test_book_.ag_avg_px_trades_[i] * test_book_.ag_volume_weighted_avg_px_[i]);
          double avg_l1_size_ = ((double)test_book_.ag_total_size_[i] / test_book_.ag_total_count_[i]);
	  if (test_book_.ag_total_count_[i] == 0) avg_l1_size_ = 0;
          double min_spread_ = test_book_.ag_min_spread_[i] / test_book_.ag_min_px_increment_[i];
          double max_spread_ = test_book_.ag_max_spread_[i] / test_book_.ag_min_px_increment_[i];
          double avg_spread_ = test_book_.ag_avg_spread_[i] / test_book_.ag_min_px_increment_[i];
          
           /*     std::cout << _this_shortcode_ << ' ' << print_secname_ << " timestamp " << test_book_.ag_tv_sec[i] << std::endl;
              std::cout << _this_shortcode_ << ' ' << print_secname_ << " min_px_ " <<
             test_book_.ag_min_px_[i] << std::endl; std::cout << _this_shortcode_ << ' ' << print_secname_ << " max_px_
             " << test_book_.ag_max_px_[i] << std::endl; std::cout << _this_shortcode_ << ' ' << print_secname_ << "
             px_range_ " << range
                        << std::endl;
                std::cout << _this_shortcode_ << ' ' << print_secname_ << " avg_px_ " << test_book_.ag_avg_px_[i] <<
             std::endl; std::cout << _this_shortcode_ << ' ' << print_secname_ << " px_stdev_ "
                        << px_stdev_ << std::endl;
                std::cout << _this_shortcode_ << ' ' << print_secname_ << " total_volume_ " <<
             test_book_.ag_total_volume_[i]
                        << std::endl;
                std::cout << _this_shortcode_ << ' ' << print_secname_ << " total_trades_ " <<
             test_book_.ag_total_trades_[i]
                        << std::endl;
                std::cout << _this_shortcode_ << ' ' << print_secname_ << " total_buy_volume_ " <<
             test_book_.ag_total_buy_volume_[i]
                        << std::endl;
                std::cout << _this_shortcode_ << ' ' << print_secname_ << " total_sell_volume_ " <<
             test_book_.ag_total_sell_volume_[i]
                        << std::endl;
                std::cout << _this_shortcode_ << ' ' << print_secname_ << " volume_weighted_avg_px_ "
                        << test_book_.ag_volume_weighted_avg_px_[i] << std::endl;
                std::cout << _this_shortcode_ << ' ' << print_secname_ << " volume_weighted_px_stdev_ "
                        << volume_weighted_px_stdev_ << std::endl;
                std::cout << _this_shortcode_ << ' ' << print_secname_ << " avg_l1_size_ "
                        << avg_l1_size_ << std::endl;
                std::cout << _this_shortcode_ << ' ' << print_secname_ << " min_trd_sz_ " <<
             test_book_.ag_min_trade_sz_[i] << std::endl; std::cout << _this_shortcode_ << ' ' << print_secname_ << "
             max_trd_sz_ " << test_book_.ag_max_trade_sz_[i] << std::endl; std::cout << _this_shortcode_ << ' ' <<
             print_secname_ << " avg_trd_sz_ " << test_book_.ag_avg_trade_sz_[i] << std::endl;

                std::cout << _this_shortcode_ << ' ' << print_secname_ << " min_spread_ "
                        << min_spread_ << std::endl;
                std::cout << _this_shortcode_ << ' ' << print_secname_ << " max_spread_ "
                        << max_spread_ << std::endl;
                std::cout << _this_shortcode_ << ' ' << print_secname_ << " avg_spread_ "
                        << avg_spread_ << std::endl;
          */
          db_update_nse.UpdateMktMin(
              test_book_.ag_tv_sec[i], exch_sym, test_book_.ag_min_px_[i], test_book_.ag_max_px_[i], range,
              test_book_.ag_avg_px_[i], px_stdev_, test_book_.ag_total_volume_[i], test_book_.ag_total_trades_[i],
              test_book_.ag_total_buy_volume_[i], test_book_.ag_total_sell_volume_[i],
              test_book_.ag_volume_weighted_avg_px_[i], volume_weighted_px_stdev_, avg_l1_size_,
              test_book_.ag_min_trade_sz_[i], test_book_.ag_max_trade_sz_[i], test_book_.ag_avg_trade_sz_[i],
              min_spread_, max_spread_, avg_spread_, _this_shortcode_, expiry_, strike_price, lotsize);
        }
      }
      std::cout << "\n        ---PUSHING WHOLE DAY DATA--- " << std::endl;
      // push whole day entry
      if (test_book_.total_count_ < 1) {
        std::cout << _this_shortcode_ << ' ' << print_secname_ << ' ' << 0 << std::endl;
      } else {
        double range = test_book_.max_px_ - test_book_.min_px_;
        double px_stdev_ = sqrt(test_book_.avg_px2_ - test_book_.avg_px_ * test_book_.avg_px_);
        double volume_weighted_px_stdev_ =
            sqrt(test_book_.volume_weighted_avg_px2_ + test_book_.avg_px_trades_ * test_book_.avg_px_trades_ -
                 2 * test_book_.avg_px_trades_ * test_book_.volume_weighted_avg_px_);
        double avg_l1_size_ = ((double)test_book_.total_size_ / test_book_.total_count_);
	if (test_book_.total_count_ == 0) avg_l1_size_ = 0;
        double min_spread_ = test_book_.min_spread_ / test_book_.min_px_increment_;
        double max_spread_ = test_book_.max_spread_ / test_book_.min_px_increment_;
        double avg_spread_ = test_book_.avg_spread_ / test_book_.min_px_increment_;
        /*
              std::cout << _this_shortcode_ << ' ' << print_secname_ << " min_px_ " << test_book_.min_px_ << std::endl;
              std::cout << _this_shortcode_ << ' ' << print_secname_ << " max_px_ " << test_book_.max_px_ << std::endl;
              std::cout << _this_shortcode_ << ' ' << print_secname_ << " px_range_ " << range
                      << std::endl;
              std::cout << _this_shortcode_ << ' ' << print_secname_ << " avg_px_ " << test_book_.avg_px_ << std::endl;
              std::cout << _this_shortcode_ << ' ' << print_secname_ << " px_stdev_ "
                      << px_stdev_ << std::endl;
              std::cout << _this_shortcode_ << ' ' << print_secname_ << " total_volume_ " << test_book_.total_volume_
                      << std::endl;
              std::cout << _this_shortcode_ << ' ' << print_secname_ << " total_trades_ " << test_book_.total_trades_
                      << std::endl;
              std::cout << _this_shortcode_ << ' ' << print_secname_ << " total_buy_volume_ " <<
           test_book_.total_buy_volume_
                      << std::endl;
              std::cout << _this_shortcode_ << ' ' << print_secname_ << " total_sell_volume_ " <<
           test_book_.total_sell_volume_
                      << std::endl;
              std::cout << _this_shortcode_ << ' ' << print_secname_ << " volume_weighted_avg_px_ "
                      << test_book_.volume_weighted_avg_px_ << std::endl;
              std::cout << _this_shortcode_ << ' ' << print_secname_ << " volume_weighted_px_stdev_ "
                      << volume_weighted_px_stdev_<< std::endl;
              std::cout << _this_shortcode_ << ' ' << print_secname_ << " avg_l1_size_ "
                      << avg_l1_size_ << std::endl;
              std::cout << _this_shortcode_ << ' ' << print_secname_ << " min_trd_sz_ " << test_book_.min_trade_sz_ <<
           std::endl; std::cout << _this_shortcode_ << ' ' << print_secname_ << " max_trd_sz_ " <<
           test_book_.max_trade_sz_ << std::endl; std::cout << _this_shortcode_ << ' ' << print_secname_ << "
           avg_trd_sz_ " << test_book_.avg_trade_sz_ << std::endl;

              std::cout << _this_shortcode_ << ' ' << print_secname_ << " min_spread_ "
                      << min_spread_ << std::endl;
              std::cout << _this_shortcode_ << ' ' << print_secname_ << " max_spread_ "
                      << max_spread_ << std::endl;
              std::cout << _this_shortcode_ << ' ' << print_secname_ << " avg_spread_ "
                      << avg_spread_ << std::endl;
        */
        db_update_nse.UpdateMktDaily(
            exch_sym, test_book_.min_px_, test_book_.max_px_, range, test_book_.avg_px_, px_stdev_,
            test_book_.total_volume_, test_book_.total_trades_, test_book_.total_buy_volume_,
            test_book_.total_sell_volume_, test_book_.volume_weighted_avg_px_, volume_weighted_px_stdev_, avg_l1_size_,
            test_book_.min_trade_sz_, test_book_.max_trade_sz_, test_book_.avg_trade_sz_, min_spread_, max_spread_,
            avg_spread_, _this_shortcode_, expiry_, strike_price, lotsize, 0);
      }
      test_book_.stop();
    }
    shortcode_list_file_.close();
  } else {
    // TODO: File is missing
    std::cerr << "Missing FIle " << shortcode_file << std::endl;
  }

  return 0;
}
