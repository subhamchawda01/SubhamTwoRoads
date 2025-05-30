// Simple Utility that shows the moving average based volume monitor for each product
// Idea is : it will startup , look at the market volumes since it started
//           compare the rate of trade (per minute ) from sampledata

#include <iostream>
#include <list>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "dvccode/CDef/debug_logger.hpp"

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/random_channel.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/sample_data_util.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/ExternalData/historical_dispatcher.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"
#include "baseinfra/MarketAdapter/indexed_hkomd_price_level_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/market_adapter_list.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_shm_processor.hpp"
#include "baseinfra/OrderRouting/prom_order_manager.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/Utils/holiday_manager_utils.hpp"
#include "dvccode/Utils/slack_utils.hpp"
#include "dvccode/Utils/thread.hpp"

#define _USE_NTP_LIVE_DATA_SOURCE_ false
#define _USE_NTP_RAW_SHM_ true

#define DEFAULT_VAL -1
#define square(x) x* x

bool _USE_CME_LIVE_DATA_SOURCE_ = false;
bool _USE_LIFFE_LIVE_DATA_SOURCE_ = false;
bool _USE_EUREX_LIVE_DATA_SOURCE_ = true;
bool _USE_CME_LIVE_SHM_DATA_SOURCE_ = false;
bool _USE_EUREX_LIVE_SHM_DATA_SOURCE_ = false;
bool _USE_HK_SHM_SOURCE_ = false;
typedef unsigned int uint32_t;
typedef std::map<int, double> UtcTimeToVolMap;
typedef std::map<int, double> UtcTimeToPStdevMap;

std::map<int, const char*> security_id_to_last_update_time_map_;

class CircularBuffer {
 private:
  std::vector<double> vec;
  uint32_t size, e;
  double s1, s2;
  double last_val, first_val;

 public:
  CircularBuffer(int n) : size(n), e(0), s1(0), s2(0), last_val(0), first_val(0){};

  void insert(double x) {
    double old_val_ = 0;
    if (vec.size() == size) {
      old_val_ = vec[e];
      vec[e] = x;
      last_val = vec[e];
      e = (e + 1) % size;
      first_val = vec[e];
    } else {
      vec.push_back(x);
      first_val = vec[0];
      last_val = vec[e];
      e = (e + 1) % size;
    }
    s1 += (x - old_val_);
    s2 += (square(x) - square(old_val_));
  }

  double Mean() {
    int n = vec.size();
    return s1 / n;
  }

  double Stdev() {
    int n = vec.size();
    return sqrt(std::max(0.0, s2 / n - (s1 * s1) / (n * n)));
  }

  double Range() {  // not really range, just last -first
    return (last_val - first_val);
  }
};

class SimpleVolumeMonitorWithMarket : public HFSAT::SecurityMarketViewChangeListener {
 public:
  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                    const HFSAT::MarketUpdateInfo& _market_update_info_);
  // void OnGlobalOrderChange ( const unsigned int _security_id_, const HFSAT::TradeType_t _buysell_,
  // 			     const int _int_price_ ) ;
  // void OnGlobalOrderExec( const unsigned int _security_id_,
  // 			  const HFSAT::TradeType_t _buysell_,
  // 			  const int _size_,
  // 			  const double _trade_px_ );

 public:
  SimpleVolumeMonitorWithMarket(HFSAT::SecurityMarketView& this_svm_t, HFSAT::DebugLogger& dbglogger_t,
                                HFSAT::Watch& watch_t, const unsigned int r_this_sid,
                                UtcTimeToVolMap& utc_time_to_vol_map, UtcTimeToPStdevMap& utc_time_to_p_stdev_map)
      : this_smv_(this_svm_t),
        dbglogger_(dbglogger_t),
        watch_(watch_t),
        traded_volume_(0),
        max_levels_(10),
        this_sid_(r_this_sid),
        prev_time_(0),
        prev_time_tp_(0),
        utc_time_to_vol_map_(utc_time_to_vol_map),
        utc_time_to_p_stdev_map_(utc_time_to_p_stdev_map),
        for_vol_avg_calc_(30),
        for_p_stdev_calc_(100),
        for_p_stdev_calc_time_(100) {
    time_stamp_last_minute_ = watch_t.msecs_from_midnight();
    this_svm_t.subscribe_tradeprints(this);
  }

  ~SimpleVolumeMonitorWithMarket() {}

  void DefLoop(unsigned int this_sid_number);
  // Doesnot do anything
  void DoNothing() { return; }
  int ComputeRunningAvg();

 private:
  HFSAT::SecurityMarketView& this_smv_;
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::Watch& watch_;
  double traded_volume_;
  int max_levels_;
  const unsigned int this_sid_;
  int prev_time_;
  int prev_time_tp_;
  UtcTimeToVolMap& utc_time_to_vol_map_;
  UtcTimeToPStdevMap& utc_time_to_p_stdev_map_;
  CircularBuffer for_vol_avg_calc_, for_p_stdev_calc_, for_p_stdev_calc_time_;
  int running_sum_;
  int running_count_;
  int time_stamp_last_minute_;
};

