// =====================================================================================
//
//       Filename:  generate_price_data.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  01/08/2013 04:32:32 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/pcaport_price.hpp"
#include "dvctrade/Indicators/pca_weights_manager.hpp"
#include "baseinfra/Tools/common_smv_source.hpp"

#define NORMALIZE_PRICE_FACTOR 100
#define USE_CHIX_L1 1

class PriceDataGenerator {
 private:
  std::map<std::string, bool> list_of_unique_shortcode_sources_;
  int date_;
  time_t unix_start_time_;
  time_t unix_end_time_;
  int time_interval_;

 public:
  PriceDataGenerator(std::map<std::string, bool>& list_of_shortcodes_, std::map<std::string, bool>& list_of_portfolios_,
                     int _date_, time_t start_time_, time_t end_time_, int interval_)
      : date_(_date_), unix_start_time_(start_time_), unix_end_time_(end_time_), time_interval_(interval_) {
    std::map<std::string, bool>::iterator itr_ = list_of_shortcodes_.begin();

    while (itr_ != list_of_shortcodes_.end()) {
      list_of_unique_shortcode_sources_[itr_->first];
      itr_++;
    }

    itr_ = list_of_portfolios_.begin();
    HFSAT::PcaWeightsManager::SetUniqueInstance(_date_);
    HFSAT::PcaWeightsManager& pca_weight_manager_ = HFSAT::PcaWeightsManager::GetUniqueInstance();

    while (itr_ != list_of_portfolios_.end()) {
      std::vector<std::string> this_portfolio_shortcode_vec_;

      pca_weight_manager_.GetPortfolioShortCodeVec(itr_->first, this_portfolio_shortcode_vec_);

      if (this_portfolio_shortcode_vec_.size() > 0) {
        for (unsigned int shortcode_counter_ = 0; shortcode_counter_ < this_portfolio_shortcode_vec_.size();
             shortcode_counter_++) {
          list_of_unique_shortcode_sources_[this_portfolio_shortcode_vec_[shortcode_counter_]];
        }
      }

      itr_++;
    }
  }

  void GetAllUniqueShortcodes(std::vector<std::string>& shortcode_list_) {
    std::map<std::string, bool>::iterator itr_ = list_of_unique_shortcode_sources_.begin();

    while (itr_ != list_of_unique_shortcode_sources_.end()) {
      shortcode_list_.push_back(itr_->first);

      itr_++;
    }
  }
};

class MarketUpdateSubscriber {
 private:
  std::map<std::string, double> data_source_to_normalize_factor_;
  HFSAT::Watch& watch_;

  time_t unix_start_time_;
  time_t unix_end_time_;
  int update_interval_;

  unsigned long last_update_interval_;
  bool print_original_price_;

 public:
  MarketUpdateSubscriber(HFSAT::Watch& _watch_, time_t start_time_, time_t end_time_, int interval_,
                         bool _print_original_price_)
      : data_source_to_normalize_factor_(),
        watch_(_watch_),
        unix_start_time_(start_time_),
        unix_end_time_(end_time_),
        update_interval_(interval_),
        last_update_interval_(0),
        print_original_price_(_print_original_price_) {}

  inline void OnShortcodePortfolioMarketUpdate(std::string data_source_, double price_) {
    int time_sec_ = watch_.tv().tv_sec;
    if (time_sec_ < unix_start_time_ || time_sec_ > unix_end_time_) return;

    if (data_source_to_normalize_factor_.find(data_source_) == data_source_to_normalize_factor_.end()) {
      double this_normalize_factor_ = 0.0;

      if (print_original_price_) {
        this_normalize_factor_ = 1.0;
      } else {
        this_normalize_factor_ = 100 / price_;  // we should not see negetive price ,.. but still cant take chances.
      }

      data_source_to_normalize_factor_[data_source_] = this_normalize_factor_;
    }

    if (update_interval_ == -1) {
      std::cout << watch_.tv() << " " << data_source_ << " "
                << price_ * (double)(data_source_to_normalize_factor_[data_source_]) << "\n";

    } else {
      if (last_update_interval_ == 0) {
        last_update_interval_ = watch_.tv().tv_sec * 1000000 + watch_.tv().tv_usec;

        std::cout << watch_.tv() << " " << data_source_ << " "
                  << price_ * (double)(data_source_to_normalize_factor_[data_source_]) << "\n";

      } else {
        unsigned long new_time_ = watch_.tv().tv_sec * 1000000 + watch_.tv().tv_usec;

        if ((int)(new_time_ - last_update_interval_) < update_interval_) return;

        last_update_interval_ = new_time_;

        std::cout << watch_.tv() << " " << data_source_ << " "
                  << price_ * (double)(data_source_to_normalize_factor_[data_source_]) << "\n";
      }
    }
  }
};

