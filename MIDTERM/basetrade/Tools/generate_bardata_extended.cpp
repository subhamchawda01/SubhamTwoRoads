/**
   \file Tools/generate_bardata_extended.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
 */

#include "baseinfra/Tools/common_smv_source.hpp"
#include "dvccode/CDef/file_utils.hpp"

#include <inttypes.h>

#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 22201225
#define MAX_SYMBOL_LEN 20

/*
this tool us used to generate synchronized bardata using tick_data files.
We intend to store this data in DB
this data is used for generating seperate trading signals for longer duration ( a defined risk per trade ), usually 
these signals are expected to be more stable and works across regimes 
*/

/*
Required fields are divided into 4 parts
a) Reference Fields (which are used to synchronize data across products) 
b) PriceInfo ( these are used as input transform function / signal functions )
c) SanityChecks ( which data to ignore / normal-time, specifically at begin when spread is high )
d) ExecutionInfo ( with certain probably these variables can be used for execution probability and cost )
*/

struct DataBar {
  int bar_length_in_secs_;

  // reference and counters
  unsigned int unix_time_in_secs_;
  unsigned int num_events_;

  // for signals
  double open_price_;
  double low_price_;
  double high_price_;
  double close_price_;

  // for cleaning data
  int avg_bas_in_ticks_;

  // for execution cost estimations and checks
  double vwap_;
  int num_trades_;
  int volume_;

  char ex_sym_[MAX_SYMBOL_LEN];

  DataBar(){}

  DataBar(unsigned int _dur_, unsigned int _init_time_in_secs_, const char* _exchange_symbol_)
    : bar_length_in_secs_(_dur_),
      unix_time_in_secs_(_init_time_in_secs_), // we start with init time and step up for each dump call
      num_events_(0), // each dump will resets this values
      open_price_(0.0), // time range is [>=t <t+step], this need a reset as well
      low_price_(0.0),
      high_price_(0.0),
      close_price_(0.0),
      avg_bas_in_ticks_(1),
      vwap_(0),
      num_trades_(0),
      volume_(0) {
    if (strlen(_exchange_symbol_) <= sizeof(ex_sym_)) {
      strncpy(ex_sym_, _exchange_symbol_, MAX_SYMBOL_LEN);      
    } else {
      std::cerr << "exchange symbol length is more than defined 20 limit\n";
      exit(-1);
    }

    unix_time_in_secs_ = (_init_time_in_secs_ / bar_length_in_secs_) * (bar_length_in_secs_);
  }
/*
whenever we hit bar-boundary we reset values
*/
  void ResetFromMktUpdateInfo(const HFSAT::Watch &_watch_, const HFSAT::MarketUpdateInfo &_mkt_update_info_, int gap) {

    if (unix_time_in_secs_ <= 0) {
      unix_time_in_secs_ = (_watch_.tv().tv_sec / bar_length_in_secs_) * bar_length_in_secs_;
    }
    open_price_ = _mkt_update_info_.mid_price_;
    close_price_ = _mkt_update_info_.mid_price_;
    low_price_ = _mkt_update_info_.mid_price_;
    high_price_ = _mkt_update_info_.mid_price_;
    avg_bas_in_ticks_ = _mkt_update_info_.spread_increments_;
    num_events_ = 0;
    num_trades_ = 0;
    vwap_ = 0;
    volume_ = 0;
  }
/*
inside the bar we simply update variables 
*/
  void UpdateFromMktUpdateInfo(const HFSAT::Watch &_watch_, const HFSAT::MarketUpdateInfo &_mkt_update_info_,
                               unsigned int last_secs_recorded_) {

    close_price_ = _mkt_update_info_.mid_price_;
    high_price_ = std::max(high_price_, double(_mkt_update_info_.mid_price_));
    low_price_ = std::min(low_price_, double(_mkt_update_info_.mid_price_));
    avg_bas_in_ticks_ = (num_events_ * avg_bas_in_ticks_ + _mkt_update_info_.spread_increments_) /
      (num_events_ + 1);
    num_events_++;
  }
/*
inside the bar execution related info
*/
  void UpdateFromTradeInfo(const HFSAT::Watch &_watch_, const HFSAT::TradePrintInfo &_trd_print_info_,
                           unsigned int last_secs_recorded_) {
    vwap_ = (volume_ * vwap_ + _trd_print_info_.size_traded_ * _trd_print_info_.trade_price_) /
                  (volume_ + _trd_print_info_.size_traded_);
    volume_ += _trd_print_info_.size_traded_;
    num_trades_++;
  }
/*
we want to make sure this is the only function which changes unix_time_in_secs_ and nothing else
*/
  void IncreaseOneStep(int current_interval = false) {
    if (current_interval) {
      num_events_ = 1;
    }
    unix_time_in_secs_ += bar_length_in_secs_;
  }
/*
each record that needs to be dumped into DB/Flat_File
*/
  std::string ToString() {
    std::stringstream ss;
    ss << std::setprecision(10) 
       << unix_time_in_secs_ << " "
       << ex_sym_ << " "
       << num_events_ << " "
       << open_price_ << " "
       << low_price_ << " "
       << high_price_ << " "
       << close_price_ << " "
       << avg_bas_in_ticks_ << " "
       << vwap_ << " "
       << num_trades_ << " "
       << volume_ << "\n";
    return ss.str();
  }
};