void SimpleVolumeMonitorWithMarket::OnMarketUpdate(const unsigned int _security_id_,
                                                   const HFSAT::MarketUpdateInfo& _market_update_info_) {
  security_id_to_last_update_time_map_[_security_id_] = watch_.time_string();

  if (watch_.msecs_from_midnight() - time_stamp_last_minute_ >= 60000) {
    // For a 30 minute window, we have a fixed 30 size list
    // for maintaining moving average
    for_vol_avg_calc_.insert(traded_volume_);
    time_stamp_last_minute_ = watch_.msecs_from_midnight();
    traded_volume_ = 0;
  }

  if (prev_time_ == 0 || (watch_.msecs_from_midnight() - prev_time_) >= 3000) {
    DefLoop(_security_id_);
    prev_time_ = watch_.msecs_from_midnight();
  }
}

void SimpleVolumeMonitorWithMarket::OnTradePrint(const unsigned int _security_id_,
                                                 const HFSAT::TradePrintInfo& _trade_print_info_,
                                                 const HFSAT::MarketUpdateInfo& _market_update_info_) {
  if (watch_.msecs_from_midnight() - time_stamp_last_minute_ >= 60000) {
    // For a 30 minute window, we have a fixed 30 size list
    // for maintaining moving average
    time_stamp_last_minute_ = watch_.msecs_from_midnight();
    for_vol_avg_calc_.insert(traded_volume_);
    traded_volume_ = 0;
  }
  for_p_stdev_calc_.insert(_trade_print_info_.trade_price_);
  for_p_stdev_calc_time_.insert(watch_.msecs_from_midnight());
  traded_volume_ += _trade_print_info_.size_traded_;
  if (prev_time_tp_ == 0 || (watch_.msecs_from_midnight() - prev_time_tp_) >= 3000) {
    OnMarketUpdate(_security_id_, _market_update_info_);
    prev_time_tp_ = watch_.msecs_from_midnight();
  }
}

int get_hhmmss(int timeval, int period, int t) {
  int time_slot = ((timeval / 1000) / period + t) * period;
  int hh = time_slot / 3600;
  time_slot -= hh * 3600;
  int mm = time_slot / 60;
  time_slot -= mm * 60;
  int ss = time_slot;
  return 10000 * hh + 100 * mm + ss;
}
int get_sec_from_hhmmss(int hhmmss) {
  int ss = hhmmss % 100;
  hhmmss /= 100;
  ss += (hhmmss % 100) * 60;
  hhmmss /= 100;
  ss += hhmmss * 3600;
  return ss;
}

// sample data SampleDataUtil::GetAvgForPeriod returns map from slot to avg_numbers instead of
// utc time to avg_numbers. The slot interval is 15  mins for sample data.
int get_utc_from_slot(int slot, int interval) {
  int total_mm = slot * interval;
  int hh = total_mm / 60;
  int mm = total_mm % 60;
  return 10000 * hh + 100 * mm;
}