class PortfolioPriceGenerator : public HFSAT::CommonIndicator {
 private:
  HFSAT::Watch& watch_;
  std::vector<MarketUpdateSubscriber*> market_update_subscriber_listener_;
  std::string this_shortcode_;
  HFSAT::PriceType_t this_pricetype_subscribed_;

 public:
  PortfolioPriceGenerator(HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_, HFSAT::PriceType_t price_type_,
                          std::string _portfolio_descriptor_shortcode_)
      : CommonIndicator(_dbglogger_, watch_, _portfolio_descriptor_shortcode_),
        watch_(_watch_),
        market_update_subscriber_listener_(),
        this_shortcode_(_portfolio_descriptor_shortcode_),
        this_pricetype_subscribed_(price_type_)

  {}

  void OnPortfolioPriceChange(double this_port_price_) {
    NotifyMarketUpdate(this_port_price_);
    //    std::cout << " Portfolio Price : " << this_port_price_ << "\n" ;
  }

  void OnPortfolioPriceReset(double _new_port_price_, double _old_port_price_, unsigned int is_data_interrupted_) {
    NotifyMarketUpdate(_new_port_price_);  // we choose to remove constituent from the port
    //    std::cout << " Portfolio Price : " << this_port_price_ << "\n" ;
  }

  // TODO handle them
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}
  void OnMarketDataResumed(const unsigned int _security_id_) {}
  // functions

  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) {
    switch (this_pricetype_subscribed_) {
      case HFSAT::kPriceTypeMktSizeWPrice: {
        NotifyMarketUpdate(_market_update_info_.mkt_size_weighted_price_);

      } break;

      case HFSAT::kPriceTypeOrderWPrice: {
        NotifyMarketUpdate(_market_update_info_.order_weighted_price_);

      } break;

      case HFSAT::kPriceTypeMidprice: {
        NotifyMarketUpdate(_market_update_info_.mid_price_);

      } break;

      case HFSAT::kPriceTypeMktSinusoidal: {
        NotifyMarketUpdate(_market_update_info_.mkt_sinusoidal_price_);

      } break;

      case HFSAT::kPriceTypeTradeWPrice: {
        NotifyMarketUpdate(_market_update_info_.trade_weighted_price_);

      } break;

      case HFSAT::kPriceTypeOfflineMixMMS: {
        NotifyMarketUpdate(_market_update_info_.offline_mix_mms_price_);

      } break;

      case HFSAT::kPriceTypeImpliedVol: {
        NotifyMarketUpdate(_market_update_info_.implied_vol_);

      } break;

      default: { } break; }
  }

  inline void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                           const HFSAT::MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void AddMarketUpdateSubscriber(MarketUpdateSubscriber* this_market_update_subscriber_) {
    HFSAT::VectorUtils::UniqueVectorAdd(market_update_subscriber_listener_, this_market_update_subscriber_);
  }

  inline void NotifyMarketUpdate(double update_price_) {
    for (auto i = 0u; i < market_update_subscriber_listener_.size(); i++) {
      market_update_subscriber_listener_[i]->OnShortcodePortfolioMarketUpdate(this_shortcode_, update_price_);
    }
  }
};

