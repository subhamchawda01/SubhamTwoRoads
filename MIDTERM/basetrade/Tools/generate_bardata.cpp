/**
   \file Tools/mkt_trade_logger.cpp

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

struct Quote {
  unsigned int bid_ords_;
  unsigned int bid_sz_;
  float bid_px_;
  float ask_px_;
  unsigned int ask_sz_;
  unsigned int ask_ords_;

  Quote() : bid_ords_(0), bid_sz_(0), bid_px_(0.0), ask_px_(0.0), ask_sz_(0), ask_ords_(0) {}
  void UpdateFromMktUpdateInfo(const HFSAT::MarketUpdateInfo &_mkt_update_info_) {
    bid_ords_ = _mkt_update_info_.bestbid_ordercount_;
    ask_ords_ = _mkt_update_info_.bestask_ordercount_;
    bid_px_ = _mkt_update_info_.bestbid_price_;
    ask_px_ = _mkt_update_info_.bestask_price_;
    bid_sz_ = _mkt_update_info_.bestbid_size_;
    ask_sz_ = _mkt_update_info_.bestask_size_;
  }
  std::string ToString() {
    std::stringstream ss;
    ss << "[ " << bid_sz_ << " " << bid_ords_ << " " << bid_px_ << " X " << ask_px_ << " " << ask_ords_ << " "
       << ask_sz_ << " ]";
    return ss.str();
  }
};

struct DataBar {
  unsigned int tv_sec_;
  unsigned int dur_in_secs_;
  // Quote open_qt_;
  // Quote close_qt_;
  float open_bid_px_;
  float open_ask_px_;
  float close_bid_px_;
  float close_ask_px_;
  float high_px_;
  float low_px_;
  unsigned int vol_trd_;
  float avg_trd_px_;
  unsigned int num_events_;
  unsigned int num_trds_;

  DataBar(unsigned int _dur_)
      : tv_sec_(0),
        dur_in_secs_(_dur_),
        open_bid_px_(0.0),
        open_ask_px_(0.0),
        close_bid_px_(0.0),
        close_ask_px_(0.0),
        high_px_(0.0),
        low_px_(0.0),
        vol_trd_(0),
        avg_trd_px_(0.0),
        num_events_(0),
        num_trds_(0) {}

  void ResetFromMktUpdateInfo(const HFSAT::Watch &_watch_, const HFSAT::MarketUpdateInfo &_mkt_update_info_, int gap) {
    // tv_sec_ = dur_in_secs_ * (_watch_.tv().tv_sec / dur_in_secs_);
    // open_qt_.UpdateFromMktUpdateInfo(_mkt_update_info_);
    // close_qt_.UpdateFromMktUpdateInfo(_mkt_update_info_);
    open_bid_px_ = _mkt_update_info_.bestbid_price_;
    open_ask_px_ = _mkt_update_info_.bestask_price_;

    close_bid_px_ = _mkt_update_info_.bestbid_price_;
    close_ask_px_ = _mkt_update_info_.bestask_price_;

    high_px_ = _mkt_update_info_.mid_price_;
    low_px_ = _mkt_update_info_.mid_price_;
    vol_trd_ = 0;
    avg_trd_px_ = _mkt_update_info_.mid_price_;
    if (gap > 1) num_events_ = 0;
    num_trds_ = 0;
  }

  void UpdateFromMktUpdateInfo(const HFSAT::Watch &_watch_, const HFSAT::MarketUpdateInfo &_mkt_update_info_,
                               unsigned int last_secs_recorded_) {
    // tv_sec_ = dur_in_secs_ * (_watch_.tv().tv_sec / dur_in_secs_);
    tv_sec_ = last_secs_recorded_;
    // close_qt_.UpdateFromMktUpdateInfo(_mkt_update_info_);
    close_ask_px_ = _mkt_update_info_.bestask_price_;
    close_bid_px_ = _mkt_update_info_.bestbid_price_;

    high_px_ = std::max(high_px_, float(_mkt_update_info_.mid_price_));
    low_px_ = std::min(low_px_, float(_mkt_update_info_.mid_price_));
    num_events_++;
  }

  void UpdateFromTradeInfo(const HFSAT::Watch &_watch_, const HFSAT::TradePrintInfo &_trd_print_info_,
                           unsigned int last_secs_recorded_) {
    // tv_sec_ = dur_in_secs_ * (_watch_.tv().tv_sec / dur_in_secs_);
    tv_sec_ = last_secs_recorded_;
    avg_trd_px_ = (vol_trd_ * avg_trd_px_ + _trd_print_info_.size_traded_ * _trd_print_info_.trade_price_) /
                  (vol_trd_ + _trd_print_info_.size_traded_);
    vol_trd_ += _trd_print_info_.size_traded_;
    num_trds_++;
  }

  void IncreaseOneStep(int current_interval = false) {
    if (current_interval) num_events_ = 1;
    tv_sec_ += dur_in_secs_;
  }

  std::string ToString() {
    std::stringstream ss;
    ss << tv_sec_
       //<< " ,Open: " << open_qt_.ToString() << " ,Close: " << close_qt_.ToString()
       << " ,Open: [ " << open_bid_px_ << " " << open_ask_px_ << " ] ,Close: [ " << close_bid_px_ << " "
       << close_ask_px_ << " ],High: " << high_px_ << " ,Low: " << low_px_ << " ,Vol: " << vol_trd_
       << " ,Avg_Trd_Px: " << avg_trd_px_ << " ,Events: " << num_events_ << " ,Trades: " << num_trds_ << "\n";
    return ss.str();
  }
};

class TestBook : public HFSAT::SecurityMarketViewChangeListener {
  HFSAT::SecurityMarketView *this_smv_;
  HFSAT::Watch &watch_;

  int start_unix_time_;
  int end_unix_time_;
  unsigned int timeout_secs_;
  unsigned int last_secs_recorded_;
  DataBar data_bar_;
  HFSAT::BulkFileWriter writer_;
  std::string filename_;
  int dump_count_;

 public:
  TestBook(HFSAT::SecurityMarketView *_this_smv_, HFSAT::Watch &_watch_, const int start_unix_time_,
           const int end_unix_time_, const int _timeout_secs_, const std::string &_filename_)
      : this_smv_(_this_smv_),
        watch_(_watch_),
        start_unix_time_(start_unix_time_ * 1000),
        end_unix_time_(end_unix_time_ * 1000),
        timeout_secs_(_timeout_secs_),
        last_secs_recorded_(0),
        data_bar_(_timeout_secs_),
        writer_(_filename_),
        filename_(_filename_),
        dump_count_(0) {
    this_smv_->subscribe_price_type(this, HFSAT::kPriceTypeMktSizeWPrice);
    this_smv_->subscribe_tradeprints(this);
    if (!writer_.is_open()) {
      std::cerr << "Can't open " << _filename_ << " for writing\n";
      exit(0);
    }
  }

  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo &_market_update_info_) {
    if (!IsPostStartTime()) {
      return;
    }

    if (IsPostEndTime()) {
      DayOver();
      exit(0);
    }

    if (last_secs_recorded_ == 0 || watch_.tv().tv_sec >= int(last_secs_recorded_ + timeout_secs_)) {
      if (last_secs_recorded_ > 0) {
        DumpDataBar();
      }

      int freq = (watch_.tv().tv_sec - last_secs_recorded_) / timeout_secs_;
      data_bar_.ResetFromMktUpdateInfo(watch_, _market_update_info_, freq);
      if (freq >= 1 && last_secs_recorded_ > 0) {
        // if multiple minutes have passed since last update, dump that many minute bars with resetted values
        for (auto i = 0; i < freq; i++) {
          if (i < (freq - 1)) {
            data_bar_.IncreaseOneStep();
            DumpDataBar();
          } else {
            data_bar_.IncreaseOneStep();
          }
        }
      }
      // Not required, but reconcile
      last_secs_recorded_ =
          (timeout_secs_ * ((watch_.tv().tv_sec - start_unix_time_) / timeout_secs_)) + start_unix_time_;
      // last_secs_recorded_ = watch_.tv().tv_sec;
    } else {
      data_bar_.UpdateFromMktUpdateInfo(watch_, _market_update_info_, last_secs_recorded_);
    }
  }

  inline void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo &_trade_print_info_,
                           const HFSAT::MarketUpdateInfo &_market_update_info_) {
    if (!IsPostStartTime()) {
      return;
    }

    OnMarketUpdate(_security_id_, _market_update_info_);
    data_bar_.UpdateFromTradeInfo(watch_, _trade_print_info_, last_secs_recorded_);
  }

  inline bool IsPostStartTime() {
    return (start_unix_time_ < 1 || (int)watch_.msecs_from_midnight() > start_unix_time_);
  }

  inline bool IsPostEndTime() { return (start_unix_time_ >= 1 && (int)watch_.msecs_from_midnight() > end_unix_time_); }

  inline void DumpDataBar() {
    writer_.Write(&data_bar_, sizeof(data_bar_));
    writer_.CheckToFlushBuffer();
    dump_count_++;
  }

  inline void DayOver() {
    if (last_secs_recorded_ > 0) {
      DumpDataBar();
    }
    writer_.Close();
    std::cout << "# of DataBars Dumped: " << dump_count_ << std::endl;
    int size_;
    if (!HFSAT::FileUtils::ExistsWithSize(filename_, size_)) {
      std::remove(filename_.c_str());
    }
  }
};

int main(int argc, char **argv) {
  // Assume that we get the filename of a file that only has all logged EBS data pertaining to one symbol.
  // Load up the File Source

  if (argc < 5 && argc != 2) {
    std::cerr << "$EXEC SIM SHORTCODE TRADINGDATE OUTDIR [TIMEOUT_SECS=60] [START_TIME=DEF] [END_TIME=DEF] "
                 "[PL/NTP/NTP_ORD/BMF_EQ/QUINCY/HKOMD/HKOMD_PF/HKOMD_CPF/CHIXORD] (to generate)\n";
    std::cerr << "or $EXEC <filename> (to read)\n";
    exit(0);
  }

  if (argc == 2) {
    // Read mode
    HFSAT::BulkFileReader reader;
    DataBar next_event(0);
    reader.open(argv[1]);
    if (reader.is_open()) {
      while (true) {
        size_t available_len = reader.read(&next_event, sizeof(next_event));
        if (available_len < sizeof(next_event)) break;
        std::cout << next_event.ToString();
      }
      reader.close();
    }
    exit(0);
  }

  bool isNTP = (argc >= 4 && strcmp(argv[argc - 1], "NTP") == 0);
  bool isNTPORD = (argc >= 4 && strcmp(argv[argc - 1], "NTP_ORD") == 0);
  bool isBMFEq = (argc >= 4 && strcmp(argv[argc - 1], "BMF_EQ") == 0);
  bool isPL = (argc >= 4 && strcmp(argv[argc - 1], "PL") == 0);
  bool isQuincy = (argc >= 4 && strcmp(argv[argc - 1], "QUINCY") == 0);
  bool isOMD = (argc >= 4 && strcmp(argv[argc - 1], "HKOMD") == 0);
  bool isOMD_PF = (argc >= 4 && strcmp(argv[argc - 1], "HKOMD_PF") == 0);
  bool isOMD_CPF = (argc >= 4 && strcmp(argv[argc - 1], "HKOMD_CPF") == 0);
  bool isCHIXORD = (argc >= 4 && strcmp(argv[argc - 1], "CHIXORD") == 0);
  bool set_book_type_ = false;
  if (isNTP || isNTPORD || isBMFEq || isQuincy || isOMD || isOMD_PF || isOMD_CPF || isCHIXORD) {
    --argc;
    set_book_type_ = true;
  }

  bool livetrading_ = false;
  int read_idx_ = 2;
  std::string _this_shortcode_ = argv[read_idx_++];

  int tradingdate_ = 0;

  // bool non_self_book_enabled_ = true;

  int start_unix_time_ = 0;
  int end_unix_time_ = 0;
  int timeout_secs_ = 60;
  std::string network_account_info_filename_ = HFSAT::FileUtils::AppendHome(
      std::string(BASESYSINFODIR) + "TradingInfo/NetworkInfo/network_account_info_filename.txt");

  tradingdate_ = atoi(argv[read_idx_++]);
  if ((tradingdate_ < MIN_YYYYMMDD) || (tradingdate_ > MAX_YYYYMMDD)) {
    std::cerr << tradingdate_ << " not logical date" << std::endl;
    exit(0);
  }
  std::string outdir_ = argv[read_idx_++];

  if (argc >= read_idx_ + 1) {
    if (strncmp(argv[read_idx_], "EVT_", 4) == 0) {
      timeout_secs_ = atoi(argv[read_idx_] + 4);
    } else {
      timeout_secs_ = atoi(argv[read_idx_]);
    }
    read_idx_++;
  }

  if (argc >= read_idx_ + 2) {
    if (strcmp(argv[read_idx_], "DEF") != 0) {
      start_unix_time_ = HFSAT::GetSecondsFromHHMM(
          HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(argv[read_idx_] + 4), argv[6]));
    }
    if (strcmp(argv[read_idx_ + 1], "DEF") != 0) {
      end_unix_time_ = HFSAT::GetSecondsFromHHMM(
          HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(argv[read_idx_ + 1] + 4), argv[7]));
    }
    read_idx_ += 2;
  }

  std::vector<std::string> shortcode_list_;
  shortcode_list_.push_back(_this_shortcode_);

  // --- Make object of CommonSMVSource and use it as an API -------------------------------------------
  CommonSMVSource *common_smv_source = new CommonSMVSource(shortcode_list_, tradingdate_, livetrading_);

  // Set the required book type and network info filename
  if (set_book_type_) {
    common_smv_source->SetBookType(isNTP, isNTPORD, isBMFEq, isPL, isQuincy, true, isOMD_PF, isOMD_CPF, false,
                                   isCHIXORD);
  }
  common_smv_source->SetNetworkAccountInfoFilename(network_account_info_filename_);

  // Initialize the smv source after setting the required variables
  common_smv_source->Initialize();

  // Get the smv and watch after creating the source
  HFSAT::SecurityMarketView *p_smv_ = common_smv_source->getSMV();
  HFSAT::Watch &watch_ = common_smv_source->getWatch();

  // Fetch exchange symbol for file naming
  if (strncmp(_this_shortcode_.c_str(), "NSE_", 4) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();
  } else if (_this_shortcode_.substr(0, 3) == "HK_") {
    HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadHKStocksSecurityDefinitions();
  }
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  std::string exch_symbol = HFSAT::ExchangeSymbolManager::GetExchSymbol(_this_shortcode_);
  if (exch_symbol.length() <= 0) {
    std::cerr << "Could not get valid exch symbol for " << exch_symbol << ". Exiting!\n";
    exit(1);
  }
  std::replace(exch_symbol.begin(), exch_symbol.end(), ' ', '~');

  std::ostringstream temp_oss_;
  temp_oss_ << outdir_ << "/MB_" << exch_symbol << "_" << tradingdate_;
  TestBook test_book_(p_smv_, watch_, start_unix_time_, end_unix_time_, timeout_secs_, temp_oss_.str());

  // start event loop : Runs historical dispatcher, callbacks of MktUpdate, and TradePrint
  common_smv_source->Run();

  test_book_.DayOver();

  return 0;
}