void convert_to_utc_map(std::map<int, double> feature_avg_, std::map<int, double>& utc_to_feature_avg_) {
  for (auto& it : feature_avg_) {
    utc_to_feature_avg_[get_utc_from_slot(it.first, 15)] = it.second;
  }
}

void SimpleVolumeMonitorWithMarket::DefLoop(unsigned int this_sid_number) {
  printf("\033[1;1H");
  printf("Time: %s\n", watch_.time_string());
  printf("%*s %13s %13s %13s %13s %13s %13s %13s\n", 16, "SYM", "FVR", "CVR", "RATIO", "FStdevR", "CStdevR", "RATIO",
         "LastUpdate");
  printf("\033[%d;1H", 4 + this_sid_number);  // move to 4+this_sid_numberth line
  int m_m_levels = std::min(max_levels_, std::min(this_smv_.NumBidLevels(), this_smv_.NumAskLevels()));

  // Finding the correct slot in the utc_to_volume_map: 15 mins slot interval
  int hhmmss = get_hhmmss(watch_.msecs_from_midnight(), 900, 1);
  // Finding the previous time slot for accountingg for weightage
  int prev_hhmmss = get_hhmmss(watch_.msecs_from_midnight(), 900, 0);
  int execess_minutes = (int)(((int)watch_.msecs_from_midnight() / 1000 - get_sec_from_hhmmss(prev_hhmmss)) / 60);
  int vol_rate_per_minute = -1;
  if (utc_time_to_vol_map_.find(hhmmss) != utc_time_to_vol_map_.end()) {
    // weightage
    if (utc_time_to_vol_map_.find(prev_hhmmss) != utc_time_to_vol_map_.end()) {
      vol_rate_per_minute = (execess_minutes * (utc_time_to_vol_map_[hhmmss] / 15) +
                             (15 - execess_minutes) * (utc_time_to_vol_map_[prev_hhmmss] / 15)) /
                            5;  // vol_avg is actually avg of 5 mins in this 15 min interval
    } else
      vol_rate_per_minute = utc_time_to_vol_map_[hhmmss] / 5;
  }

  // Avoid division by zero later
  if (vol_rate_per_minute == 0) vol_rate_per_minute = 1;

  // =-=-=-=-=--=-=-=-=-=-=-=--=-= NOW for PRICE =-=-=-=-=--=-=-=--=-=-=-=--=-=
  // Finding the correct slot in the utc_to_volume_map
  hhmmss = get_hhmmss(watch_.msecs_from_midnight(), 900, 1);  // price stdev is for 15 minutes
  // Finding the previous time slot for weightage
  prev_hhmmss = get_hhmmss(watch_.msecs_from_midnight(), 900, 0);
  execess_minutes = (int)(((int)watch_.msecs_from_midnight() / 1000 - get_sec_from_hhmmss(prev_hhmmss)) / 60);
  double p_stdev_avg_ = -1.0;
  if (utc_time_to_p_stdev_map_.find(hhmmss) != utc_time_to_p_stdev_map_.end()) {
    p_stdev_avg_ = utc_time_to_p_stdev_map_[hhmmss];
  }
  if (p_stdev_avg_ == 0.0) p_stdev_avg_ = -1.0;
  //  int t_level_ = 0; // Max Bid and Min Ask
  if (m_m_levels >= 1 && std::min(this_smv_.NumBidLevels(), this_smv_.NumAskLevels()) >= 1) {
    double stdev_scaling_factor =
        sqrt(900 / (for_p_stdev_calc_time_.Range() / 1000));  // (900 secs) / (#secs b/w our first and last updates)

    std::string secname = std::string(this_smv_.secname());
    // This is done show readable names in case of NSE. Instead of NSE3669 etc., we want to show
    // NSE_NIFTY_FUT_20150827 etc.
    if (this_smv_.exch_source() == HFSAT::kExchSourceNSE) {
      HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
      secname = sec_name_indexer_.GetShortcodeFromId(this_smv_.security_id());

      if (std::string::npos != secname.find("NSE_")) {
        secname = secname.substr(secname.find("NSE_") + std::string("NSE_").length());
      }
    }
    switch (this_smv_.secname()[0]) {
      case 'Z':
      case 'U': {
        //%11.7f
        printf("%*s %13d %13d %13.2f %13.5f %13.5f %13.5f %13s\n",
               16,  // 13 Width for the Symbol
               secname.c_str(),

               (int)vol_rate_per_minute, (int)for_vol_avg_calc_.Mean(),
               (float)for_vol_avg_calc_.Mean() / vol_rate_per_minute,

               p_stdev_avg_,  // historical std dev of price in last 15 mins = 900 secs
               for_p_stdev_calc_.Stdev() * stdev_scaling_factor,
               for_p_stdev_calc_.Stdev() * stdev_scaling_factor / p_stdev_avg_,

               security_id_to_last_update_time_map_[this_sid_number]);
      } break;
      default: {
        printf("%*s %13d %13d %13.2f %13.5f %13.5f %13.5f %13s\n",
               16,  // 13 Width for the Symbol
               secname.c_str(),

               (int)vol_rate_per_minute, (int)for_vol_avg_calc_.Mean(),
               (float)for_vol_avg_calc_.Mean() / vol_rate_per_minute,

               p_stdev_avg_, for_p_stdev_calc_.Stdev() * stdev_scaling_factor,
               for_p_stdev_calc_.Stdev() * stdev_scaling_factor / p_stdev_avg_,

               security_id_to_last_update_time_map_[this_sid_number]);

      } break;
    }
  }
}