/*
we use SMV change lisenter to capute l1_events + trade_events for a product
*/
class TestBook : public HFSAT::SecurityMarketViewChangeListener {
  HFSAT::SecurityMarketView *this_smv_;
  HFSAT::Watch &watch_;

  int start_unix_time_;
  int end_unix_time_;

  unsigned int barlength_in_secs_;
  DataBar data_bar_;

  unsigned int last_secs_recorded_;

  HFSAT::BulkFileWriter writer_;
  std::string filename_;
  int dump_count_;

 public:
  TestBook(HFSAT::SecurityMarketView *_this_smv_, HFSAT::Watch &_watch_, std::string& _exch_symbol_,
	   const int _start_unix_time_, const int _end_unix_time_, const int _barlength_in_secs_, 
	   const std::string &_filename_)
      : this_smv_(_this_smv_),
        watch_(_watch_),
        start_unix_time_(_start_unix_time_),
        end_unix_time_(_end_unix_time_),
        barlength_in_secs_(_barlength_in_secs_),
	data_bar_(barlength_in_secs_, start_unix_time_, _exch_symbol_.c_str()),
        last_secs_recorded_(0),
    writer_(_filename_, 16384, std::ios::out),
        filename_(_filename_),
        dump_count_(0) {


    // l1 and trade events
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
    // first time or crossed the line
    if (last_secs_recorded_ == 0 || 
	watch_.tv().tv_sec >= int(last_secs_recorded_ + barlength_in_secs_)) {

      if (last_secs_recorded_== 0) { // just reset
	// just setting prices 
	data_bar_.ResetFromMktUpdateInfo(watch_, _market_update_info_, 0);
      } else { // dump and reset
	DumpDataBar();
	int freq = (watch_.tv().tv_sec - last_secs_recorded_) / barlength_in_secs_;
	// there is no vol / samples / events 
	data_bar_.ResetFromMktUpdateInfo(watch_, _market_update_info_, freq);
	if (freq >= 1) {
	  for (auto i = 0; i < freq; i++) {
	    if (i < (freq - 1)) {
	      data_bar_.IncreaseOneStep();
	      DumpDataBar();
	    } else {
	      data_bar_.IncreaseOneStep();
	    }
	  }
	}
      }

      last_secs_recorded_ =
	(barlength_in_secs_ * ((watch_.tv().tv_sec / barlength_in_secs_)));
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

  inline bool IsPostEndTime() { 
    return (end_unix_time_ >= 1 && (int)watch_.msecs_from_midnight() > end_unix_time_);
  }

  inline void DumpDataBar() {
    //writer_.Write(&data_bar_, sizeof(data_bar_));
    writer_ << data_bar_.ToString();
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
    DataBar next_event;
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
// this I am hoping is absolutely useless 
  bool isNTP = (argc >= 4 && strcmp(argv[argc - 1], "NTP") == 0);
  bool isNTPORD = (argc >= 4 && strcmp(argv[argc - 1], "NTP_ORD") == 0);
  bool isBMFEq = (argc >= 4 && strcmp(argv[argc - 1], "BMF_EQ") == 0);
  bool isPL = (argc >= 4 && strcmp(argv[argc - 1], "PL") == 0);
  bool isQuincy = (argc >= 4 && strcmp(argv[argc - 1], "QUINCY") == 0);
  bool isOMD = (argc >= 4 && strcmp(argv[argc - 1], "HKOMD") == 0);
  bool isOMD_PF = (argc >= 4 && strcmp(argv[argc - 1], "HKOMD_PF") == 0);
  bool isOMD_CPF = (argc >= 4 && strcmp(argv[argc - 1], "HKOMD_CPF") == 0);
  bool isCHIXORD = (argc >= 4 && strcmp(argv[argc - 1], "CHIXORD") == 0);
  bool isCMEOBF = (argc >= 4 && strcmp(argv[argc - 1], "CMEOBF") == 0);

  bool set_book_type_ = false;
  if (isNTP || isNTPORD || isBMFEq || isQuincy || isOMD || isOMD_PF || isOMD_CPF || isCHIXORD || isCMEOBF) {
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
  int bardata_length_in_secs_ = 30;
  std::string network_account_info_filename_ = HFSAT::FileUtils::AppendHome(
      std::string(BASESYSINFODIR) + "TradingInfo/NetworkInfo/network_account_info_filename.txt");

  tradingdate_ = atoi(argv[read_idx_++]);
  if ((tradingdate_ < MIN_YYYYMMDD) || (tradingdate_ > MAX_YYYYMMDD)) {
    std::cerr << tradingdate_ << " not logical date" << std::endl;
    exit(0);
  }
  std::string outdir_ = argv[read_idx_++];
  std::ostringstream temp_oss_;
  temp_oss_ << outdir_ << "/B_" << _this_shortcode_ << "_" << tradingdate_;
  int t_size_ = 0;
  bool over_write_ = false;
  if (HFSAT::FileUtils::ExistsWithSize(temp_oss_.str(), t_size_) && !over_write_) {
    std::cout << "file already exists and not over-writing\n";
    exit(0);
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
                                   isCHIXORD, isCMEOBF);
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

  TestBook test_book_(p_smv_, watch_, exch_symbol, start_unix_time_, end_unix_time_, bardata_length_in_secs_, temp_oss_.str());

  // start event loop : Runs historical dispatcher, callbacks of MktUpdate, and TradePrint
  common_smv_source->Run();

  test_book_.DayOver();

  return 0;
}
