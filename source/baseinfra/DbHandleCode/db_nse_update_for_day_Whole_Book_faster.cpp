/**
   \file DbHandleCode/db_nse_update_for_day_Whole_Book_faster.cpp

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
#define COLUMN_SIZE 1024
#define ROW_SIZE 610

// MOVED TO GLOBAL NAMESPACE DUE TO SPACE(seg fault)
  double ag_min_px_[COLUMN_SIZE][ROW_SIZE];
  double ag_max_px_[COLUMN_SIZE][ROW_SIZE];
  double ag_avg_px_[COLUMN_SIZE][ROW_SIZE];
  double ag_avg_px2_[COLUMN_SIZE][ROW_SIZE];
  double ag_avg_px_trades_[COLUMN_SIZE][ROW_SIZE];
  double ag_volume_weighted_avg_px_[COLUMN_SIZE][ROW_SIZE];
  double ag_volume_weighted_avg_px2_[COLUMN_SIZE][ROW_SIZE];
  int ag_total_volume_[COLUMN_SIZE][ROW_SIZE];
  int ag_total_trades_[COLUMN_SIZE][ROW_SIZE];
  int ag_total_buy_volume_[COLUMN_SIZE][ROW_SIZE];
  int ag_total_sell_volume_[COLUMN_SIZE][ROW_SIZE];
  uint64_t ag_total_size_[COLUMN_SIZE][ROW_SIZE];
  int ag_total_count_[COLUMN_SIZE][ROW_SIZE];
  double ag_min_trade_sz_[COLUMN_SIZE][ROW_SIZE];
  double ag_max_trade_sz_[COLUMN_SIZE][ROW_SIZE];
  double ag_avg_trade_sz_[COLUMN_SIZE][ROW_SIZE];
  double ag_max_spread_[COLUMN_SIZE][ROW_SIZE];
  double ag_min_spread_[COLUMN_SIZE][ROW_SIZE];
  double ag_avg_spread_[COLUMN_SIZE][ROW_SIZE];
  double ag_min_px_increment_[COLUMN_SIZE][ROW_SIZE];
  double ag_price_stdev_[COLUMN_SIZE][ROW_SIZE];
  int ag_tv_sec[COLUMN_SIZE][ROW_SIZE];
  int ag_no_of_msg[COLUMN_SIZE][ROW_SIZE];

class TestBook : public HFSAT::Thread, public HFSAT::SecurityMarketViewChangeListener {
  const HFSAT::SecurityMarketViewPtrVec &this_smv_vec;
  HFSAT::Watch &watch_;

  /// 0 : keep changing book and showing current book
  /// 1 : event by event
  /// 2 : keep changing book and refreshing screen till the specified time is reached
  bool day_over_;

  int start_unix_time_;
  int end_unix_time_;

 public:
  TestBook(const HFSAT::SecurityMarketViewPtrVec &_this_smv_vec, HFSAT::Watch &_watch_,
           const int t_start_unix_time_ = 0, const int t_end_unix_time_ = 0)
      : this_smv_vec(_this_smv_vec),
        watch_(_watch_),
        day_over_(false),
        start_unix_time_(t_start_unix_time_),
        end_unix_time_(t_end_unix_time_) {
    for (unsigned int index = 0; index < this_smv_vec.size(); index++) {
      this_smv_vec[index]->ComputeMidPrice();
    }

    for (int index = 0; index < COLUMN_SIZE; index++) {
      max_px_[index] = avg_px_[index] = avg_px2_[index] = avg_px_trades_[index] = volume_weighted_avg_px_[index] =
          volume_weighted_avg_px2_[index] = avg_spread_[index] = 0.0;
	no_of_msg[index] = 0;
      total_volume_[index] = total_trades_[index] = total_buy_volume_[index] = total_sell_volume_[index] =
          total_size_[index] = total_count_[index] = max_trade_sz_[index] = avg_trade_sz_[index] = max_spread_[index] =
              0;
      min_px_[index] = min_trade_sz_[index] = min_spread_[index] = 10000000.0;
    }

    for (int id = 0; id < COLUMN_SIZE; id++) {
      for (int index = 0; index < ROW_SIZE; index++) {
        ag_max_px_[id][index] = ag_avg_px_[id][index] = ag_avg_px2_[id][index] = ag_avg_px_trades_[id][index] =
            ag_volume_weighted_avg_px2_[id][index] = ag_volume_weighted_avg_px_[id][index] = 0.0;
        ag_total_volume_[id][index] = ag_total_trades_[id][index] = ag_total_buy_volume_[id][index] =
            ag_total_sell_volume_[id][index] = ag_total_size_[id][index] = ag_total_count_[id][index] = 0;
        ag_min_trade_sz_[id][index] = ag_max_trade_sz_[id][index] = ag_avg_trade_sz_[id][index] =
            ag_max_spread_[id][index] = ag_avg_spread_[id][index] = ag_min_px_increment_[id][index] =
                ag_price_stdev_[id][index] = 0;
        ag_min_px_[id][index] = 10000000.0;
        ag_min_trade_sz_[id][index] = 10000000.0;
        ag_min_spread_[id][index] = 10000000.0;
	ag_no_of_msg[id][index] = 0;
      }
    }
  }
  double min_px_[COLUMN_SIZE];
  double max_px_[COLUMN_SIZE];
  double avg_px_[COLUMN_SIZE];
  double avg_px2_[COLUMN_SIZE];
  double avg_px_trades_[COLUMN_SIZE];
  double volume_weighted_avg_px_[COLUMN_SIZE];
  double volume_weighted_avg_px2_[COLUMN_SIZE];
  int total_volume_[COLUMN_SIZE];
  int total_trades_[COLUMN_SIZE];
  int total_buy_volume_[COLUMN_SIZE];
  int total_sell_volume_[COLUMN_SIZE];
  uint64_t total_size_[COLUMN_SIZE];
  int total_count_[COLUMN_SIZE];
  double min_trade_sz_[COLUMN_SIZE];
  double max_trade_sz_[COLUMN_SIZE];
  double avg_trade_sz_[COLUMN_SIZE];
  double max_spread_[COLUMN_SIZE];
  double min_spread_[COLUMN_SIZE];
  double avg_spread_[COLUMN_SIZE];
  double min_px_increment_[COLUMN_SIZE];
  double price_stdev_[COLUMN_SIZE];
  int no_of_msg[COLUMN_SIZE];

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
      if (this_smv_vec[_security_id_]->bestask_size() > 0 && this_smv_vec[_security_id_]->bestbid_size() > 0) {
        int index_ = getIndexFromSec(watch_.tv().tv_sec % 86400);
        // std::cout<<"GET INDEX: "<< index_ <<std::endl;
        ag_tv_sec[_security_id_][index_] = watch_.tv().tv_sec - watch_.tv().tv_sec % 60;
        total_size_[_security_id_] +=
            (this_smv_vec[_security_id_]->bestask_size() + this_smv_vec[_security_id_]->bestbid_size()) / 2;
        ag_total_size_[_security_id_][index_] +=
            (this_smv_vec[_security_id_]->bestask_size() + this_smv_vec[_security_id_]->bestbid_size()) / 2;

        avg_px_[_security_id_] =
            (avg_px_[_security_id_] * total_count_[_security_id_] + this_smv_vec[_security_id_]->mid_price()) /
            (total_count_[_security_id_] + 1);
        ag_avg_px_[_security_id_][index_] =
            (ag_avg_px_[_security_id_][index_] * ag_total_count_[_security_id_][index_] +
             this_smv_vec[_security_id_]->mid_price()) /
            (ag_total_count_[_security_id_][index_] + 1);

        avg_px2_[_security_id_] =
            (avg_px2_[_security_id_] * total_count_[_security_id_] +
             this_smv_vec[_security_id_]->mid_price() * this_smv_vec[_security_id_]->mid_price()) /
            (total_count_[_security_id_] + 1);
        ag_avg_px2_[_security_id_][index_] =
            (ag_avg_px2_[_security_id_][index_] * ag_total_count_[_security_id_][index_] +
             this_smv_vec[_security_id_]->mid_price() * this_smv_vec[_security_id_]->mid_price()) /
            (ag_total_count_[_security_id_][index_] + 1);

        total_count_[_security_id_]++;
        ag_total_count_[_security_id_][index_]++;
        if (this_smv_vec[_security_id_]->mid_price() > max_px_[_security_id_]) {
          max_px_[_security_id_] = this_smv_vec[_security_id_]->mid_price();
        }
        if (this_smv_vec[_security_id_]->mid_price() > ag_max_px_[_security_id_][index_]) {
          ag_max_px_[_security_id_][index_] = this_smv_vec[_security_id_]->mid_price();
        }

        if (this_smv_vec[_security_id_]->mid_price() < min_px_[_security_id_]) {
          min_px_[_security_id_] = this_smv_vec[_security_id_]->mid_price();
        }

        if (this_smv_vec[_security_id_]->mid_price() < ag_min_px_[_security_id_][index_]) {
          ag_min_px_[_security_id_][index_] = this_smv_vec[_security_id_]->mid_price();
        }
        double spread = this_smv_vec[_security_id_]->bestask_price() - this_smv_vec[_security_id_]->bestbid_price();

        if (spread > max_spread_[_security_id_]) max_spread_[_security_id_] = spread;
        if (spread > ag_max_spread_[_security_id_][index_]) ag_max_spread_[_security_id_][index_] = spread;

        if (spread < min_spread_[_security_id_]) min_spread_[_security_id_] = spread;
        if (spread < ag_min_spread_[_security_id_][index_]) ag_min_spread_[_security_id_][index_] = spread;

        avg_spread_[_security_id_] =
            (avg_spread_[_security_id_] * (total_count_[_security_id_] - 1) + spread) / total_count_[_security_id_];
        ag_avg_spread_[_security_id_][index_] =
            (ag_avg_spread_[_security_id_][index_] * (ag_total_count_[_security_id_][index_] - 1) + spread) /
            ag_total_count_[_security_id_][index_];
        min_px_increment_[_security_id_] = this_smv_vec[_security_id_]->min_price_increment();
        ag_min_px_increment_[_security_id_][index_] = this_smv_vec[_security_id_]->min_price_increment();
	ag_no_of_msg[_security_id_][index_]++;
	no_of_msg[_security_id_]++;
      }
    }
  }

  inline void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo &_trade_print_info_,
                           const HFSAT::MarketUpdateInfo &_market_update_info_) {
//    std::cout<<"OnTradePrint " << watch_.tv().tv_sec   << " " << watch_.tv().tv_sec % 86400 <<" " << start_unix_time_ <<" "<< end_unix_time_ <<std::endl;
    if (watch_.tv().tv_sec % 86400 > start_unix_time_ &&
        watch_.tv().tv_sec % 86400 < end_unix_time_) {  // if u want to filter based on time
//      if (this_smv_vec[_security_id_]->bestask_size() > 0 && this_smv_vec[_security_id_]->bestbid_size() > 0) {
        int index_ = getIndexFromSec(watch_.tv().tv_sec % 86400);
//        std::cout<<"GET INDEX: "<< index_ <<std::endl;
        ag_tv_sec[_security_id_][index_] = watch_.tv().tv_sec - watch_.tv().tv_sec % 60;
        avg_px_trades_[_security_id_] =
            (avg_px_trades_[_security_id_] * total_trades_[_security_id_] + _trade_print_info_.trade_price_) /
            (total_trades_[_security_id_] + 1);
        ag_avg_px_trades_[_security_id_][index_] =
            (ag_avg_px_trades_[_security_id_][index_] * ag_total_trades_[_security_id_][index_] +
             _trade_print_info_.trade_price_) /
            (ag_total_trades_[_security_id_][index_] + 1);

        avg_trade_sz_[_security_id_] =
            (avg_trade_sz_[_security_id_] * total_trades_[_security_id_] + _trade_print_info_.size_traded_) /
            (total_trades_[_security_id_] + 1);
        ag_avg_trade_sz_[_security_id_][index_] =
            (ag_avg_trade_sz_[_security_id_][index_] * ag_total_trades_[_security_id_][index_] +
             _trade_print_info_.size_traded_) /
            (ag_total_trades_[_security_id_][index_] + 1);

        total_trades_[_security_id_] += 1;
        ag_total_trades_[_security_id_][index_] += 1;

        if (_trade_print_info_.buysell_ == HFSAT::kTradeTypeBuy) {
          total_buy_volume_[_security_id_] += _trade_print_info_.size_traded_;
          ag_total_buy_volume_[_security_id_][index_] += _trade_print_info_.size_traded_;
        } else if (_trade_print_info_.buysell_ == HFSAT::kTradeTypeSell) {
          total_sell_volume_[_security_id_] += _trade_print_info_.size_traded_;
          ag_total_sell_volume_[_security_id_][index_] += _trade_print_info_.size_traded_;
        }

        if (_trade_print_info_.size_traded_ > max_trade_sz_[_security_id_]) {
          max_trade_sz_[_security_id_] = _trade_print_info_.size_traded_;
        } else if (_trade_print_info_.size_traded_ < min_trade_sz_[_security_id_]) {
          min_trade_sz_[_security_id_] = _trade_print_info_.size_traded_;
        }

        if (_trade_print_info_.size_traded_ > ag_max_trade_sz_[_security_id_][index_]) {
          ag_max_trade_sz_[_security_id_][index_] = _trade_print_info_.size_traded_;
        } else if (_trade_print_info_.size_traded_ < ag_min_trade_sz_[_security_id_][index_]) {
          ag_min_trade_sz_[_security_id_][index_] = _trade_print_info_.size_traded_;
        }

        volume_weighted_avg_px_[_security_id_] =
            (volume_weighted_avg_px_[_security_id_] * total_volume_[_security_id_] +
             _trade_print_info_.trade_price_ * _trade_print_info_.size_traded_) /
            (total_volume_[_security_id_] + _trade_print_info_.size_traded_);
        ag_volume_weighted_avg_px_[_security_id_][index_] =
            (ag_volume_weighted_avg_px_[_security_id_][index_] * ag_total_volume_[_security_id_][index_] +
             _trade_print_info_.trade_price_ * _trade_print_info_.size_traded_) /
            (ag_total_volume_[_security_id_][index_] + _trade_print_info_.size_traded_);

        volume_weighted_avg_px2_[_security_id_] =
            (volume_weighted_avg_px2_[_security_id_] * total_volume_[_security_id_] +
             (_trade_print_info_.trade_price_ * _trade_print_info_.trade_price_ * _trade_print_info_.size_traded_)) /
            (total_volume_[_security_id_] + _trade_print_info_.size_traded_);
        ag_volume_weighted_avg_px2_[_security_id_][index_] =
            (ag_volume_weighted_avg_px2_[_security_id_][index_] * ag_total_volume_[_security_id_][index_] +
             (_trade_print_info_.trade_price_ * _trade_print_info_.trade_price_ * _trade_print_info_.size_traded_)) /
            (ag_total_volume_[_security_id_][index_] + _trade_print_info_.size_traded_);

        total_volume_[_security_id_] += _trade_print_info_.size_traded_;
        ag_total_volume_[_security_id_][index_] += _trade_print_info_.size_traded_;
	ag_no_of_msg[_security_id_][index_]++;
        no_of_msg[_security_id_]++;
	ag_min_px_increment_[_security_id_][index_] = this_smv_vec[_security_id_]->min_price_increment();
	min_px_increment_[_security_id_] = this_smv_vec[_security_id_]->min_price_increment();
      }
//    }
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
  bool force_clear_db = false;
  if (argc == 4) {
     std::cout << "DB FORCE CLEAR ENABLED " << std::endl;
     force_clear_db = true;
   }
  std::string shortcode_file = argv[1];
  int tradingdate_ = atoi(argv[2]);
  int start_unix_time_ = 0;
  int end_unix_time_ = 10 * 60 * 60;
  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  std::ifstream shortcode_list_file_(shortcode_file);
  bool is_cash = false;
  if (shortcode_list_file_.is_open()) {
    std::string _this_shortcode_;
    while (getline(shortcode_list_file_, _this_shortcode_)) {
      if (_this_shortcode_ == "") continue;
      is_cash = HFSAT::NSESecurityDefinitions::IsEquity(_this_shortcode_);
      break;
    }
  }
  std::cout<<"END TIME VALUE " << end_unix_time_ << " LAST INDEX: "<< (end_unix_time_ /60) <<std::endl;
  HFSAT::DbUpdateNse &db_update_nse = HFSAT::DbUpdateNse::GetUniqueInstance(argv[2], force_clear_db, false, is_cash);
  std::vector<std::string> shortcode_list_;
  if (shortcode_list_file_.is_open()) {
    std::string _this_shortcode_;
    while (getline(shortcode_list_file_, _this_shortcode_)) {
      if (_this_shortcode_ == "") continue;
      shortcode_list_.push_back(_this_shortcode_);
    }
    if ( shortcode_list_.size() > 1000){
       std::cout << "NUMBER OF SHORTCODE IN SHORTCODELIST FILE GREATER THAN 1000" << std::endl; 
       exit(-1);
    }
    
    // --- Make object of CommonSMVSource and use it as an API -------------------------------------------
    CommonSMVSource *common_smv_source = new CommonSMVSource(shortcode_list_, tradingdate_);
    // book buidling modification purpose
    common_smv_source->SetTMXBookType(is_TMX_OF);
    // Initialize the smv source after setting the required variables
    common_smv_source->Initialize();

    // Get the smv and watch after creating the source
    HFSAT::SecurityMarketViewPtrVec &p_smv_ = common_smv_source->getSMVMap();
    HFSAT::Watch &watch_ = common_smv_source->getWatch();
    std::cout<<"RUNNING BOOK UPDATES ...  " <<std::endl;
    TestBook test_book_(p_smv_, watch_, start_unix_time_, end_unix_time_);
    test_book_.run();
    // Subscribe yourself to the smv
    for (unsigned int id = 0; id < p_smv_.size(); id++) {
      p_smv_[id]->subscribe_L1_Only(&test_book_);
    }

    // start event loop : Runs historical dispatcher, callbacks of MktUpdate, and TradePrint
    common_smv_source->Run();

    test_book_.DayOver();
    HFSAT::SecurityNameIndexer &sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
    for (unsigned int id = 0; id < p_smv_.size(); id++) {
      std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(id);
      const char *exchange_symbol_ = sec_name_indexer_.GetSecurityNameFromId(id);
      std::string exch_sym = sec_name_indexer_.GetSecurityNameFromId(id);

      std::string exp = std::to_string(HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(_this_shortcode_));
      std::string expiry_ = exp.substr(0, 4) + "-" + exp.substr(4, 2) + "-" + exp.substr(6, 2);
      int strike_price = HFSAT::NSESecurityDefinitions::GetStrikePriceFromShortCode(_this_shortcode_);
      int lotsize = HFSAT::SecurityDefinitions::GetContractMinOrderSize(_this_shortcode_, tradingdate_);
      std::cout << "Exch_sym " << exch_sym << " SYM: " << _this_shortcode_  << " ID " << id << " EXP: " << expiry_ << " SP: " << strike_price << " LOT: " << lotsize
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
      db_update_nse.UpdateDb("NSE_MTBT_MIN");
      for (int i = 0; i < ROW_SIZE; i++) {
        if (ag_no_of_msg[id][i] < 1) {
          // std::cout << _this_shortcode_ << ' ' << print_secname_ << ' ' << 0 << std::endl;
        } else {
          if (ag_min_trade_sz_[id][i] == 10000000 ) ag_min_trade_sz_[id][i] = 0;
    	  if (ag_min_px_[id][i] == 10000000 ) ag_min_px_[id][i] = 0;
          if (ag_min_spread_[id][i] == 10000000 ) ag_min_spread_[id][i] = 0;
          double range = ag_max_px_[id][i] - ag_min_px_[id][i];
          double px_stdev_ =
              sqrt(ag_avg_px2_[id][i] - ag_avg_px_[id][i] * ag_avg_px_[id][i]);
          double volume_weighted_px_stdev_ =
              sqrt(ag_volume_weighted_avg_px2_[id][i] +
                   ag_avg_px_trades_[id][i] * ag_avg_px_trades_[id][i] -
                   2 * ag_avg_px_trades_[id][i] * ag_volume_weighted_avg_px_[id][i]);
          double avg_l1_size_ = ((double)ag_total_size_[id][i] / ag_total_count_[id][i]);
          if (ag_total_count_[id][i] == 0) avg_l1_size_ = 0;
          double min_spread_ = ag_min_spread_[id][i] / ag_min_px_increment_[id][i];
          double max_spread_ = ag_max_spread_[id][i] / ag_min_px_increment_[id][i];
          double avg_spread_ = ag_avg_spread_[id][i] / ag_min_px_increment_[id][i];
/*
          std::cout << _this_shortcode_ << ' ' << print_secname_ << " timestamp " << ag_tv_sec[id][i]
                    << std::endl;
          std::cout << _this_shortcode_ << ' ' << print_secname_ << " min_px_ " << ag_min_px_[id][i]
                    << std::endl;
          std::cout << _this_shortcode_ << ' ' << print_secname_ << " max_px_ " << ag_max_px_[id][i]
                    << std::endl;
          std::cout << _this_shortcode_ << ' ' << print_secname_ << " px_range_ " << range << std::endl;
          std::cout << _this_shortcode_ << ' ' << print_secname_ << " avg_px_ " << ag_avg_px_[id][i]
                    << std::endl;
          std::cout << _this_shortcode_ << ' ' << print_secname_ << " px_stdev_ " << px_stdev_ << std::endl;
          std::cout << _this_shortcode_ << ' ' << print_secname_ << " total_volume_ "
                    << ag_total_volume_[id][i] << std::endl;
          std::cout << _this_shortcode_ << ' ' << print_secname_ << " total_trades_ "
                    << ag_total_trades_[id][i] << std::endl;
          std::cout << _this_shortcode_ << ' ' << print_secname_ << " total_buy_volume_ "
                    << ag_total_buy_volume_[id][i] << std::endl;
          std::cout << _this_shortcode_ << ' ' << print_secname_ << " total_sell_volume_ "
                    << ag_total_sell_volume_[id][i] << std::endl;
          std::cout << _this_shortcode_ << ' ' << print_secname_ << " volume_weighted_avg_px_ "
                    << ag_volume_weighted_avg_px_[i] << std::endl;
          std::cout << _this_shortcode_ << ' ' << print_secname_ << " volume_weighted_px_stdev_ "
                    << volume_weighted_px_stdev_ << std::endl;
          std::cout << _this_shortcode_ << ' ' << print_secname_ << " avg_l1_size_ " << avg_l1_size_ << std::endl;
          std::cout << _this_shortcode_ << ' ' << print_secname_ << " min_trd_sz_ "
                    << ag_min_trade_sz_[id][i] << std::endl;
          std::cout << _this_shortcode_ << ' ' << print_secname_ << " max_trd_sz_ "
                    << ag_max_trade_sz_[id][i] << std::endl;
          std::cout << _this_shortcode_ << ' ' << print_secname_ << " avg_trd_sz_ "
                    << ag_avg_trade_sz_[id][i] << std::endl;

          std::cout << _this_shortcode_ << ' ' << print_secname_ << " min_spread_ " << min_spread_ << std::endl;
          std::cout << _this_shortcode_ << ' ' << print_secname_ << " max_spread_ " << max_spread_ << std::endl;
          std::cout << _this_shortcode_ << ' ' << print_secname_ << " avg_spread_ " << avg_spread_ << std::endl;
*/
	db_update_nse.PrepareMktMinMultipleRows(
              ag_tv_sec[id][i], exch_sym, ag_min_px_[id][i], ag_max_px_[id][i], range,
              ag_avg_px_[id][i], px_stdev_, ag_total_volume_[id][i],
              ag_total_trades_[id][i], ag_total_buy_volume_[id][i],
              ag_total_sell_volume_[id][i], ag_volume_weighted_avg_px_[id][i],
              volume_weighted_px_stdev_, avg_l1_size_, ag_min_trade_sz_[id][i],
              ag_max_trade_sz_[id][i], ag_avg_trade_sz_[id][i], min_spread_, max_spread_,
              avg_spread_, _this_shortcode_, expiry_, strike_price, lotsize, ag_no_of_msg[id][i]);
        }
      }
      std::cout << "\n        ---PUSHING WHOLE DAY DATA--- " << std::endl;
      // push whole day entry
      if (test_book_.no_of_msg[id] < 1) {
        std::cout << _this_shortcode_ << ' ' << print_secname_ << ' ' << 0 << std::endl;
      } else {
        db_update_nse.ExecuteMktMinMultipleRows(); // atleast 1 row should exist
	db_update_nse.UpdateDb("NSE_MTBT");
	if (test_book_.min_trade_sz_[id] == 10000000 ) test_book_.min_trade_sz_[id] = 0;
	if (test_book_.min_px_[id] == 10000000 ) test_book_.min_px_[id] = 0;
	if (test_book_.min_spread_[id] == 10000000 ) test_book_.min_spread_[id] = 0;
        double range = test_book_.max_px_[id] - test_book_.min_px_[id];
        double px_stdev_ = sqrt(test_book_.avg_px2_[id] - test_book_.avg_px_[id] * test_book_.avg_px_[id]);
        double volume_weighted_px_stdev_ = sqrt(
            test_book_.volume_weighted_avg_px2_[id] + test_book_.avg_px_trades_[id] * test_book_.avg_px_trades_[id] -
            2 * test_book_.avg_px_trades_[id] * test_book_.volume_weighted_avg_px_[id]);
        double avg_l1_size_ = ((double)test_book_.total_size_[id] / test_book_.total_count_[id]);
	if (test_book_.total_count_[id] == 0) avg_l1_size_ = 0;
        double min_spread_ = test_book_.min_spread_[id] / test_book_.min_px_increment_[id];
        double max_spread_ = test_book_.max_spread_[id] / test_book_.min_px_increment_[id];
        double avg_spread_ = test_book_.avg_spread_[id] / test_book_.min_px_increment_[id];
/*
        std::cout << _this_shortcode_ << ' ' << print_secname_ << " min_px_ " << test_book_.min_px_[id] << std::endl;
        std::cout << _this_shortcode_ << ' ' << print_secname_ << " max_px_ " << test_book_.max_px_[id] << std::endl;
        std::cout << _this_shortcode_ << ' ' << print_secname_ << " px_range_ " << range << std::endl;
        std::cout << _this_shortcode_ << ' ' << print_secname_ << " avg_px_ " << test_book_.avg_px_[id] << std::endl;
        std::cout << _this_shortcode_ << ' ' << print_secname_ << " px_stdev_ " << px_stdev_ << std::endl;
        std::cout << _this_shortcode_ << ' ' << print_secname_ << " total_volume_ " << test_book_.total_volume_[id]
                  << std::endl;
        std::cout << _this_shortcode_ << ' ' << print_secname_ << " total_trades_ " << test_book_.total_trades_[id]
                  << std::endl;
        std::cout << _this_shortcode_ << ' ' << print_secname_ << " total_buy_volume_ "
                  << test_book_.total_buy_volume_[id] << std::endl;
        std::cout << _this_shortcode_ << ' ' << print_secname_ << " total_sell_volume_ "
                  << test_book_.total_sell_volume_[id] << std::endl;
        std::cout << _this_shortcode_ << ' ' << print_secname_ << " volume_weighted_avg_px_ "
                  << test_book_.volume_weighted_avg_px_[id] << std::endl;
        std::cout << _this_shortcode_ << ' ' << print_secname_ << " volume_weighted_px_stdev_ "
                  << volume_weighted_px_stdev_ << std::endl;
        std::cout << _this_shortcode_ << ' ' << print_secname_ << " avg_l1_size_ " << avg_l1_size_ << std::endl;
        std::cout << _this_shortcode_ << ' ' << print_secname_ << " min_trd_sz_ " << test_book_.min_trade_sz_[id]
                  << std::endl;
        std::cout << _this_shortcode_ << ' ' << print_secname_ << " max_trd_sz_ " << test_book_.max_trade_sz_
                  << std::endl;
        std::cout << _this_shortcode_ << ' ' << print_secname_ << " avg_trd_sz_ " << test_book_.avg_trade_sz_[id]
                  << std::endl;

        std::cout << _this_shortcode_ << ' ' << print_secname_ << " min_spread_ " << min_spread_ << std::endl;
        std::cout << _this_shortcode_ << ' ' << print_secname_ << " max_spread_ " << max_spread_ << std::endl;
        std::cout << _this_shortcode_ << ' ' << print_secname_ << " avg_spread_ " << avg_spread_ << std::endl;
*/
        db_update_nse.UpdateMktDaily(
            exch_sym, test_book_.min_px_[id], test_book_.max_px_[id], range, test_book_.avg_px_[id], px_stdev_,
            test_book_.total_volume_[id], test_book_.total_trades_[id], test_book_.total_buy_volume_[id],
            test_book_.total_sell_volume_[id], test_book_.volume_weighted_avg_px_[id], volume_weighted_px_stdev_,
            avg_l1_size_, test_book_.min_trade_sz_[id], test_book_.max_trade_sz_[id], test_book_.avg_trade_sz_[id],
            min_spread_, max_spread_, avg_spread_, _this_shortcode_, expiry_, strike_price, lotsize, test_book_.no_of_msg[id]);
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