void SendSlackNotif(std::string alert_message) {
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);
  alert_message = alert_message + std::string(hostname);
  HFSAT::SlackManager* slack_manager = new HFSAT::SlackManager(NSE_PRODISSUES);
  slack_manager->sendNotification(alert_message);
}

void PrintMap(std::map<int, double>& map_to_print) {
  std::map<int, double>::const_iterator it = map_to_print.begin();
  for (it = map_to_print.begin(); it != map_to_print.end(); it++) {
    std::cerr << "TIME: " << it->first << " AVG: " << it->second << std::endl;
  }
}

bool CheckAndAddNSEDefinitions(std::vector<std::string>& t_shortcode_vec_) {
  bool is_nse_present_ = false;
  for (auto i = 0u; i < t_shortcode_vec_.size(); i++) {
    std::cout << " Shc:" << t_shortcode_vec_[i] << "\n";
    if (strncmp(t_shortcode_vec_[i].c_str(), "NSE_", 4) == 0) {
      is_nse_present_ = true;
    }
  }
  if (is_nse_present_) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal())
        .LoadNSESecurityDefinitions();
  }

  return is_nse_present_;
}

int main(int argc, char** argv) {
  std::vector<std::string> sec_list_vec;
  std::vector<const char*> exchange_symbol_vec;
  if (argc < 2) {
    std::cerr << " usage : Input file name containing the Symbols E.g ~/infracore_install/files/mkt_sec.txt "
              << std::endl;
    exit(0);
  }

  int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();

  struct timeval current_time;
  gettimeofday(&current_time, NULL);

  // Make Sure This Is Done Above All Other Classe's Initialization, Needed For ASX TICK Changes
  HFSAT::SecurityDefinitions::ResetASXMpiIfNeeded(tradingdate_, (time_t)current_time.tv_sec);

  std::string filename_input(argv[1]);

  security_id_to_last_update_time_map_.clear();

  /////////FILE INPUT OF SHORTCODES///////////
  char line[1024];
  std::ifstream sec_file_;
  sec_file_.open(filename_input.c_str(), std::ifstream::in);
  if (!sec_file_.is_open()) {
    std::cerr << filename_input << "  FILE DOESNOT EXIST " << std::endl;
    exit(-1);
  }

  while (!sec_file_.eof()) {
    bzero(line, 1024);
    sec_file_.getline(line, 1024);
    if (strstr(line, "#") || strlen(line) == 0) continue;
    HFSAT::PerishableStringTokenizer st(line, 1024);
    const std::vector<const char*>& tokens = st.GetTokens();
    if (tokens.size() < 1) {
      std::cerr << " Bad file..See #entries " << std::endl;
      exit(-1);
    }
    std::cerr << " SEC_SHORTCODE " << tokens[0] << std::endl;
    sec_list_vec.push_back(std::string(tokens[0]));
  }
  ////////@end FILE INPUT////////////////
  bool is_nse_added = CheckAndAddNSEDefinitions(sec_list_vec);

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);

  //// CONSULT AVERAGE VOLUME FILE/////////////
  std::map<std::string, UtcTimeToVolMap> symbol_to_utc_time_vol_map_map;
  std::map<std::string, UtcTimeToPStdevMap> symbol_to_utc_time_p_stdev_map_map;

  for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++) {
    if (symbol_to_utc_time_vol_map_map.find(sec_list_vec[ii]) == symbol_to_utc_time_vol_map_map.end()) {
      UtcTimeToVolMap utc_time_vol_map;
      UtcTimeToPStdevMap utc_time_to_p_stdev_map;

      UtcTimeToVolMap slot_to_feature_map;
      // use sample data to fetch avg volume and pstdev
      // the maps  utc_time_vol_map & utc_time_to_p_stdev_map have slot number to
      // avg/stdev mapping. Convert slots it to actual utc using get_utc_from_slot()
      HFSAT::SampleDataUtil::GetAvgForPeriod(sec_list_vec[ii], tradingdate_, 20, 0, 86400000, "VOL",
                                             slot_to_feature_map, false);
      convert_to_utc_map(slot_to_feature_map, utc_time_vol_map);
      slot_to_feature_map.clear();

      // also convert the avg_stdev map to utc keys
      HFSAT::SampleDataUtil::GetAvgForPeriod(sec_list_vec[ii], tradingdate_, 20, 0, 86400000, "STDEV",
                                             slot_to_feature_map, false);
      convert_to_utc_map(slot_to_feature_map, utc_time_to_p_stdev_map);
      slot_to_feature_map.clear();

      symbol_to_utc_time_vol_map_map[sec_list_vec[ii]] = utc_time_vol_map;
      symbol_to_utc_time_p_stdev_map_map[sec_list_vec[ii]] = utc_time_to_p_stdev_map;

    } else {
      std::cerr << "Not loading Avg volume file for symbol " << sec_list_vec[ii] << " ..already loaded??" << std::endl;
    }
    // PrintMap ( symbol_to_utc_time_vol_map_map[sec_list_vec[ii]]);
  }

  //// END CONSULTING VOLUME FILE////////////

  HFSAT::TradingLocation_t curr_location_ = HFSAT::TradingLocationUtils::GetTradingLocationFromHostname();
  if (curr_location_ == HFSAT::kTLocM1 || curr_location_ == HFSAT::kTLocSYD) {
    _USE_CME_LIVE_DATA_SOURCE_ = true;
    _USE_LIFFE_LIVE_DATA_SOURCE_ = true;
    _USE_CME_LIVE_SHM_DATA_SOURCE_ = true;
    _USE_EUREX_LIVE_SHM_DATA_SOURCE_ = true;
  }
  if (curr_location_ == HFSAT::kTLocHK) {
    _USE_CME_LIVE_DATA_SOURCE_ = true;
    _USE_LIFFE_LIVE_DATA_SOURCE_ = true;
    _USE_EUREX_LIVE_DATA_SOURCE_ = true;
    _USE_EUREX_LIVE_SHM_DATA_SOURCE_ = true;
  }
  if (curr_location_ == HFSAT::kTLocJPY) {
    _USE_HK_SHM_SOURCE_ = false;
    _USE_CME_LIVE_DATA_SOURCE_ = true;
    _USE_LIFFE_LIVE_DATA_SOURCE_ = true;
    _USE_EUREX_LIVE_DATA_SOURCE_ = true;
    _USE_EUREX_LIVE_SHM_DATA_SOURCE_ = true;
  }

  for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++)
    exchange_symbol_vec.push_back(HFSAT::ExchangeSymbolManager::GetExchSymbol(sec_list_vec[ii]));

  HFSAT::DebugLogger dbglogger_(1024000, 1);
  // setup DebugLogger
  {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/logs/alllogs/volume_monitor." << HFSAT::DateTime::GetCurrentIsoDateLocal();
    std::string logfilename_ = t_temp_oss_.str();
    dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out);
  }
  // dbglogger_.AddLogLevel ( BOOK_ERROR );
  dbglogger_.AddLogLevel(BOOK_INFO);
  // dbglogger_.AddLogLevel ( BOOK_TEST );

  HFSAT::Watch watch_(dbglogger_, tradingdate_);

  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++)
    sec_name_indexer_.AddString(exchange_symbol_vec[ii], sec_list_vec[ii]);

  HFSAT::SimpleLiveDispatcher simple_live_dispatcher_;

  HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_ = HFSAT::sid_to_security_market_view_map();
  HFSAT::ShortcodeSecurityMarketViewMap& shortcode_smv_map_ =
      HFSAT::ShortcodeSecurityMarketViewMap::GetUniqueInstance();

  for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++) {
    HFSAT::ExchSource_t _this_exch_source_ =
        HFSAT::SecurityDefinitions::GetContractExchSource(sec_list_vec[ii], tradingdate_);
    int _this_sid_ = sec_name_indexer_.GetIdFromString(sec_list_vec[ii]);

    if (_this_exch_source_ == HFSAT::kExchSourceEUREX) {
      _this_exch_source_ = HFSAT::kExchSourceEOBI;
    }

    bool set_temporary_bool_checking_if_this_is_an_indexed_book_ =
        HFSAT::CommonSimIndexedBookBool(_this_exch_source_, sec_list_vec[ii], curr_location_);

    HFSAT::SecurityMarketView* p_smv_ = new HFSAT::SecurityMarketView(
        dbglogger_, watch_, sec_name_indexer_, sec_list_vec[ii], exchange_symbol_vec[ii], _this_sid_,
        _this_exch_source_, set_temporary_bool_checking_if_this_is_an_indexed_book_);
    std::cerr << " SID : for SMV " << _this_sid_ << std::endl;
    sid_to_smv_ptr_map_.push_back(p_smv_);                  // add to security_id_ to SMV* map
    shortcode_smv_map_.AddEntry(sec_list_vec[ii], p_smv_);  // add to shortcode_ to SMV* map
  }

  HFSAT::IndexedLiffePriceLevelMarketViewManager indexed_liffe_price_level_market_view_manager_(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);
  HFSAT::IndexedIceMarketViewManager indexed_ice_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::HKEXIndexedMarketViewManager indexed_hkex_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                        sid_to_smv_ptr_map_);
  HFSAT::OSEOrderLevelMarketViewManager ose_order_level_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                             sid_to_smv_ptr_map_);
  HFSAT::OSEL1PriceMarketViewManager ose_l1_price_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                       sid_to_smv_ptr_map_);
  HFSAT::IndexedEobiPriceLevelMarketViewManager indexed_eobi_price_feed_market_view_manager_(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);
  HFSAT::IndexedCmeMarketViewManager indexed_cme_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedNtpMarketViewManager indexed_ntp_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedNtpMarketViewManager indexed_puma_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                       sid_to_smv_ptr_map_);
  HFSAT::IndexedRtsMarketViewManager indexed_rts_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedMicexMarketViewManager indexed_micex_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                          sid_to_smv_ptr_map_);
  HFSAT::IndexedOsePriceFeedMarketViewManager indexed_ose_price_feed_market_view_manager_(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);
  HFSAT::IndexedCfeMarketViewManager indexed_cfe_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedTmxMarketViewManager indexed_tmx_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedHKOMDPriceLevelMarketViewManager indexed_hkomd_price_level_market_view_manager(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);

  HFSAT::IndexedNSEMarketViewManager indexed_nse_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::MDSMessages::CombinedMDSMessagesShmProcessor combined_mds_messages_shm_processor_(
      dbglogger_, sec_name_indexer_, HFSAT::kComShmConsumer);
  bool USE_COMBINED_SOURCE = true;

  // Reading from CombinedShmWriter for BMF, CFE and MOS locations
  if (curr_location_ == HFSAT::kTLocBMF || curr_location_ == HFSAT::kTLocCFE || curr_location_ == HFSAT::kTLocM1 ||
      curr_location_ == HFSAT::kTLocSYD || curr_location_ == HFSAT::kTLocNSE) {
    USE_COMBINED_SOURCE = true;
  }

  for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++) {
    sid_to_smv_ptr_map_[ii]->market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_ = true;
    sid_to_smv_ptr_map_[ii]->InitializeSMVForIndexedBook();
  }

  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::CME, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_cme_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::CME_LS, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_cme_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::EUREX, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_cme_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::EOBI_PF,
      (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_eobi_price_feed_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::EOBI_LS,
      (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_eobi_price_feed_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::EUREX_LS, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_cme_market_view_manager_)),
      &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::ICE, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ice_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::ASX, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ice_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::LIFFE,
      (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_liffe_price_level_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::LIFFE_LS,
      (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_liffe_price_level_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::RTS, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_cme_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::MICEX, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_micex_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::NTP, (void*)((HFSAT::NTPPriceLevelGlobalListener*)(&indexed_ntp_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::BMF_EQ, (void*)((HFSAT::NTPPriceLevelGlobalListener*)(&indexed_ntp_market_view_manager_)),
      &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::OSE_CF, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ose_price_feed_market_view_manager_)),
      &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::HKOMDPF,
      (void*)((HFSAT::HKHalfBookGlobalListener*)(&indexed_hkomd_price_level_market_view_manager)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::CSM, (void*)((HFSAT::CFEPriceLevelGlobalListener*)&(indexed_cfe_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::TMX, (void*)((HFSAT::FullBookGlobalListener*)(&indexed_tmx_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::TMX_LS, (void*)((HFSAT::FullBookGlobalListener*)(&indexed_tmx_market_view_manager_)), &watch_);

  if (true == is_nse_added) {
    combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
        HFSAT::MDS_MSG::NSE, (void*)((HFSAT::OrderGlobalListenerNSE*)&(indexed_nse_market_view_manager_)), &watch_);
  }
  printf("\033c");      // clear screen
  printf("\033[1;1H");  // move to this_sid_number line
  printf("Time: %s\n", watch_.time_string());
  printf("%*s %13s %13s %13s %13s %13s %13s %13s\n", 16, "SYM", "FVR", "CVR", "RATIO", "FStdevR", "CStdevR", "RATIO",
         "LastUpdate");

  for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++) {
    const unsigned int _this_sid_ = (unsigned int)sec_name_indexer_.GetIdFromString(sec_list_vec[ii]);

    std::string _this_shortcode_ = sec_list_vec[ii];

    SimpleVolumeMonitorWithMarket* simple_ba_price_with_market_price_ = new SimpleVolumeMonitorWithMarket(
        *(shortcode_smv_map_.GetSecurityMarketView(_this_shortcode_)), dbglogger_, watch_, _this_sid_,
        symbol_to_utc_time_vol_map_map[sec_list_vec[ii]], symbol_to_utc_time_p_stdev_map_map[sec_list_vec[ii]]);
    // Dummy function to remove unused variable warning
    simple_ba_price_with_market_price_->DoNothing();
  }

  if (USE_COMBINED_SOURCE) {
    combined_mds_messages_shm_processor_.RunLiveShmSource();
  } else {
    try {
      simple_live_dispatcher_.RunLive();
    } catch (int e) {
    }
  }

  return 0;
}