void printUsage() {
  std::cerr << " Usage < exec > < SHORTCODE > < SHORTCODE_LIST > < PORT > < PORTFOLIO_LIST > < DATE > < PRICE_TYPE > < "
               "UTC_START_TIME > < UTC_END_TIME > < INTERVAL (USEC) = -1 > ORIG_PRICE \n";
  std::cerr << " SHORTCODE FESX_0 FGBL_0 -1 PORT ULEQUI -1 20130108 MktSizeWPrice 1200 1300 -1 \n";

  exit(-1);
}

int main(int argc, char** argv) {
  std::map<std::string, bool> list_of_shortcodes_;
  std::map<std::string, bool> list_of_portfolios_;
  bool print_original_price_ = false;
  if (argc < 8) {
    printUsage();
    exit(-1);
  }

  if (std::string(argv[1]) != "SHORTCODE") {
    printUsage();
    exit(-1);
  }

  int index_ = 2;

  for (; index_ < argc; index_++) {
    if (std::string(argv[index_]) == "-1") break;

    list_of_shortcodes_[argv[index_]];
  }

  index_++;

  if (std::string(argv[index_]) != "PORT") {
    printUsage();
    exit(-1);
  }

  index_++;

  for (; index_ < argc; index_++) {
    if (std::string(argv[index_]) == "-1") break;

    list_of_portfolios_[argv[index_]];
  }

  int date_ = atoi(argv[++index_]);

  if (date_ < 20110101 || date_ > 20990101) {
    printUsage();
    exit(-1);
  }

  std::string price_type_ = argv[++index_];

  if (price_type_ != "MktSizeWPrice" && price_type_ != "OrderWPrice" && price_type_ != "MidPrice" &&
      price_type_ != "Midprice" && price_type_ != "MktSinusoidal" && price_type_ != "TradeWPrice" &&
      price_type_ != "ImpliedVol" && price_type_ != "OfflineMixMMS") {
    printUsage();
    exit(-1);
  }

  int start_time_ = 0;
  int end_time_ = 0;

  const char* start_str_ = (argv[++index_]);
  const char* end_str_ = (argv[++index_]);

  {
    if ((strncmp(start_str_, "EST_", 4) == 0) || (strncmp(start_str_, "CST_", 4) == 0) ||
        (strncmp(start_str_, "CET_", 4) == 0) || (strncmp(start_str_, "BRT_", 4) == 0) ||
        (strncmp(start_str_, "UTC_", 4) == 0) || (strncmp(start_str_, "KST_", 4) == 0) ||
        (strncmp(start_str_, "HKT_", 4) == 0) || (strncmp(start_str_, "MSK_", 4) == 0) ||
        (strncmp(start_str_, "IST_", 4) == 0) || (strncmp(start_str_, "JST_", 4) == 0) ||
        (strncmp(start_str_, "BST_", 4) == 0)) {
      start_time_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(date_, atoi(start_str_ + 4), start_str_);
    } else {
      start_time_ = atoi(start_str_);
    }
  }
  {
    if ((strncmp(end_str_, "EST_", 4) == 0) || (strncmp(end_str_, "CST_", 4) == 0) ||
        (strncmp(end_str_, "CET_", 4) == 0) || (strncmp(end_str_, "BRT_", 4) == 0) ||
        (strncmp(end_str_, "UTC_", 4) == 0) || (strncmp(end_str_, "KST_", 4) == 0) ||
        (strncmp(end_str_, "HKT_", 4) == 0) || (strncmp(end_str_, "MSK_", 4) == 0) ||
        (strncmp(end_str_, "IST_", 4) == 0) || (strncmp(end_str_, "JST_", 4) == 0) ||
        (strncmp(end_str_, "BST_", 4) == 0)) {
      end_time_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(date_, atoi(end_str_ + 4), end_str_);
    } else {
      end_time_ = atoi(end_str_);
    }
  }

  struct tm timeinfo = {0};

  timeinfo.tm_year = (date_ / 10000) - 1900;
  timeinfo.tm_mon = (date_ / 100) % 100 - 1;
  timeinfo.tm_mday = (date_ % 100);

  time_t unixtime_start_ = mktime(&timeinfo);
  time_t unixtime_end_ = mktime(&timeinfo);

  start_time_ = (start_time_ / 100) * 3600 + (start_time_ % 100) * 60;
  end_time_ = (end_time_ / 100) * 3600 + (end_time_ % 100) * 60;

  unixtime_start_ += start_time_;
  unixtime_end_ += end_time_;

  int time_interval_ = -1;

  if (index_ < argc - 2) {
    time_interval_ = atoi(argv[++index_]);
  }
  if (index_ < argc - 1) {
    print_original_price_ = (atoi(argv[++index_]) != 0);
  }

  std::vector<std::string> list_of_all_shortcodes_;
  PriceDataGenerator price_date_generator_(list_of_shortcodes_, list_of_portfolios_, date_, unixtime_start_,
                                           unixtime_end_, time_interval_);
  price_date_generator_.GetAllUniqueShortcodes(list_of_all_shortcodes_);

  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "/spare/local/logs/alllogs/generate_price_data_" << HFSAT::DateTime::GetCurrentIsoDateLocal();
  std::string logfilename = t_temp_oss_.str();

  // --- Make object of CommonSMVSource and use it as an API -------------------------------------------
  CommonSMVSource* common_smv_source = new CommonSMVSource(list_of_all_shortcodes_, date_);

  // Get the dbglogger and watch after creating the source
  HFSAT::Watch& watch_ = common_smv_source->getWatch();
  HFSAT::DebugLogger& dbglogger_ = common_smv_source->getLogger();

  // Set debug logger filename
  common_smv_source->SetDbgloggerFileName(logfilename);

  // Initialize the smv source after setting the required variables
  common_smv_source->Initialize();

  MarketUpdateSubscriber market_update_subscriber_(watch_, unixtime_start_, unixtime_end_, time_interval_,
                                                   print_original_price_);

  // Get the security id to smv ptr from common source
  HFSAT::SecurityMarketViewPtrVec sid_to_smv_ptr_map = common_smv_source->getSMVMap();

  for (auto smv : sid_to_smv_ptr_map) {
    std::string shortcode = smv->shortcode();
    if (list_of_shortcodes_.find(shortcode) != list_of_shortcodes_.end()) {
      PortfolioPriceGenerator* portfolio_price_generator_ =
          new PortfolioPriceGenerator(dbglogger_, watch_, HFSAT::StringToPriceType_t(price_type_), shortcode);
      smv->subscribe_price_type(portfolio_price_generator_, HFSAT::StringToPriceType_t(price_type_));
      portfolio_price_generator_->AddMarketUpdateSubscriber(&market_update_subscriber_);
    }
  }

  std::map<std::string, bool>::iterator itr_ = list_of_portfolios_.begin();

  while (itr_ != list_of_portfolios_.end()) {
    HFSAT::PCAPortPrice* indep_portfolio_price__ =
        HFSAT::PCAPortPrice::GetUniqueInstance(dbglogger_, watch_, itr_->first, HFSAT::kPriceTypeMktSizeWPrice);

    PortfolioPriceGenerator* portfolio_price_generator_ =
        new PortfolioPriceGenerator(dbglogger_, watch_, HFSAT::kPriceTypeMax, itr_->first);
    indep_portfolio_price__->AddPriceChangeListener(portfolio_price_generator_);
    portfolio_price_generator_->AddMarketUpdateSubscriber(&market_update_subscriber_);

    itr_++;
  }

  // start event loop : Runs historical dispatcher, callbacks of MktUpdate, and TradePrint
  common_smv_source->Run();

  //  std::cout << " List Of Data : " << list_of_data_.size () << " Date : " << date_ << " Start Time : " << start_time_
  //  << " End Time : " << end_time_ << " Time Interval : " << time_interval_ << "\n" ;

  return 0;
}
